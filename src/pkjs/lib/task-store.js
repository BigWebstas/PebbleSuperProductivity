// Maintains a local cache of SuperSync entities (rebuilt by replaying
// operations) and derives "today's" task list from it.
//
// See the top-of-file comment in supersync-client.js for the operation
// field-name assumptions this replay logic depends on.
'use strict';

var supersync = require('./supersync-client.js');

function todayStr() {
  var d = new Date();
  var mm = ('0' + (d.getMonth() + 1)).slice(-2);
  var dd = ('0' + d.getDate()).slice(-2);
  return d.getFullYear() + '-' + mm + '-' + dd;
}

// state: { task: { [id]: {id, title, isDone, dueDay, ...} }, ... }
function emptyState() {
  return { task: {} };
}

function ensureCollection(state, entityType) {
  if (!state[entityType]) {
    state[entityType] = {};
  }
  return state[entityType];
}

// Applies one SuperSync operation to `state` in place. `key` is the derived
// AES-256 key (byte array) if E2EE is on, or null/undefined otherwise.
// Never throws - a single malformed/unrecognized op should not take down
// the whole sync (it just means that entity may be stale until next
// snapshot restore).
function applyOperation(op, state, key) {
  try {
    var payload = op.payload;
    if (op.encrypted && payload && key) {
      payload = supersync.decryptPayload(payload, key);
    }

    switch (op.type) {
      case 'CRT': {
        var created = ensureCollection(state, op.entityType);
        created[op.entityId] = payload;
        break;
      }
      case 'UPD': {
        var coll = ensureCollection(state, op.entityType);
        coll[op.entityId] = Object.assign({}, coll[op.entityId], payload);
        break;
      }
      case 'DEL': {
        var delColl = ensureCollection(state, op.entityType);
        if (payload && Array.isArray(payload.ids)) {
          payload.ids.forEach(function (id) { delete delColl[id]; });
        } else {
          delete delColl[op.entityId];
        }
        break;
      }
      case 'MOV':
        // Ordering only - doesn't affect which tasks we show on the watch.
        break;
      case 'SYNC_IMPORT':
      case 'BACKUP_IMPORT':
      case 'REPAIR':
        // These are expected to carry a full-state replacement. Shape is
        // unconfirmed (see supersync-client.js header comment); best-effort
        // merge if it looks like { task: { entities: {...} } }.
        if (payload && payload.task && payload.task.entities) {
          state.task = payload.task.entities;
        }
        break;
      default:
        console.log('[task-store] unhandled op type: ' + op.type);
    }
  } catch (err) {
    console.log('[task-store] failed to apply op ' + (op && op.id) + ': ' + err.message);
  }
}

function applyOperations(ops, state, key) {
  ops.forEach(function (op) {
    applyOperation(op, state, key);
  });
}

// Returns up to `limit` tasks relevant to "today", not-done first.
function getTodayTasks(state, limit) {
  var today = todayStr();
  var all = Object.keys(state.task || {}).map(function (id) {
    return state.task[id];
  }).filter(Boolean);

  var todays = all.filter(function (t) { return t.dueDay === today; });
  // Fall back to "everything not done" if nothing matched dueDay - keeps
  // the watch useful even if the dueDay-based "today" heuristic above turns
  // out to not match the live schema.
  var pool = todays.length > 0 ? todays : all.filter(function (t) { return !t.isDone; });

  pool.sort(function (a, b) {
    if (!!a.isDone !== !!b.isDone) {
      return a.isDone ? 1 : -1;
    }
    return String(a.title).localeCompare(String(b.title));
  });

  return pool.slice(0, limit).map(function (t) {
    return { id: t.id, title: t.title || '(untitled)', isDone: !!t.isDone };
  });
}

module.exports = {
  emptyState: emptyState,
  applyOperation: applyOperation,
  applyOperations: applyOperations,
  getTodayTasks: getTodayTasks,
  todayStr: todayStr,
};
