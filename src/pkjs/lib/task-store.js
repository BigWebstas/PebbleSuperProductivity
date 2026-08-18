// Maintains a local cache of SuperSync entities (rebuilt by replaying
// operations) and derives the watch's task list from it.
//
// See the top-of-file comment in supersync-client.js for the operation
// field-name assumptions this replay logic depends on.
//
// TASK entity semantics, confirmed by decrypting a real account's full op
// history: this is a Redux action-replay log, not a flat CRUD op log.
// `op.opType` (CRT/UPD/...) doesn't tell you how to interpret the payload
// for TASK entities - `op.actionType` does, and each action type has its
// own bespoke `payload.actionPayload` shape, mirroring the app's actual
// NgRx actions (root-store/meta/task-shared.actions.ts,
// features/project/store/project.actions.ts) and their meta-reducers
// (root-store/meta/task-shared-meta-reducers/*) in the super-productivity
// GitHub repo.
//
// getActiveTasks() returns every main task that is NOT sitting in a
// project's backlog - no date filtering. Confirmed against
// project.model.ts: backlog membership lives on the PROJECT entity
// (project.taskIds vs project.backlogTaskIds), not on the task itself, so
// it's tracked here as a synthetic task.__inBacklog flag, seeded from the
// SYNC_IMPORT project snapshot and kept up to date by the handful of
// "[Project] ... Backlog ..." move actions and the isAddToBacklog/
// isMoveToBacklog flags addTask/scheduleTaskWithTime/applyShortSyntax
// carry. (project.actions.ts also has several backlog *reorder* actions -
// moveProjectTask{Up,Down,ToTop,ToBottom,In}BacklogList - which only
// change position within the backlog, never membership, so they're
// deliberately not handled here.)
//
// Subtasks (task.parentId set) are never selected independently - a
// subtask is shown by riding along with its (visible) parent via the
// parent's subTaskIds, indented, regardless of the subtask's own
// isDone/backlog status.
//
// SYNC_IMPORT/BACKUP_IMPORT/REPAIR carry a full NgRx EntityState snapshot
// per feature slice (payload.task = { ids: [...], entities: {...} },
// payload.project likewise), also confirmed against the same real
// account, whose op history starts with exactly this op - without it, a
// task/project created before the visible history begins would only ever
// show whatever fields a later action happened to touch.
'use strict';

function dateToDateStr(d) {
  var mm = ('0' + (d.getMonth() + 1)).slice(-2);
  var dd = ('0' + d.getDate()).slice(-2);
  return d.getFullYear() + '-' + mm + '-' + dd;
}

function todayStr() {
  return dateToDateStr(new Date());
}

// dueWithTime is a ms timestamp (a task scheduled for a specific time of
// day), separate from dueDay (a plain date) - the real app sets both
// together when scheduling from its UI, but they're independent fields, so
// a todayOnly filter keyed on dueDay alone would miss a dueWithTime-only
// task. No custom "start of day" offset here (unlike the real app's
// startOfNextDayDiffMs) - just the phone's local calendar day.
function msIsToday(ms) {
  return dateToDateStr(new Date(ms)) === todayStr();
}

// state: { task: { [id]: {id, title, isDone, parentId?, projectId?,
//                          __inBacklog?, ...} },
//          project: { [id]: {id, title, ...} } }
function emptyState() {
  return { task: {}, project: {} };
}

function ensureCollection(state, entityType) {
  if (!state[entityType]) {
    state[entityType] = {};
  }
  return state[entityType];
}

function setInBacklog(tasks, id, val) {
  if (id && tasks[id]) {
    tasks[id].__inBacklog = val;
  }
}

// Like a plain replace, but carries the synthetic __inBacklog flag
// forward - real Task payloads never include it (we invented it), so a
// naive `tasks[id] = task` would silently drop whatever backlog state
// we'd tracked so far every time one of these full-snapshot actions fires.
function replaceTaskPreservingBacklog(tasks, task) {
  if (!task || !task.id) {
    return;
  }
  var prevInBacklog = tasks[task.id] ? tasks[task.id].__inBacklog : false;
  tasks[task.id] = task;
  tasks[task.id].__inBacklog = !!prevInBacklog;
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
    case '[Task Shared] addTask':
      replaceTaskPreservingBacklog(tasks, actionPayload.task);
      if (actionPayload.task) {
        setInBacklog(tasks, actionPayload.task.id, !!actionPayload.isAddToBacklog);
      }
      break;

    // These carry a full, already-correct task snapshot (the action
    // creator itself bakes in dueDay/dueWithTime before dispatch) - a
    // straight replace matches what the real reducer's entity-adapter
    // update ends up storing. isMoveToBacklog mirrors
    // handleScheduleTaskWithTime's backlog-move side effect.
    case '[Task Shared] scheduleTaskWithTime':
    case '[Task Shared] reScheduleTaskWithTime':
      replaceTaskPreservingBacklog(tasks, actionPayload.task);
      if (actionPayload.isMoveToBacklog && actionPayload.task) {
        setInBacklog(tasks, actionPayload.task.id, true);
      }
      break;

    case '[Task Shared] restoreTask':
    case '[Task Shared] restoreDeletedTask':
      replaceTaskPreservingBacklog(tasks, actionPayload.task);
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
    // sets dueDay to the target day and clears remindAt. Kept for its own
    // sake (dueDay is still real task state worth having correct) even
    // though it no longer drives the active-task filter.
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

    // Archived tasks leave the active view entirely, regardless of
    // backlog/due-date status.
    case '[Task Shared] moveToArchive':
      deleteTasks(tasks, (actionPayload.tasks || []).map(function (t) { return t.id; }));
      break;

    // Mirrors handleApplyShortSyntax in short-syntax-shared.reducer.ts: a
    // task's title can itself carry scheduling ("do the thing today" or
    // "at 3pm") and/or a backlog move, applied atomically alongside plain
    // field changes.
    case '[Task Shared] applyShortSyntax': {
      var scId = actionPayload.task && actionPayload.task.id;
      if (scId) {
        var scChanges = Object.assign({}, actionPayload.taskChanges);
        var info = actionPayload.schedulingInfo;
        if (info && info.dueWithTime) {
          scChanges.dueWithTime = info.dueWithTime;
          scChanges.dueDay = undefined;
        } else if (info && info.day) {
          scChanges.dueDay = info.day;
          scChanges.dueWithTime = undefined;
        }
        mergeTaskChanges(tasks, scId, scChanges);
        if (info && info.isMoveToBacklog) {
          setInBacklog(tasks, scId, true);
        }
      }
      break;
    }

    // Mirrors handleConvertToMainTask in task-shared-crud.reducer.ts:
    // promotes a subtask to a main task (clearing parentId, without which
    // it stays invisible to isMainTask() forever), optionally planning it
    // for today.
    case '[Task Shared] convertToMainTask': {
      var mainTask = actionPayload.task;
      if (mainTask && mainTask.id) {
        var mainChanges = { parentId: undefined };
        if (actionPayload.isPlanForToday && !mainTask.dueWithTime) {
          mainChanges.dueDay = actionPayload.today || todayStr();
        }
        mergeTaskChanges(tasks, mainTask.id, mainChanges);
      }
      break;
    }

    // The inverse of convertToMainTask: demotes a task to a subtask of
    // targetParentId. Kept for symmetry, so a demoted task doesn't
    // continue being (incorrectly) eligible as a main task.
    case '[Task Shared] convertToSubTask':
      mergeTaskChanges(tasks, actionPayload.taskId, { parentId: actionPayload.targetParentId });
      break;

    // The following four, from project.actions.ts's "MOVE TASK ACTIONS"
    // section, change backlog *membership* (as opposed to the several
    // *reorder-within-backlog* actions there, which don't and are
    // deliberately not handled).
    case '[Project] Auto Move Task from regular to backlog':
    case '[Project] Move Task from regular to backlog':
      setInBacklog(tasks, actionPayload.taskId, true);
      break;

    case '[Project] Auto Move Task from backlog to regular':
    case '[Project] Move Task from backlog to regular':
      setInBacklog(tasks, actionPayload.taskId, false);
      break;

    default:
      // Time tracking, reminders, Today-tag ordering, tags, deadlines, and
      // other TASK-entity actions don't affect
      // title/isDone/backlog-membership - nothing to do.
      break;
  }
}

function applyProjectAction(op, actionPayload, state) {
  var projects = ensureCollection(state, 'project');
  if (!actionPayload) {
    return;
  }
  switch (op.actionType) {
    case '[Project] Add Project':
      if (actionPayload.project && actionPayload.project.id) {
        projects[actionPayload.project.id] = actionPayload.project;
      }
      break;

    case '[Project] Update Project':
      if (actionPayload.project && actionPayload.project.id) {
        projects[actionPayload.project.id] =
          Object.assign({}, projects[actionPayload.project.id], actionPayload.project.changes);
      }
      break;

    // No per-task payload here (just a projectId) - clear the flag for
    // every task currently attributed to that project instead.
    case '[Project] Move all backlog tasks to regular': {
      var tasks = state.task;
      Object.keys(tasks).forEach(function (id) {
        if (tasks[id] && tasks[id].projectId === actionPayload.projectId) {
          tasks[id].__inBacklog = false;
        }
      });
      break;
    }

    default:
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
    if (entityType === 'project') {
      applyProjectAction(op, payload && payload.actionPayload, state);
      return;
    }

    // Everything else (GLOBAL_CONFIG, TAG, PLUGIN_USER_DATA, ...) is
    // unused by the watch's task list - kept as a best-effort flat CRUD
    // merge (this project's original, unverified assumption) purely so
    // unrelated entity types don't spam the "unhandled" log.
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
        // payload.task = { ids: [...], entities: { [id]: Task } },
        // payload.project likewise. This is a full replacement, not a
        // merge - it fires once, at whatever point the local history
        // begins.
        if (payload && payload.task && payload.task.entities) {
          state.task = payload.task.entities;
        }
        if (payload && payload.project && payload.project.entities) {
          state.project = payload.project.entities;
          // Seed __inBacklog from each project's backlogTaskIds - this is
          // the only place backlog membership is available as a
          // ready-made list rather than an incremental move action.
          Object.keys(state.project).forEach(function (projectId) {
            var backlogIds = state.project[projectId].backlogTaskIds || [];
            backlogIds.forEach(function (taskId) {
              if (state.task[taskId]) {
                state.task[taskId].__inBacklog = true;
              }
            });
          });
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

function projectTitleFor(state, task) {
  var project = task.projectId && state.project && state.project[task.projectId];
  return (project && project.title) || 'No Project';
}

function titleCompare(a, b) {
  // Plain ordinal comparison, not localeCompare(): confirmed against the
  // basalt emulator that its embedded JS engine throws "Internal error.
  // Icu error." on locale-aware string ops with no ICU data loaded, and
  // locale-aware sorting isn't needed for this anyway.
  var at = String(a);
  var bt = String(b);
  return at < bt ? -1 : at > bt ? 1 : 0;
}

// Returns up to `limit` rows: main tasks that are not sitting in a
// project's backlog (see the top-of-file comment - no date filtering),
// each immediately followed by its own subtasks (indented), regardless of
// the subtask's own isDone/backlog status.
//
// When groupByProject is true, rows are grouped by project title (not-
// done-first, then title, within each group; groups themselves ordered by
// title, "No Project" included as its own group); every row carries a
// `project` field equal to its group's title, so a caller can detect
// group boundaries as runs of equal `project` values. When false, every
// row's `project` is '' - a single implicit group, matching the flat list
// this had before grouping existed.
//
// When todayOnly is true, tasks are further restricted to ones actually
// planned for today: dueDay === today, OR dueWithTime falls on today's
// calendar day (a task scheduled for a specific time can carry dueWithTime
// without dueDay also being set) - not undated, overdue, or future-dated
// ones. This mirrors the real app's virtual TODAY_TAG, whose membership is
// likewise derived from dueDay/dueWithTime rather than a synced list
// (boards.util.ts: "TODAY_TAG is virtual: membership derives from
// dueDay/dueWithTime"). Doesn't consider deadlineDay/deadlineWithTime or
// explicit tag assignment - not a full port, just enough to match what the
// desktop's Today page actually shows for the common case.
function getActiveTasks(state, limit, groupByProject, todayOnly) {
  var allTasks = state.task || {};
  var today = todayStr();
  var mainTasks = Object.keys(allTasks)
    .map(function (id) { return allTasks[id]; })
    .filter(function (t) { return t && isMainTask(t) && !t.__inBacklog; })
    .filter(function (t) {
      return !todayOnly || t.dueDay === today || (t.dueWithTime && msIsToday(t.dueWithTime));
    });

  function withinGroupSort(a, b) {
    if (!!a.isDone !== !!b.isDone) {
      return a.isDone ? 1 : -1;
    }
    return titleCompare(a.title, b.title);
  }

  var rows = [];
  if (groupByProject) {
    var byProject = {};
    mainTasks.forEach(function (t) {
      var name = projectTitleFor(state, t);
      if (!byProject[name]) {
        byProject[name] = [];
      }
      byProject[name].push(t);
    });
    Object.keys(byProject).sort(titleCompare).forEach(function (name) {
      byProject[name].sort(withinGroupSort);
      byProject[name].forEach(function (t) {
        pushTaskAndSubtasks(rows, allTasks, t, name);
      });
    });
  } else {
    mainTasks.sort(withinGroupSort);
    mainTasks.forEach(function (t) {
      pushTaskAndSubtasks(rows, allTasks, t, '');
    });
  }

  return rows.slice(0, limit);
}

// Pebble's MenuLayer has no per-row indent control, so nesting is baked
// into the title string itself. Plain leading spaces alone read as barely
// different from a regular row at this font size - a leading marker plus
// wider indentation reads unambiguously as "sub-item of the row above".
// Plain ASCII (~), not a Unicode glyph (e.g. the box-drawing corner
// U+2514) that isn't guaranteed to exist in Pebble's system fonts on every
// platform this targets - confirmed in the emulator that U+2514 renders as
// an empty missing-glyph box on this app's system font, even on color
// platforms.
var SUBTASK_PREFIX = '    ~ ';

function pushTaskAndSubtasks(rows, allTasks, t, groupName) {
  rows.push({ id: t.id, title: t.title || '(untitled)', isDone: !!t.isDone, project: groupName, dueWithTime: t.dueWithTime || undefined });
  (t.subTaskIds || []).forEach(function (subId) {
    var sub = allTasks[subId];
    if (sub) {
      rows.push({ id: sub.id, title: SUBTASK_PREFIX + (sub.title || '(untitled)'), isDone: !!sub.isDone, project: groupName, dueWithTime: sub.dueWithTime || undefined });
    }
  });
}

module.exports = {
  emptyState: emptyState,
  applyOperation: applyOperation,
  applyOperations: applyOperations,
  getActiveTasks: getActiveTasks,
  todayStr: todayStr,
};
