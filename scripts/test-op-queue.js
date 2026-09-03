// Sanity checks for the persisted offline op queue in
// src/pkjs/lib/op-queue.js. Run with: node scripts/test-op-queue.js
'use strict';

const assert = require('assert');
const q = require('../src/pkjs/lib/op-queue.js');

let failures = 0;
function check(name, fn) {
  try {
    fn();
    console.log(`ok   - ${name}`);
  } catch (err) {
    failures++;
    console.log(`FAIL - ${name}`);
    console.log(`       ${err.stack}`);
  }
}

// Map-backed stand-in for localStorage - the app passes the real one.
function fakeStorage(initial) {
  const map = Object.assign({}, initial || {});
  return {
    getItem(k) {
      return Object.prototype.hasOwnProperty.call(map, k) ? map[k] : null;
    },
    setItem(k, v) {
      map[k] = String(v);
    },
    removeItem(k) {
      delete map[k];
    },
    _raw() {
      return map;
    },
  };
}

function op(id) {
  return { id: id, actionType: '[Task Shared] updateTask', entityId: 't' + id, payload: 'enc' };
}

check('empty storage lists as []', () => {
  assert.deepStrictEqual(q.list(fakeStorage()), []);
  assert.strictEqual(q.count(fakeStorage()), 0);
});

check('enqueue then list preserves insertion order', () => {
  const s = fakeStorage();
  q.enqueue(s, [op('a')]);
  q.enqueue(s, [op('b'), op('c')]);
  assert.deepStrictEqual(q.list(s).map((o) => o.id), ['a', 'b', 'c']);
  assert.strictEqual(q.count(s), 3);
});

check('enqueue skips an op id already queued', () => {
  const s = fakeStorage();
  q.enqueue(s, [op('a'), op('b')]);
  q.enqueue(s, [op('b'), op('c')]);
  assert.deepStrictEqual(q.list(s).map((o) => o.id), ['a', 'b', 'c']);
});

check('enqueue ignores empty / null / id-less entries', () => {
  const s = fakeStorage();
  q.enqueue(s, null);
  q.enqueue(s, []);
  q.enqueue(s, [{ actionType: 'x' }]);
  assert.deepStrictEqual(q.list(s), []);
});

check('remove drops the matching id and keeps the rest in order', () => {
  const s = fakeStorage();
  q.enqueue(s, [op('a'), op('b'), op('c')]);
  q.remove(s, 'b');
  assert.deepStrictEqual(q.list(s).map((o) => o.id), ['a', 'c']);
  q.remove(s, 'missing');
  assert.deepStrictEqual(q.list(s).map((o) => o.id), ['a', 'c']);
});

check('clear empties the queue', () => {
  const s = fakeStorage();
  q.enqueue(s, [op('a'), op('b')]);
  q.clear(s);
  assert.deepStrictEqual(q.list(s), []);
});

check('queue is trimmed from the front at MAX_PENDING', () => {
  const s = fakeStorage();
  const many = [];
  for (let i = 0; i < q.MAX_PENDING + 5; i++) {
    many.push(op('n' + i));
  }
  q.enqueue(s, many);
  const ids = q.list(s).map((o) => o.id);
  assert.strictEqual(ids.length, q.MAX_PENDING);
  assert.strictEqual(ids[0], 'n5'); // first 5 dropped
  assert.strictEqual(ids[ids.length - 1], 'n' + (q.MAX_PENDING + 4));
});

check('corrupt stored value reads back as an empty queue', () => {
  const s = fakeStorage({ sp_pending_ops: 'not json{' });
  assert.deepStrictEqual(q.list(s), []);
  // and a fresh enqueue still works over the top of it
  q.enqueue(s, [op('a')]);
  assert.deepStrictEqual(q.list(s).map((o) => o.id), ['a']);
});

check('a non-array stored value reads back as an empty queue', () => {
  const s = fakeStorage({ sp_pending_ops: '{"id":"a"}' });
  assert.deepStrictEqual(q.list(s), []);
});

check('entries round-trip through JSON unchanged', () => {
  const s = fakeStorage();
  const full = {
    id: 'op-x',
    opType: 'UPD',
    actionType: '[Task Shared] updateTask',
    entityType: 'TASK',
    entityId: 't1',
    payload: { some: 'encrypted-blob', n: 3 },
    isPayloadEncrypted: true,
    vectorClock: { 'pebble-1': 4 },
    clientId: 'pebble-1',
    timestamp: 1234567890,
    schemaVersion: 4,
  };
  q.enqueue(s, [full]);
  assert.deepStrictEqual(q.list(s)[0], full);
});

console.log('');
if (failures > 0) {
  console.log(`${failures} check(s) FAILED`);
  process.exit(1);
} else {
  console.log('All checks passed.');
}
