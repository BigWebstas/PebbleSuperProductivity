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
// own bespoke `payload.actionPayload` shape, mirroring the app's actual
// NgRx actions (root-store/meta/task-shared.actions.ts) and their meta-
// reducer (root-store/meta/task-shared-meta-reducers/
// task-shared-scheduling.reducer.ts) in the super-productivity GitHub repo.
//
// What actually determines "is this task in Today", confirmed from
// tag.const.ts's own doc comment plus the scheduling reducer: **task.dueDay
// (or task.dueWithTime falling on today)**, not tag membership. The
// TODAY_TAG.taskIds list the app also maintains is purely display
// ordering, not membership, and isn't tracked here - the watch doesn't
// need to render a user-chosen order among a handful of titles.
// `dueDay`/`dueWithTime` are mutually exclusive (setting one clears the
// other) per the app's own "ARCHITECTURE-DECISIONS.md Decision #1".
//
// getTodayTasks() only returns tasks with dueDay/dueWithTime === today -
// no backlog fallback. An account with genuinely nothing planned for today
// shows an empty list, not a dump of the backlog.
//
// Subtasks (task.parentId set) never carry their own due date in the real
// app - membership in Today is a main-task-only concept, and a subtask is
// shown by being nested under its (visible) parent via parentId's
// task.subTaskIds. getTodayTasks() mirrors that: it selects and sorts main
// tasks only (by today-membership), then interleaves each one's subtasks
// immediately after it, regardless of the subtask's own isDone/due-date.
//
// SYNC_IMPORT/BACKUP_IMPORT/REPAIR carry a full NgRx EntityState snapshot
// per feature slice (payload.task = { ids: [...], entities: {...} }),
// also confirmed against the same real account, whose op history starts
// with exactly this op - without it, a task created before the visible
// history begins would only ever show whatever fields a later action
// happened to touch.
'use strict';

function todayStr() {
  var d = new Date();
  var mm = ('0' + (d.getMonth() + 1)).slice(-2);
  var dd = ('0' + d.getDate()).slice(-2);
  return d.getFullYear() + '-' + mm + '-' + dd;
}

function dateStrFromMs(ms) {
  var d = new Date(ms);
  var mm = ('0' + (d.getMonth() + 1)).slice(-2);
  var dd = ('0' + d.getDate()).slice(-2);
  return d.getFullYear() + '-' + mm + '-' + dd;
}

// dueWithTime is a Unix-epoch-ms timestamp (used by calendar-imported and
// time-of-day-scheduled tasks); dueDay is a plain "YYYY-MM-DD" string. The
// app keeps these mutually exclusive, but this tolerates both being set
// anyway rather than assuming that invariant holds across every op.
function taskDueDay(task) {
  if (task.dueDay) {
    return task.dueDay;
  }
  if (task.dueWithTime) {
    return dateStrFromMs(task.dueWithTime);
  }
  return null;
}

// state: { task: { [id]: {id, title, isDone, dueDay|dueWithTime, ...} } }
function emptyState() {
  return { task: {} };
}

function ensureCollection(state, entityType) {
  if (!state[entityType]) {
    state[entityType] = {};
  }
  return state[entityType];
}

function replaceTask(tasks, task) {
  if (task && task.id) {
    tasks[task.id] = task;
  }
}

function mergeTaskChanges(tasks, id, changes) {
  if (id) {
    tasks[id] = Object.assign({}, tasks[id], changes);
  }
}

function deleteTasks(tasks, ids) {
  (ids || []).forEach(function (id) { delete tasks[id]; });
}

function applyTaskAction(op, actionPayload, state) {
  var tasks = state.task;
  if (!actionPayload) {
    return;
  }
  switch (op.actionType) {
    // These carry a full, already-correct task snapshot (the action
    // creator itself bakes in dueDay/dueWithTime before dispatch) - a
    // straight replace matches what the real reducer's entity-adapter
    // add/update ends up storing.
    case '[Task Shared] addTask':
    case '[Task Shared] scheduleTaskWithTime':
    case '[Task Shared] reScheduleTaskWithTime':
    case '[Task Shared] restoreTask':
    case '[Task Shared] restoreDeletedTask':
      replaceTask(tasks, actionPayload.task);
      break;

    case '[Task Shared] updateTask':
      if (actionPayload.task) {
        mergeTaskChanges(tasks, actionPayload.task.id, actionPayload.task.changes);
      }
      break;

    case '[Task Shared] updateTasks':
      (actionPayload.tasks || []).forEach(function (u) {
        mergeTaskChanges(tasks, u.id, u.changes);
      });
      break;

    // Mirrors handlePlanTasksForToday in task-shared-scheduling.reducer.ts:
    // sets dueDay to the target day and clears remindAt (and, in the real
    // reducer, sometimes dueWithTime - skipped here since it doesn't
    // affect title/isDone/today-membership either way).
    case '[Task Shared] planTasksForToday': {
      var today = actionPayload.today || todayStr();
      (actionPayload.taskIds || []).forEach(function (id) {
        if (tasks[id]) {
          mergeTaskChanges(tasks, id, { dueDay: today, remindAt: undefined });
        }
      });
      break;
    }

    // Mirrors handleUnScheduleTask: clears scheduling, or pins to today if
    // isLeaveInToday.
    case '[Task Shared] unscheduleTask': {
      var day = actionPayload.isLeaveInToday ? (actionPayload.today || todayStr()) : undefined;
      mergeTaskChanges(tasks, actionPayload.id, {
        dueDay: day,
        dueWithTime: undefined,
        remindAt: undefined,
      });
      break;
    }

    case '[Task Shared] deleteTask':
      if (actionPayload.task) {
        deleteTasks(tasks, [actionPayload.task.id]);
      }
      break;

    case '[Task Shared] deleteTasks':
      deleteTasks(tasks, actionPayload.taskIds);
      break;

    // Archived tasks leave the active Today view entirely, regardless of
    // dueDay.
    case '[Task Shared] moveToArchive':
      deleteTasks(tasks, (actionPayload.tasks || []).map(function (t) { return t.id; }));
      break;

    default:
      // Time tracking, reminders, Today-tag *ordering* (as opposed to
      // membership - see the top-of-file comment), tags/projects/deadlines,
      // and other TASK-entity actions don't affect
      // title/isDone/today-membership - nothing to do.
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
        // history begins.
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

function isMainTask(t) {
  return !t.parentId;
}

// Returns up to `limit` rows: main tasks actually due today, each
// immediately followed by its own subtasks (indented), regardless of the
// subtask's own isDone/due-date - subtasks aren't independently filtered,
// they just ride along with their parent.
function getTodayTasks(state, limit) {
  var today = todayStr();
  var allTasks = state.task || {};
  var mainTasks = Object.keys(allTasks)
    .map(function (id) { return allTasks[id]; })
    .filter(function (t) { return t && isMainTask(t); });

  var selected = mainTasks.filter(function (t) { return taskDueDay(t) === today; });

  selected.sort(function (a, b) {
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

  var rows = [];
  selected.forEach(function (t) {
    rows.push({ id: t.id, title: t.title || '(untitled)', isDone: !!t.isDone });
    (t.subTaskIds || []).forEach(function (subId) {
      var sub = allTasks[subId];
      if (sub) {
        rows.push({ id: sub.id, title: '  ' + (sub.title || '(untitled)'), isDone: !!sub.isDone });
      }
    });
  });

  return rows.slice(0, limit);
}

module.exports = {
  emptyState: emptyState,
  applyOperation: applyOperation,
  applyOperations: applyOperations,
  getTodayTasks: getTodayTasks,
  todayStr: todayStr,
};
