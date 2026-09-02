// Live tracking presence over the SuperSync WebSocket (super-productivity
// desktop v18.21.1, PR #9771). Phase 1: VIEWER + remote stop only - this
// client renders what another device is tracking and can ask that device to
// stop; it does not broadcast its own tracking state (that is Phase 2).
//
// Wire contract mirrored from the super-productivity repo (UNVERIFIED against
// a live account, same caveat as supersync-client.js's REST routes):
//   src/app/op-log/sync/super-sync-websocket.service.ts   - socket + framing
//   packages/super-sync-server/.../websocket-connection.service.ts - relay
//   src/app/features/tracking-presence/tracking-presence.service.ts - codec
//   src/app/features/tracking-presence/tracking-presence.model.ts   - shapes
//
// Transport summary:
//   URL   wss://<host>/api/sync/ws?token=<jwt>&clientId=<id>   (http->ws swap)
//   in    {type:"connected"} once; {type:"ping"} ~30s -> reply {type:"pong"};
//         {type:"presence_state", payload, ordinal, producerConnected};
//         {type:"presence_cmd", payload}   (ignored here - we only send these)
//   out   {type:"pong"}; {type:"presence_cmd", payload}
//   close 4003 auth / 4008 conn-limit / 4009 replaced -> do NOT reconnect
//
// `payload` is JSON.stringify({ enc, data }). When enc, `data` is an
// AES-GCM/Argon2id blob in supersync-client.js's exact op-payload format, so
// the crypto object from createCrypto() decodes it directly. Fail closed:
// a plaintext envelope while a key is configured is dropped, not trusted.
'use strict';

// presence_state payload, after decode (tracking-presence.model.ts):
//   { v:1, sessionId, seq, state:"tracking"|"stopped", reason?:"idle",
//     taskId:string|null, sinceTs:number, deviceLabel:string, focusCycle?:number }

// Viewer staleness / linger windows (tracking-presence.model.ts constants).
var STALE_AFTER_MS = 90 * 1000;
var STOPPED_LINGER_MS = 10 * 1000;

// Socket liveness: server pings ~30s, so silence past this means a dead pipe.
var LIVENESS_TIMEOUT_MS = 45 * 1000;

var MIN_RECONNECT_MS = 1000;
var MAX_RECONNECT_MS = 60 * 1000;
var MAX_RECONNECT_ATTEMPTS = 50;

// Close codes the server uses to say "don't come back on your own".
var NO_RECONNECT_CLOSE_CODES = { 4003: 1, 4008: 1, 4009: 1 };

function noop() {}

// Strip markup-capable chars and cap length - deviceLabel is relayed from
// another device and, with E2EE off, a hostile server could inject it
// (mirrors sanitizeDeviceLabel in tracking-presence.service.ts).
function sanitizeDeviceLabel(v) {
  if (typeof v !== 'string') {
    return '';
  }
  return v.replace(/[<>&"'`]/g, '').slice(0, 32);
}

// opts: { baseUrl, token, clientId, getCrypto, log? }
//   getCrypto: () => crypto|null   (called lazily so it always reflects the
//                                   current pairing, like index.js's getCrypto)
//   log:       optional (msg) => void, defaults to console.log with a prefix
function PresenceClient(opts) {
  this._baseUrl = opts.baseUrl;
  this._token = opts.token;
  this._clientId = opts.clientId;
  this._getCrypto = opts.getCrypto || function () { return null; };
  this._log = opts.log || function (m) { console.log('[presence] ' + m); };

  // Timers, overridable so tests don't wait real seconds.
  var t = opts.tuning || {};
  this._lingerMs = t.lingerMs || STOPPED_LINGER_MS;
  this._livenessMs = t.livenessMs || LIVENESS_TIMEOUT_MS;
  this._minReconnectMs = t.minReconnectMs || MIN_RECONNECT_MS;
  this._maxReconnectMs = t.maxReconnectMs || MAX_RECONNECT_MS;

  this._ws = null;
  this._intentionalClose = false;
  this._reconnectAttempts = 0;
  this._reconnectTimer = null;
  this._livenessTimer = null;
  this._lingerTimer = null;

  // Dedupe / current-view state.
  this._lastOrdinal = -1;
  this._current = null; // last emitted session view, or null

  this._onStateCb = noop;
  this._onClearedCb = noop;
}

// cb({ state, reason, taskId, sinceTs, deviceLabel, sessionId, seq,
//      producerConnected, ordinal, receivedAt, opaque? })
// `opaque:true` (+ a `reason` of 'no-key' | 'needs-derive' | 'plaintext')
// means the payload could not be decoded here - render a device-less
// "tracking on another device" with no task title and no Stop.
PresenceClient.prototype.onState = function (cb) {
  this._onStateCb = cb || noop;
};

// cb() - the shown session should be hidden (linger elapsed, or disconnect).
PresenceClient.prototype.onCleared = function (cb) {
  this._onClearedCb = cb || noop;
};

PresenceClient.prototype.isConnected = function () {
  return !!this._ws && this._ws.readyState === 1;
};

PresenceClient.prototype.connect = function () {
  this._intentionalClose = false;
  if (this._ws && (this._ws.readyState === 0 || this._ws.readyState === 1)) {
    return;
  }
  this._open();
};

PresenceClient.prototype.disconnect = function () {
  this._intentionalClose = true;
  this._clearTimer('_reconnectTimer');
  this._clearTimer('_livenessTimer');
  this._clearTimer('_lingerTimer');
  if (this._ws) {
    try {
      this._ws.close(1000, 'client disconnect');
    } catch (e) {
      // already closing/closed
    }
    this._ws = null;
  }
  if (this._current) {
    this._current = null;
    this._lastOrdinal = -1;
    this._onClearedCb();
  }
};

// Ask the device that owns `sessionId` to stop. Fire-and-forget: the viewer
// UI clears on that device's own "stopped" broadcast, never optimistically
// (the producer ignores a stale sessionId - CAS guard).
PresenceClient.prototype.requestStop = function (sessionId) {
  if (!sessionId || !this.isConnected()) {
    return;
  }
  var cmd = { v: 1, cmd: 'stop', sessionId: sessionId, deviceLabel: 'Pebble' };
  var envelope = this._encodeEnvelope(cmd);
  if (!envelope) {
    this._log('requestStop: could not encode cmd envelope');
    return;
  }
  this._send({ type: 'presence_cmd', payload: JSON.stringify(envelope) });
};

// ---------------- socket ----------------

PresenceClient.prototype._open = function () {
  var wsUrl = this._baseUrl.replace(/^https:/i, 'wss:').replace(/^http:/i, 'ws:');
  var url = wsUrl + '/api/sync/ws?token=' + encodeURIComponent(this._token) +
    '&clientId=' + encodeURIComponent(this._clientId);

  var ws;
  try {
    ws = new WebSocket(url);
  } catch (e) {
    this._log('WebSocket construct failed: ' + (e && e.message));
    this._scheduleReconnect();
    return;
  }
  this._ws = ws;
  var self = this;

  ws.onopen = function () {
    if (self._ws !== ws) {
      return;
    }
    self._log('connected');
    self._reconnectAttempts = 0;
    self._armLiveness();
  };

  ws.onmessage = function (event) {
    if (self._ws !== ws) {
      return;
    }
    self._armLiveness();
    var msg;
    try {
      msg = JSON.parse(event.data);
    } catch (e) {
      return; // non-JSON frame, ignore
    }
    self._handleFrame(msg);
  };

  ws.onclose = function (event) {
    if (self._ws !== ws) {
      return;
    }
    self._ws = null;
    self._clearTimer('_livenessTimer');
    var code = event && event.code;
    if (self._intentionalClose || NO_RECONNECT_CLOSE_CODES[code]) {
      self._log('closed (' + code + '), not reconnecting');
      return;
    }
    self._log('closed (' + code + '), will reconnect');
    self._scheduleReconnect();
  };

  ws.onerror = function () {
    if (self._ws !== ws) {
      return;
    }
    // A close event follows; reconnect is handled there.
    self._log('socket error');
  };
};

PresenceClient.prototype._send = function (obj) {
  if (!this.isConnected()) {
    return;
  }
  try {
    this._ws.send(JSON.stringify(obj));
  } catch (e) {
    this._log('send failed: ' + (e && e.message));
  }
};

PresenceClient.prototype._handleFrame = function (msg) {
  switch (msg && msg.type) {
    case 'ping':
      this._send({ type: 'pong' });
      break;
    case 'connected':
    case 'new_ops':
      break; // op sync stays on the REST path
    case 'presence_state':
      if (typeof msg.payload === 'string' && typeof msg.ordinal === 'number') {
        this._onPresenceState(msg.payload, msg.ordinal, msg.producerConnected !== false);
      }
      break;
    case 'presence_cmd':
      break; // Phase 1 viewer: we send these, never act on them
    default:
      break;
  }
};

// ---------------- reconnect / liveness ----------------

PresenceClient.prototype._scheduleReconnect = function () {
  if (this._intentionalClose || this._reconnectTimer) {
    return;
  }
  if (this._reconnectAttempts >= MAX_RECONNECT_ATTEMPTS) {
    this._log('giving up after ' + MAX_RECONNECT_ATTEMPTS + ' reconnect attempts');
    return;
  }
  this._reconnectAttempts++;
  var delay = Math.min(this._minReconnectMs * Math.pow(2, this._reconnectAttempts - 1), this._maxReconnectMs);
  delay = Math.round(delay * (0.9 + Math.random() * 0.2)); // +/-10% jitter
  var self = this;
  this._reconnectTimer = setTimeout(function () {
    self._reconnectTimer = null;
    if (!self._intentionalClose) {
      self._open();
    }
  }, delay);
};

PresenceClient.prototype._armLiveness = function () {
  this._clearTimer('_livenessTimer');
  var self = this;
  this._livenessTimer = setTimeout(function () {
    self._livenessTimer = null;
    self._log('liveness timeout, forcing reconnect');
    if (self._ws) {
      try {
        self._ws.close(4000, 'liveness timeout');
      } catch (e) {
        self._ws = null;
        self._scheduleReconnect();
      }
    }
  }, this._livenessMs);
};

PresenceClient.prototype._clearTimer = function (name) {
  if (this[name]) {
    clearTimeout(this[name]);
    this[name] = null;
  }
};

// ---------------- presence codec ----------------

// Returns { enc, data } or null.
PresenceClient.prototype._encodeEnvelope = function (obj) {
  var crypto = this._getCrypto();
  if (crypto) {
    try {
      return { enc: true, data: crypto.encrypt(obj) };
    } catch (e) {
      this._log('encrypt failed: ' + (e && e.message));
      return null;
    }
  }
  return { enc: false, data: JSON.stringify(obj) };
};

// Returns one of:
//   { payload: <decoded object> }
//   { opaque: 'no-key' | 'needs-derive' | 'plaintext' }
//   null   (malformed - drop silently)
PresenceClient.prototype._decodeEnvelope = function (payloadStr) {
  var envelope;
  try {
    envelope = JSON.parse(payloadStr);
  } catch (e) {
    return null;
  }
  if (!envelope || typeof envelope.data !== 'string') {
    return null;
  }
  var crypto = this._getCrypto();
  if (envelope.enc) {
    if (!crypto) {
      return { opaque: 'no-key' };
    }
    if (!crypto.canDecryptWithoutDerive(envelope.data)) {
      // Deriving Argon2id here would stall the socket for tens of seconds.
      return { opaque: 'needs-derive' };
    }
    try {
      return { payload: crypto.decrypt(envelope.data) };
    } catch (e) {
      this._log('decrypt failed: ' + (e && e.message));
      return null;
    }
  }
  if (crypto) {
    // Encryption configured but the envelope is plaintext - hostile-server
    // guard, fail closed (tracking-presence.service.ts does the same).
    return { opaque: 'plaintext' };
  }
  try {
    return { payload: JSON.parse(envelope.data) };
  } catch (e) {
    return null;
  }
};

PresenceClient.prototype._onPresenceState = function (payloadStr, ordinal, producerConnected) {
  // Server-assigned ordinal orders states across devices without trusting
  // their clocks. Equal ordinals are re-announcements (producerConnected
  // flipped) and must pass.
  if (ordinal < this._lastOrdinal) {
    return;
  }

  var decoded = this._decodeEnvelope(payloadStr);
  if (!decoded) {
    return;
  }
  this._lastOrdinal = ordinal;

  if (decoded.opaque) {
    this._clearTimer('_lingerTimer');
    this._current = {
      opaque: decoded.opaque,
      producerConnected: producerConnected,
      ordinal: ordinal,
      receivedAt: Date.now(),
    };
    this._onStateCb({
      opaque: true,
      reason: decoded.opaque,
      state: 'tracking',
      taskId: null,
      sessionId: null,
      deviceLabel: '',
      sinceTs: 0,
      seq: 0,
      producerConnected: producerConnected,
      ordinal: ordinal,
      receivedAt: this._current.receivedAt,
    });
    return;
  }

  var p = decoded.payload;
  if (!p || p.v !== 1 ||
      typeof p.sessionId !== 'string' ||
      (p.state !== 'tracking' && p.state !== 'stopped') ||
      typeof p.seq !== 'number' || !isFinite(p.seq) ||
      typeof p.sinceTs !== 'number' || !isFinite(p.sinceTs) ||
      (p.taskId !== null && typeof p.taskId !== 'string')) {
    return;
  }

  // Same session, older producer seq - a straggler, drop it.
  if (this._current && !this._current.opaque &&
      this._current.sessionId === p.sessionId && p.seq < this._current.seq) {
    return;
  }

  this._clearTimer('_lingerTimer');

  var view = {
    opaque: false,
    state: p.state,
    reason: p.reason === 'idle' ? 'idle' : undefined,
    taskId: p.taskId,
    sinceTs: p.sinceTs,
    deviceLabel: sanitizeDeviceLabel(p.deviceLabel),
    sessionId: p.sessionId,
    seq: p.seq,
    producerConnected: producerConnected,
    ordinal: ordinal,
    receivedAt: Date.now(),
  };
  this._current = view;
  this._onStateCb(view);

  // A plain "stopped" (task switched, or user stopped) lingers briefly so a
  // stop+start within seconds mutates the surface in place. An idle pause
  // (reason:"idle") stays visible as "Paused" until superseded.
  if (p.state === 'stopped' && p.reason !== 'idle') {
    var self = this;
    this._lingerTimer = setTimeout(function () {
      self._lingerTimer = null;
      self._current = null;
      self._lastOrdinal = -1;
      self._onClearedCb();
    }, this._lingerMs);
  }
};

module.exports = {
  PresenceClient: PresenceClient,
  STALE_AFTER_MS: STALE_AFTER_MS,
  STOPPED_LINGER_MS: STOPPED_LINGER_MS,
  sanitizeDeviceLabel: sanitizeDeviceLabel,
};
