// Maintains a local cache of SuperSync entities (rebuilt by replaying
// operations) and derives "today's" task list from it.
//
// See the top-of-file comment in supersync-client.js for the operation
// field-name assumptions this replay logic depends on.
//
// TASK entity semantics, confirmed by decrypting a real account's full op
// history: this is a Redux action-replay log, not a flat CRUD op log.
// `op.opType` (CRT/UPD/...) doesn't tell you how to interpret the payload
// for TASK entities - `op.actionType` does, and each action type has its
// own bespoke `payload.actionPayload` shape (mirroring the app's actual
// NgRx actions):
//   "[Task Shared] addTask"            -> actionPayload.task = full task
//   "[Task Shared] scheduleTaskWithTime" -> actionPayload.task = full task
//   "[Task Shared] updateTask"         -> actionPayload.task = {id, changes}
//   "[Task Shared] planTasksForToday"  -> actionPayload = {taskIds, today}
// Across one real account's entire 500-op history, these were the only
// TASK actionTypes that affect title/isDone/today-membership; everything
// else observed (time-tracking syncs, dismissReminderOnly, plugin/tag/
// global-config updates) doesn't affect what the watch shows, and falls
// through to the default no-op case below.
//
// SYNC_IMPORT/BACKUP_IMPORT/REPAIR carry a full NgRx EntityState snapshot
// per feature slice (payload.task = { ids: [...], entities: {...} }) -
// also confirmed against the same real account, whose op history starts
// with exactly this op. Without it, any task created before the visible
// op history begins is invisible except for whatever fields a later
// updateTask's partial `changes` happened to touch.
'use strict';

function todayStr() {
  var d = new Date();
  var mm = ('0' + (d.getMonth() + 1)).slice(-2);
  var dd = ('0' + d.getDate()).slice(-2);
  return d.getFullYear() + '-' + mm + '-' + dd;
}

// dueWithTime is a Unix-epoch-ms timestamp (used by calendar-imported
// tasks); dueDay is a plain "YYYY-MM-DD" string (used elsewhere in the
// app). Only one is normally present. Converts either into "YYYY-MM-DD" in
// local time, or null if neither is set.
function taskDueDay(task) {
  if (task.dueDay) {
    return task.dueDay;
  }
  if (task.dueWithTime) {
    var d = new Date(task.dueWithTime);
    var mm = ('0' + (d.getMonth() + 1)).slice(-2);
    var dd = ('0' + d.getDate()).slice(-2);
    return d.getFullYear() + '-' + mm + '-' + dd;
  }
  return null;
}

// state: { task: { [id]: {id, title, isDone, dueDay|dueWithTime, ...} },
//          plannedToday: { date: 'YYYY-MM-DD', taskIds: [...] } | null, ... }
function emptyState() {
  return { task: {}, plannedToday: null };
}

function ensureCollection(state, entityType) {
  if (!state[entityType]) {
    state[entityType] = {};
  }
  return state[entityType];
}

function applyTaskAction(op, actionPayload, state) {
  var tasks = state.task;
  switch (op.actionType) {
    case '[Task Shared] addTask':
    case '[Task Shared] scheduleTaskWithTime': {
      var task = actionPayload && actionPayload.task;
      if (task && task.id) {
        tasks[task.id] = task;
      }
      break;
    }
    case '[Task Shared] updateTask': {
      var upd = actionPayload && actionPayload.task;
      if (upd && upd.id) {
        tasks[upd.id] = Object.assign({}, tasks[upd.id], upd.changes);
      }
      break;
    }
    case '[Task Shared] planTasksForToday': {
      if (actionPayload && Array.isArray(actionPayload.taskIds) && actionPayload.today) {
        state.plannedToday = { date: actionPayload.today, taskIds: actionPayload.taskIds };
      }
      break;
    }
    default:
      // Time tracking, reminders, and other TASK-entity actions don't
      // affect title/isDone/today-membership - nothing to do.
      break;
  }
}

// Applies one SuperSync operation to `state` in place. `crypto` is the
// object returned by supersync-client.js's createCrypto(password) if E2EE is
// on, or null/undefined otherwise. Never throws - a single malformed/
// unrecognized op should not take down the whole sync (it just means that
// entity may be stale until next snapshot restore).
//
// `entry` is one element of GET /api/sync/ops's `ops` array, confirmed
// against a live account to be shaped { serverSeq, op: {...}, receivedAt } -
// NOT a flat Operation object. entityType is uppercase ("TASK",
// "GLOBAL_CONFIG", ...), the op-type field is `opType` not `type`, and the
// encrypted flag is `isPayloadEncrypted` not `encrypted`.
function applyOperation(entry, state, crypto) {
  var op = entry && entry.op;
  if (!op) {
    return;
  }
  try {
    var payload = op.payload;
    if (op.isPayloadEncrypted && payload && crypto) {
      payload = crypto.decrypt(payload);
    }
    var entityType = op.entityType && String(op.entityType).toLowerCase();

    if (entityType === 'task') {
      applyTaskAction(op, payload && payload.actionPayload, state);
      return;
    }

    // Everything else (GLOBAL_CONFIG, TAG, PLUGIN_USER_DATA, ...) is
    // unused by the watch's "today's tasks" view - kept as a best-effort
    // flat CRUD merge (this project's original, unverified assumption)
    // purely so unrelated entity types don't spam the "unhandled" log.
    switch (op.opType) {
      case 'CRT': {
        var created = ensureCollection(state, entityType);
        created[op.entityId] = payload;
        break;
      }
      case 'UPD': {
        var coll = ensureCollection(state, entityType);
        coll[op.entityId] = Object.assign({}, coll[op.entityId], payload);
        break;
      }
      case 'DEL': {
        var delColl = ensureCollection(state, entityType);
        if (payload && Array.isArray(payload.ids)) {
          payload.ids.forEach(function (id) { delete delColl[id]; });
        } else {
          delete delColl[op.entityId];
        }
        break;
      }
      case 'MOV':
        break;
      case 'SYNC_IMPORT':
      case 'BACKUP_IMPORT':
      case 'REPAIR':
        // Confirmed against a live account: this carries a full NgRx
        // EntityState snapshot per feature slice, e.g.
        // payload.task = { ids: [...], entities: { [id]: Task } }. Only
        // task entities matter for the watch. This is a full replacement,
        // not a merge - it fires once, at whatever point the local
        // history begins, so any task created before that point would
        // otherwise never appear (its only visible mutations are
        // updateTask ops with a partial `changes` object, missing
        // whichever fields were never subsequently touched).
        if (payload && payload.task && payload.task.entities) {
          state.task = payload.task.entities;
        }
        break;
      default:
        console.log('[task-store] unhandled op type: ' + op.opType);
    }
  } catch (err) {
    console.log('[task-store] failed to apply op ' + (op && op.id) + ': ' + err.message);
  }
}

function applyOperations(entries, state, crypto) {
  entries.forEach(function (entry) {
    applyOperation(entry, state, crypto);
  });
}

// Returns up to `limit` tasks relevant to "today", not-done first.
function getTodayTasks(state, limit) {
  var today = todayStr();
  var all = Object.keys(state.task || {}).map(function (id) {
    return state.task[id];
  }).filter(Boolean);

  var todays;
  // Prefer the app's own authoritative "planned for today" list (from a
  // "[Task Shared] planTasksForToday" action) when it's actually for
  // today - it's a direct signal from the real app rather than a guess
  // about which due-date field means "today".
  if (state.plannedToday && state.plannedToday.date === today) {
    todays = state.plannedToday.taskIds
      .map(function (id) { return state.task[id]; })
      .filter(Boolean);
  } else {
    todays = all.filter(function (t) { return taskDueDay(t) === today; });
  }

  // Fall back to "everything not done" if nothing matched - keeps the
  // watch useful even on a fresh account with no planTasksForToday/due-date
  // signal yet.
  var pool = todays.length > 0 ? todays : all.filter(function (t) { return !t.isDone; });

  pool.sort(function (a, b) {
    if (!!a.isDone !== !!b.isDone) {
      return a.isDone ? 1 : -1;
    }
    // Plain ordinal comparison, not localeCompare(): confirmed against the
    // basalt emulator that its embedded JS engine throws "Internal error.
    // Icu error." on locale-aware string ops with no ICU data loaded, and
    // locale-aware sorting isn't needed for this anyway.
    var at = String(a.title);
    var bt = String(b.title);
    return at < bt ? -1 : at > bt ? 1 : 0;
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
