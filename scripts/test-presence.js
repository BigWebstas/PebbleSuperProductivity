// Unit tests for src/pkjs/lib/presence-client.js - the live-tracking presence
// viewer (Phase 1). Covers envelope decode (plaintext + E2EE + fail-closed),
// ordinal/seq dedupe, the stopped-linger clear, ping/pong, and the remote-stop
// command shape. No network: a fake WebSocket is installed as global.WebSocket.
//
// Run with: node scripts/test-presence.js
'use strict';

const assert = require('assert');
const { createCrypto } = require('../src/pkjs/lib/supersync-client.js');

let failures = 0;
const tests = [];
function check(name, fn) {
  tests.push({ name, fn });
}
const delay = (ms) => new Promise((r) => setTimeout(r, ms));

// ---- fake WebSocket -------------------------------------------------------

let lastSocket = null;
class FakeWebSocket {
  constructor(url) {
    this.url = url;
    this.readyState = 0; // CONNECTING
    this.sent = [];
    this.closed = null;
    this.onopen = null;
    this.onmessage = null;
    this.onclose = null;
    this.onerror = null;
    lastSocket = this;
  }
  send(data) {
    this.sent.push(JSON.parse(data));
  }
  close(code, reason) {
    this.readyState = 3;
    this.closed = { code, reason };
    if (this.onclose) {
      this.onclose({ code: code || 1000, reason: reason || '' });
    }
  }
  // test helpers
  _open() {
    this.readyState = 1;
    if (this.onopen) this.onopen();
  }
  _emit(obj) {
    if (this.onmessage) this.onmessage({ data: JSON.stringify(obj) });
  }
  _sentOfType(type) {
    return this.sent.filter((m) => m.type === type);
  }
}
global.WebSocket = FakeWebSocket;

const { PresenceClient } = require('../src/pkjs/lib/presence-client.js');

function newClient(over) {
  const opts = Object.assign(
    {
      baseUrl: 'https://sync.example.com',
      token: 'jwt-abc',
      clientId: 'pebble-test',
      getCrypto: () => null,
      log: () => {},
      tuning: { lingerMs: 10, livenessMs: 10000, minReconnectMs: 5, heartbeatMs: 10000 },
    },
    over || {}
  );
  const c = new PresenceClient(opts);
  c.connect();
  lastSocket._open();
  return c;
}

function envelopePlain(payload) {
  return JSON.stringify({ enc: false, data: JSON.stringify(payload) });
}

const TRACKING = {
  v: 1,
  sessionId: 's1',
  seq: 1,
  state: 'tracking',
  taskId: 'task-42',
  sinceTs: 1700000000000,
  deviceLabel: 'Desktop',
};

// ---- tests --------------------------------------------------------------

check('connect builds the ws:// URL with token + clientId', () => {
  newClient();
  assert.strictEqual(
    lastSocket.url,
    'wss://sync.example.com/api/sync/ws?token=jwt-abc&clientId=pebble-test'
  );
});

check('replies to a server ping with a pong', () => {
  newClient();
  lastSocket._emit({ type: 'ping' });
  assert.deepStrictEqual(lastSocket._sentOfType('pong'), [{ type: 'pong' }]);
});

check('decodes a plaintext presence_state and emits the fields', () => {
  const c = newClient();
  let got = null;
  c.onState((s) => { got = s; });
  lastSocket._emit({ type: 'presence_state', payload: envelopePlain(TRACKING), ordinal: 1, producerConnected: true });
  assert.ok(got);
  assert.strictEqual(got.state, 'tracking');
  assert.strictEqual(got.taskId, 'task-42');
  assert.strictEqual(got.deviceLabel, 'Desktop');
  assert.strictEqual(got.sessionId, 's1');
  assert.strictEqual(got.opaque, false);
});

check('sanitizes a hostile deviceLabel', () => {
  const c = newClient();
  let got = null;
  c.onState((s) => { got = s; });
  const hostile = Object.assign({}, TRACKING, { deviceLabel: '<img src=x>' + 'A'.repeat(80) });
  lastSocket._emit({ type: 'presence_state', payload: envelopePlain(hostile), ordinal: 1 });
  assert.ok(got.deviceLabel.indexOf('<') === -1 && got.deviceLabel.indexOf('>') === -1);
  assert.ok(got.deviceLabel.length <= 32);
});

check('drops a lower ordinal (stale cross-device order)', () => {
  const c = newClient();
  const seen = [];
  c.onState((s) => seen.push(s.seq));
  lastSocket._emit({ type: 'presence_state', payload: envelopePlain(Object.assign({}, TRACKING, { seq: 5 })), ordinal: 3 });
  lastSocket._emit({ type: 'presence_state', payload: envelopePlain(Object.assign({}, TRACKING, { seq: 6 })), ordinal: 2 });
  assert.deepStrictEqual(seen, [5]);
});

check('drops a lower seq within the same session', () => {
  const c = newClient();
  const seen = [];
  c.onState((s) => seen.push(s.seq));
  lastSocket._emit({ type: 'presence_state', payload: envelopePlain(Object.assign({}, TRACKING, { seq: 4 })), ordinal: 1 });
  lastSocket._emit({ type: 'presence_state', payload: envelopePlain(Object.assign({}, TRACKING, { seq: 2 })), ordinal: 2 });
  lastSocket._emit({ type: 'presence_state', payload: envelopePlain(Object.assign({}, TRACKING, { seq: 7 })), ordinal: 3 });
  assert.deepStrictEqual(seen, [4, 7]);
});

check('an equal ordinal passes (re-announcement, producerConnected flip)', () => {
  const c = newClient();
  const conns = [];
  c.onState((s) => conns.push(s.producerConnected));
  lastSocket._emit({ type: 'presence_state', payload: envelopePlain(TRACKING), ordinal: 4, producerConnected: true });
  lastSocket._emit({ type: 'presence_state', payload: envelopePlain(TRACKING), ordinal: 4, producerConnected: false });
  assert.deepStrictEqual(conns, [true, false]);
});

check('a plain stopped lingers then clears', async () => {
  const c = newClient();
  let cleared = false;
  c.onCleared(() => { cleared = true; });
  c.onState(() => {});
  lastSocket._emit({ type: 'presence_state', payload: envelopePlain(TRACKING), ordinal: 1 });
  lastSocket._emit({ type: 'presence_state', payload: envelopePlain(Object.assign({}, TRACKING, { seq: 2, state: 'stopped' })), ordinal: 2 });
  assert.strictEqual(cleared, false, 'not cleared immediately');
  await delay(30);
  assert.strictEqual(cleared, true, 'cleared after linger');
});

check('an idle-paused stopped does NOT linger-clear', async () => {
  const c = newClient();
  let cleared = false;
  let last = null;
  c.onCleared(() => { cleared = true; });
  c.onState((s) => { last = s; });
  lastSocket._emit({ type: 'presence_state', payload: envelopePlain(TRACKING), ordinal: 1 });
  lastSocket._emit({
    type: 'presence_state',
    payload: envelopePlain(Object.assign({}, TRACKING, { seq: 2, state: 'stopped', reason: 'idle' })),
    ordinal: 2,
  });
  assert.strictEqual(last.reason, 'idle');
  await delay(30);
  assert.strictEqual(cleared, false);
});

check('requestStop sends a presence_cmd naming the session', () => {
  const c = newClient();
  c.onState(() => {});
  lastSocket._emit({ type: 'presence_state', payload: envelopePlain(TRACKING), ordinal: 1 });
  c.requestStop('s1');
  const cmds = lastSocket._sentOfType('presence_cmd');
  assert.strictEqual(cmds.length, 1);
  const env = JSON.parse(cmds[0].payload);
  assert.strictEqual(env.enc, false);
  const cmd = JSON.parse(env.data);
  assert.deepStrictEqual(cmd, { v: 1, cmd: 'stop', sessionId: 's1', deviceLabel: 'Pebble' });
});

check('disconnect fires onCleared and stops reconnecting', () => {
  const c = newClient();
  let cleared = false;
  c.onCleared(() => { cleared = true; });
  c.onState(() => {});
  lastSocket._emit({ type: 'presence_state', payload: envelopePlain(TRACKING), ordinal: 1 });
  const sock = lastSocket;
  c.disconnect();
  assert.strictEqual(cleared, true);
  assert.strictEqual(sock.closed.code, 1000);
  assert.strictEqual(c.isConnected(), false);
});

check('does not reconnect on auth-failure close code 4003', async () => {
  newClient();
  const first = lastSocket;
  first.onclose({ code: 4003 });
  await delay(20);
  assert.strictEqual(lastSocket, first, 'no new socket after 4003');
});

check('reconnects on an unexpected close', async () => {
  newClient();
  const first = lastSocket;
  first.onclose({ code: 1006 });
  await delay(20);
  assert.notStrictEqual(lastSocket, first, 'a new socket was created');
});

// ---- E2EE envelope (one real Argon2id derivation) -----------------------

check('E2EE: decodes a ciphertext whose salt is cached; refuses plaintext', () => {
  const PASSWORD = 'jwebstas@gmail.com-presence-test';
  const keyStore = {};
  let encSalt = null;
  const persistence = {
    loadKeys: () => Object.assign({}, keyStore),
    saveKey: (s, k) => { keyStore[s] = k; },
    loadEncryptSalt: () => encSalt,
    saveEncryptSalt: (s) => { encSalt = s; },
  };
  // "Producer" crypto encrypts a presence payload; its derive seeds keyStore,
  // so a viewer crypto built from the same persistence is a cache hit.
  const producer = createCrypto(PASSWORD, persistence);
  const ciphertext = producer.encrypt(TRACKING);
  const viewer = createCrypto(PASSWORD, persistence);

  const c = newClient({ getCrypto: () => viewer });
  let got = null;
  c.onState((s) => { got = s; });

  lastSocket._emit({
    type: 'presence_state',
    payload: JSON.stringify({ enc: true, data: ciphertext }),
    ordinal: 1,
  });
  assert.ok(got && !got.opaque, 'decoded the ciphertext');
  assert.strictEqual(got.taskId, 'task-42');

  got = null;
  lastSocket._emit({ type: 'presence_state', payload: envelopePlain(TRACKING), ordinal: 2 });
  assert.ok(got && got.opaque && got.reason === 'plaintext');
});

check('E2EE: an uncached salt is shown opaquely, never derived inline', () => {
  let inlineCalls = 0;
  const viewer = {
    canDecryptWithoutDerive: () => false,
    decrypt: () => { inlineCalls++; throw new Error('stop'); },
    encrypt: () => 'x',
  };
  const c = newClient({ getCrypto: () => viewer });
  let got = null;
  c.onState((s) => { got = s; });
  lastSocket._emit({
    type: 'presence_state',
    payload: JSON.stringify({ enc: true, data: 'AAAAAAAAAAAAAAAAAAAAAAAAAAAA' }),
    ordinal: 1,
  });
  // Opaque emitted synchronously; no inline decrypt on the message path.
  assert.ok(got && got.opaque && got.reason === 'needs-derive');
  assert.strictEqual(inlineCalls, 0);
});

check('E2EE: an uncached salt is derived once in the background, then shown', async () => {
  let deriveCalls = 0;
  const real = {
    v: 1, sessionId: 'a1', seq: 3, state: 'tracking',
    taskId: 'task-77', sinceTs: 1700000000000, deviceLabel: 'Android',
  };
  const viewer = {
    canDecryptWithoutDerive: () => false,
    decrypt: () => { deriveCalls++; return real; },
    encrypt: () => 'x',
  };
  const c = newClient({ getCrypto: () => viewer });
  const seen = [];
  c.onState((s) => { seen.push(s); });
  const frame = { type: 'presence_state', payload: JSON.stringify({ enc: true, data: 'S'.repeat(40) }), ordinal: 1 };
  lastSocket._emit(frame);
  assert.ok(seen[0].opaque, 'first emit opaque');
  await delay(5);
  assert.strictEqual(deriveCalls, 1, 'derived exactly once');
  assert.strictEqual(seen[seen.length - 1].opaque, false);
  assert.strictEqual(seen[seen.length - 1].taskId, 'task-77');
  assert.strictEqual(seen[seen.length - 1].deviceLabel, 'Android');
  // A repeat frame (same salt) must not schedule a second derive. (With the
  // real crypto the salt is cached after the first derive and decodes inline;
  // only this always-false mock keeps reporting needs-derive.)
  lastSocket._emit({ type: 'presence_state', payload: frame.payload, ordinal: 2 });
  await delay(5);
  assert.strictEqual(deriveCalls, 1, 'no second derive for the same salt');
});

// ---- producer (Phase 2) ------------------------------------------------

function lastStateSent() {
  const states = lastSocket._sentOfType('presence_state');
  if (!states.length) return null;
  const env = JSON.parse(states[states.length - 1].payload);
  return JSON.parse(env.data);
}

check('broadcastTracking sends a tracking state labelled Pebble', () => {
  const c = newClient();
  c.broadcastTracking('task-9', 1700000000000);
  const p = lastStateSent();
  assert.strictEqual(p.state, 'tracking');
  assert.strictEqual(p.taskId, 'task-9');
  assert.strictEqual(p.deviceLabel, 'Pebble');
  assert.strictEqual(p.sinceTs, 1700000000000);
  assert.strictEqual(p.v, 1);
  assert.ok(typeof p.sessionId === 'string' && p.sessionId.length > 0);
  assert.ok(c.isBroadcasting());
});

check('same task keeps the session and bumps seq; a new task starts a new one', () => {
  const c = newClient();
  c.broadcastTracking('task-9', 1000);
  const first = lastStateSent();
  c.broadcastTracking('task-9', 1000);
  const second = lastStateSent();
  assert.strictEqual(second.sessionId, first.sessionId);
  assert.ok(second.seq > first.seq);
  c.broadcastTracking('task-X', 2000);
  const third = lastStateSent();
  assert.notStrictEqual(third.sessionId, first.sessionId);
});

check('broadcastStopped sends stopped then clears the producer', () => {
  const c = newClient();
  c.broadcastTracking('task-9', 1000);
  c.broadcastStopped();
  assert.strictEqual(lastStateSent().state, 'stopped');
  assert.strictEqual(c.isBroadcasting(), false);
  c.broadcastStopped(); // no-op, no throw
});

check('the heartbeat re-announces the tracking state', async () => {
  const c = newClient({ tuning: { heartbeatMs: 15, lingerMs: 10, livenessMs: 10000, minReconnectMs: 5 } });
  c.broadcastTracking('task-9', 1000);
  const before = lastSocket._sentOfType('presence_state').length;
  await delay(40);
  assert.ok(lastSocket._sentOfType('presence_state').length > before, 'heartbeat sent more states');
  c.broadcastStopped();
});

check('a reconnect re-announces the tracking state', async () => {
  const c = newClient();
  c.broadcastTracking('task-9', 1000);
  const old = lastSocket;
  old.onclose({ code: 1006 });
  await delay(20); // let the backoff reconnect fire
  assert.notStrictEqual(lastSocket, old, 'a new socket was created');
  lastSocket._open();
  assert.strictEqual(lastStateSent().state, 'tracking');
});

check('a presence_cmd stop for our session fires onStopCommand', () => {
  const c = newClient();
  let stopped = 0;
  c.onStopCommand(() => { stopped++; });
  c.broadcastTracking('task-9', 1000);
  const sess = lastStateSent().sessionId;
  const env = JSON.stringify({ enc: false, data: JSON.stringify({ v: 1, cmd: 'stop', sessionId: sess }) });
  lastSocket._emit({ type: 'presence_cmd', payload: env });
  assert.strictEqual(stopped, 1);
  // A stale sessionId is ignored.
  const stale = JSON.stringify({ enc: false, data: JSON.stringify({ v: 1, cmd: 'stop', sessionId: 'nope' }) });
  lastSocket._emit({ type: 'presence_cmd', payload: stale });
  assert.strictEqual(stopped, 1);
});

check('presence_cmd is ignored when we are not broadcasting', () => {
  const c = newClient();
  let stopped = 0;
  c.onStopCommand(() => { stopped++; });
  const env = JSON.stringify({ enc: false, data: JSON.stringify({ v: 1, cmd: 'stop', sessionId: 'x' }) });
  lastSocket._emit({ type: 'presence_cmd', payload: env });
  assert.strictEqual(stopped, 0);
});

check('disconnect stops broadcasting', () => {
  const c = newClient();
  c.broadcastTracking('task-9', 1000);
  c.disconnect();
  assert.strictEqual(c.isBroadcasting(), false);
});

// ---- runner ------------------------------------------------------------

(async () => {
  for (const t of tests) {
    try {
      await t.fn();
      console.log(`ok   - ${t.name}`);
    } catch (err) {
      failures++;
      console.log(`FAIL - ${t.name}`);
      console.log(`       ${err.stack}`);
    }
  }
  console.log('');
  if (failures > 0) {
    console.log(`${failures} check(s) FAILED`);
    process.exit(1);
  }
  console.log('All checks passed.');
  // Tests deliberately leave sockets / heartbeats running (no per-test
  // teardown); exit rather than let those timers keep the process alive.
  process.exit(0);
})();
