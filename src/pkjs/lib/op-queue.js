// PebbleKit JS: a persisted FIFO of SuperSync ops that failed to upload
// because of a transport failure - the phone had no connectivity, or the
// request timed out. It is flushed oldest-first at the top of the next
// doSync(), before any fresh op is built, so a change made on the watch
// while the phone was offline still reaches the server, in the order it
// happened, once the phone is back online.
//
// Only transport failures are queued. A server-side rejection (a
// vector-clock conflict, a quota error, a duplicate op id, a validation
// failure) is deliberately NOT queued: it will not turn into an acceptance
// on a later retry, and SuperSync's own operation-log ordering is the whole
// conflict-resolution story for this client (see README.md "Limitations").
//
// Each entry is a complete, already-encrypted op object exactly as
// buildTaskUpdateOp() / buildBacklogToRegularOp() / the habit + note + add-
// task builders produced it, so it re-uploads verbatim with no rebuild. The
// server dedupes by op id, so re-sending one that actually did land (its
// response lost to a dropped connection) is harmless - it comes back as a
// duplicate-op-id rejection and is dropped from the queue just the same.
//
// Every function takes the storage object (localStorage in the app, a fake
// Map-backed one in the tests) so the module stays pure and unit-testable.
'use strict';

var STORAGE_KEY = 'sp_pending_ops';

// Bounds the queue so a long offline stretch can't grow localStorage
// without limit. Oldest entries are dropped first when the cap is hit -
// losing the oldest un-synced change is the least-bad option once something
// has clearly gone wrong for a very long time.
var MAX_PENDING = 200;

function read(storage) {
  try {
    var raw = storage.getItem(STORAGE_KEY);
    if (!raw) {
      return [];
    }
    var arr = JSON.parse(raw);
    return Array.isArray(arr) ? arr : [];
  } catch (e) {
    return [];
  }
}

function write(storage, ops) {
  try {
    storage.setItem(STORAGE_KEY, JSON.stringify(ops));
  } catch (e) {
    // localStorage is full - the op is lost, which is exactly the outcome
    // that this queue exists to improve on but there is nothing useful to
    // do about it here.
  }
}

// Appends ops to the tail of the queue, skipping any whose id is already
// queued (guards against the same failed upload being enqueued twice), then
// trims from the front to MAX_PENDING.
function enqueue(storage, ops) {
  if (!ops || !ops.length) {
    return;
  }
  var queue = read(storage);
  var seen = {};
  var i;
  for (i = 0; i < queue.length; i++) {
    if (queue[i] && queue[i].id) {
      seen[queue[i].id] = true;
    }
  }
  for (i = 0; i < ops.length; i++) {
    var op = ops[i];
    if (!op || !op.id || seen[op.id]) {
      continue;
    }
    seen[op.id] = true;
    queue.push(op);
  }
  if (queue.length > MAX_PENDING) {
    queue = queue.slice(queue.length - MAX_PENDING);
  }
  write(storage, queue);
}

// The whole queue, oldest first.
function list(storage) {
  return read(storage);
}

// Drops the entry with this op id. Called once an op has either landed or
// been rejected by the server - both mean it should stop being retried.
function remove(storage, opId) {
  var queue = read(storage);
  var kept = [];
  for (var i = 0; i < queue.length; i++) {
    if (queue[i] && queue[i].id !== opId) {
      kept.push(queue[i]);
    }
  }
  write(storage, kept);
}

function count(storage) {
  return read(storage).length;
}

function clear(storage) {
  write(storage, []);
}

module.exports = {
  STORAGE_KEY: STORAGE_KEY,
  MAX_PENDING: MAX_PENDING,
  enqueue: enqueue,
  list: list,
  remove: remove,
  count: count,
  clear: clear,
};
