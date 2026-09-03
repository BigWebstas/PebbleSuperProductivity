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
// A task's dueDay/dueWithTime doesn't only change via TASK-entity ops.
// Scheduling a task through the desktop's Schedule dialog with no specific
// time (including its "Today" quick-access button - dialog-schedule-task.
// component.ts) dispatches PlannerActions.planTaskForDay/transferTask,
// synced under entityType 'PLANNER' (planner.actions.ts), even though the
// real task.reducer.ts's own `on(PlannerActions.planTaskForDay, ...)`
// handler sets task.dueDay directly - see applyPlannerAction below.
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
// day). The real app enforces dueDay/dueWithTime as MUTUALLY EXCLUSIVE -
// setting one clears the other (task-shared-scheduling.reducer.ts) - so a
// todayOnly filter keyed on dueDay alone would miss a dueWithTime-only
// task entirely. No custom "start of day" offset here (unlike the real
// app's startOfNextDayDiffMs) - just the phone's local calendar day.
function msIsToday(ms) {
  return dateToDateStr(new Date(ms)) === todayStr();
}

// state: { task: { [id]: {id, title, isDone, parentId?, projectId?,
//                          tagIds?, __inBacklog?, ...} },
//          project: { [id]: {id, title, ...} },
//          simpleCounter: { [id]: {id, title, isEnabled, type, countOnDay,
//                                   streakMinValue?, isTrackStreaks?, ...} },
//          note: { [id]: {id, projectId, isPinnedToToday, content, created,
//                          modified, ...} },
//          tag: { [id]: {id, title, ...} } }
function emptyState() {
  return { task: {}, project: {}, simpleCounter: {}, note: {}, tag: {} };
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

// Mirrors removeTaskFromParentSideEffects in
// task-shared-crud.reducer.ts (just the subTaskIds splice - the real
// helper's time recalc doesn't affect anything the watch shows): drops
// `childId` from whichever task currently lists it in subTaskIds. Used by
// both convert actions so a re-parented task can't stay referenced by its
// old parent (which would otherwise keep rendering it as a nested row -
// twice, once a convertToMainTask also lists it at top level).
function detachFromParent(tasks, childId) {
  Object.keys(tasks).forEach(function (pid) {
    var sub = tasks[pid].subTaskIds;
    if (sub && sub.indexOf(childId) !== -1) {
      mergeTaskChanges(tasks, pid, { subTaskIds: sub.filter(function (s) { return s !== childId; }) });
    }
  });
}

// Inserts `childId` into `parentId`'s subTaskIds if absent, honoring the
// afterTaskId anchor the convert actions carry (moveItemAfterAnchor in the
// real reducer): null => prepend, a known id => right after it, anything
// else => append.
function attachToParent(tasks, parentId, childId, afterTaskId) {
  var parent = tasks[parentId];
  if (!parent) {
    return;
  }
  var sub = parent.subTaskIds || [];
  if (sub.indexOf(childId) !== -1) {
    return;
  }
  var next;
  if (!afterTaskId) {
    next = [childId].concat(sub);
  } else {
    var at = sub.indexOf(afterTaskId);
    next = at === -1 ? sub.concat([childId]) : sub.slice(0, at + 1).concat([childId], sub.slice(at + 1));
  }
  mergeTaskChanges(tasks, parentId, { subTaskIds: next });
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

    // Mirrors on(addSubTask, ...) in tasks/store/task.reducer.ts. A subtask
    // is NOT created via addTask - it's its own '[Task] Add SubTask' action
    // ({ task, parentId }), dispatched both when a subtask is added by hand
    // and, crucially, once per subTaskTemplate when
    // task-repeat-cfg.service.ts materializes a recurring task's daily
    // instance. Without this case that action fell through to `default` and
    // was dropped entirely: the subtask entity never entered state.task and
    // its id never reached the parent's subTaskIds, so a recurring task with
    // subtasks (or any subtask added since the last full snapshot) showed on
    // the watch as just its bare parent row. The real reducer also forces
    // projectId to the parent's and tagIds to [], and (only for the very
    // first subtask, and only if the parent has none of its own) copies the
    // parent's timeEstimate/timeSpent down - that last part is skipped here
    // since it only nudges the "spent / estimate" subtitle, never whether a
    // row appears.
    case '[Task] Add SubTask': {
      var subTask = actionPayload.task;
      var subParentId = actionPayload.parentId;
      var subParent = subParentId && tasks[subParentId];
      if (subTask && subTask.id && subParent) {
        tasks[subTask.id] = Object.assign({}, subTask, {
          parentId: subParentId,
          projectId: subParent.projectId,
          tagIds: [],
        });
        setInBacklog(tasks, subTask.id, false);
        // Append (real reducer uses [...subTaskIds, task.id]), with the same
        // already-present guard it keeps for replayed/imported ids.
        var subParentIds = subParent.subTaskIds || [];
        if (subParentIds.indexOf(subTask.id) === -1) {
          mergeTaskChanges(tasks, subParentId, { subTaskIds: subParentIds.concat([subTask.id]) });
        }
      }
      break;
    }

    // NOT a full task snapshot - confirmed wrong by reading the actual
    // reducer (handleScheduleTaskWithTime in
    // task-shared-scheduling.reducer.ts): dueWithTime/remindAt are their
    // OWN top-level actionPayload fields, siblings of `task`, and the real
    // reducer only ever reads `task.id` from the task field itself
    // (taskAdapter.updateOne({ id: task.id, changes: { dueWithTime,
    // dueDay: undefined, remindAt } })) - a narrow merge onto the task
    // already in the store, not a replace. Whether actionPayload.task
    // happens to also carry a matching dueWithTime depends entirely on the
    // calling code and isn't guaranteed: task-repeat-cfg.service.ts's
    // recurring-task creation passes `task: taskWithTargetDates`, a
    // snapshot built BEFORE the schedule was computed, so it never has
    // dueWithTime - a previous version of this code did a full replace
    // with that task object and silently dropped the schedule entirely,
    // which is why a recurring task with a time never showed "@ ..." like
    // a normal scheduled task did. isMoveToBacklog mirrors
    // handleScheduleTaskWithTime's own backlog-move side effect.
    case '[Task Shared] scheduleTaskWithTime':
    case '[Task Shared] reScheduleTaskWithTime': {
      var schedId = actionPayload.task && actionPayload.task.id;
      if (schedId) {
        mergeTaskChanges(tasks, schedId, {
          dueWithTime: actionPayload.dueWithTime,
          dueDay: undefined,
          remindAt: actionPayload.remindAt,
        });
        if (actionPayload.isMoveToBacklog) {
          setInBacklog(tasks, schedId, true);
        }
      }
      break;
    }

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

    // Mirrors handleMoveToOtherProject in project-shared.reducer.ts:
    // reassigns projectId on the task and every one of its subtasks (only
    // the parent moves between the two projects' own taskIds lists, but
    // ALL of them get the new projectId - subtasks are never independently
    // listed in a project's taskIds). Previously unhandled entirely, which
    // left a moved task's projectId stale here - showing under its old
    // project, or "No Project" if it never had one - even though the real
    // account has it correctly reassigned. Also clears __inBacklog: the
    // real reducer removes the moved tasks from the old project's
    // backlogTaskIds and never adds them to the new project's, so a move
    // always drops backlog membership regardless of where it started.
    case '[Task Shared] moveToOtherProject': {
      var movedTask = actionPayload.task;
      if (movedTask && movedTask.id && actionPayload.targetProjectId) {
        var movedIds = [movedTask.id].concat(movedTask.subTaskIds || []);
        movedIds.forEach(function (id) {
          mergeTaskChanges(tasks, id, { projectId: actionPayload.targetProjectId });
          setInBacklog(tasks, id, false);
        });
      }
      break;
    }

    // Mirrors handlePlanTasksForToday in task-shared-scheduling.reducer.ts:
    // sets dueDay to the target day and clears remindAt. Kept for its own
    // sake (dueDay is still real task state worth having correct) even
    // though it no longer drives the active-task filter.
    case '[Task Shared] planTasksForToday': {
      // Confirmed against the real handlePlanTasksForToday
      // (task-shared-scheduling.reducer.ts): besides setting dueDay, it
      // ALSO conditionally clears dueWithTime via shouldClearDueTimeForToday
      // (is-today.util.ts) - cleared unless the existing dueWithTime
      // already happens to land on today. Previously this case only ever
      // set dueDay, never touching a leftover dueWithTime - harmless if the
      // task had none, but taskIsPlannedForToday() checks dueWithTime
      // FIRST, so a task "Add to Today"'d while still carrying a stale
      // dueWithTime (any value not already today) stayed hidden from the
      // watch's Today Only filter forever, even though the desktop
      // correctly cleared it and showed the task normally.
      var today = actionPayload.today || todayStr();
      (actionPayload.taskIds || []).forEach(function (id) {
        if (tasks[id]) {
          var changes = { dueDay: today, remindAt: undefined };
          var existingDueWithTime = tasks[id].dueWithTime;
          if (existingDueWithTime && !msIsToday(existingDueWithTime)) {
            changes.dueWithTime = undefined;
          }
          mergeTaskChanges(tasks, id, changes);
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
    // for today. The detachFromParent() call mirrors the reducer's own
    // removeTaskFromParentSideEffects: without it the former parent's
    // subTaskIds still lists this id, so pushTaskAndSubtasks() renders the
    // promoted task a SECOND time as a nested row under its old parent.
    case '[Task Shared] convertToMainTask': {
      var mainTask = actionPayload.task;
      if (mainTask && mainTask.id) {
        detachFromParent(tasks, mainTask.id);
        var mainChanges = { parentId: undefined };
        if (actionPayload.isPlanForToday && !mainTask.dueWithTime) {
          mainChanges.dueDay = actionPayload.today || todayStr();
        }
        mergeTaskChanges(tasks, mainTask.id, mainChanges);
      }
      break;
    }

    // Mirrors handleConvertToSubTask in task-shared-crud.reducer.ts
    // ({ taskId, targetParentId, afterTaskId }): demotes a task to a subtask
    // of targetParentId. Setting only parentId (as this used to) made the
    // task vanish from the watch entirely - isMainTask() now rejects it, but
    // nothing had added it to the target parent's subTaskIds, which is the
    // only place pushTaskAndSubtasks() looks for children. Now also inherits
    // the parent's projectId, clears dueDay, drops backlog membership, and
    // detaches from any previous parent - all per the real reducer.
    case '[Task Shared] convertToSubTask': {
      var cstId = actionPayload.taskId;
      var cstParentId = actionPayload.targetParentId;
      if (cstId && tasks[cstId] && cstParentId && tasks[cstParentId]) {
        detachFromParent(tasks, cstId);
        mergeTaskChanges(tasks, cstId, {
          parentId: cstParentId,
          projectId: tasks[cstParentId].projectId,
          dueDay: undefined,
        });
        setInBacklog(tasks, cstId, false);
        attachToParent(tasks, cstParentId, cstId, actionPayload.afterTaskId);
      }
      break;
    }

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

    // Confirmed against time-tracking.actions.ts/task.reducer.ts: the
    // payload is only { taskId, date, duration } - a DELTA in ms for that
    // calendar day, never the full timeSpentOnDay map - and the real
    // reducer applies it ADDITIVELY (tasks[id].timeSpentOnDay[date] =
    // (existing || 0) + duration) so concurrent contributions from other
    // clients aren't clobbered. timeSpent is just the sum across every day
    // in that map - recomputed here rather than tracked separately so it
    // can never drift from timeSpentOnDay. No-ops on a task we don't know
    // about yet, matching the real reducer's own no-op when the entity
    // isn't loaded (task.reducer.ts).
    case '[TimeTracking] Sync time spent': {
      var ttId = actionPayload.taskId;
      var ttDate = actionPayload.date;
      var ttDuration = actionPayload.duration;
      if (ttId && ttDate && typeof ttDuration === 'number' && tasks[ttId]) {
        var timeSpentOnDay = Object.assign({}, tasks[ttId].timeSpentOnDay);
        timeSpentOnDay[ttDate] = (timeSpentOnDay[ttDate] || 0) + ttDuration;
        var timeSpent = 0;
        Object.keys(timeSpentOnDay).forEach(function (d) { timeSpent += timeSpentOnDay[d]; });
        mergeTaskChanges(tasks, ttId, { timeSpentOnDay: timeSpentOnDay, timeSpent: timeSpent });
      }
      break;
    }

    default:
      // Reminders, Today-tag ordering, tags, deadlines, and other
      // TASK-entity actions don't affect
      // title/isDone/backlog-membership/timeSpent - nothing to do.
      break;
  }
}

// PLANNER-entity ops (planner.actions.ts) - separate from TASK-entity ops
// even though the desktop's own task.reducer.ts reacts to these by setting
// dueDay directly on the task. Confirmed live: a task scheduled via the
// Schedule dialog's plain date picker (or its "Today" quick-access button -
// dialog-schedule-task.component.ts's onQuickAccessClick/_planForDay, taken
// whenever no specific time is set) never appeared on the watch under
// Today Only despite showing "Planned for: Today" on desktop - the op is
// captured with entityType 'PLANNER'/actionType '[Planner] Plan Task for
// Day', which used to fall into applyOperation's generic flat-merge
// fallback (writing into state.planner, never touched by
// taskIsPlannedForToday) instead of updating task.dueDay the way the real
// task.reducer.ts's own `on(PlannerActions.planTaskForDay, ...)` does.
function applyPlannerAction(op, actionPayload, state) {
  var tasks = state.task;
  if (!actionPayload) {
    return;
  }
  switch (op.actionType) {
    // Mirrors task.reducer.ts's on(PlannerActions.planTaskForDay, ...).
    case '[Planner] Plan Task for Day': {
      var pId = actionPayload.task && actionPayload.task.id;
      if (pId) {
        mergeTaskChanges(tasks, pId, {
          dueDay: actionPayload.day,
          dueWithTime: undefined,
          remindAt: undefined,
        });
      }
      break;
    }

    // Mirrors handleTransferTask in planner-shared.reducer.ts (the drag-
    // and-drop reschedule in the Schedule/Planner week view) - same
    // dueDay-setting effect as Plan Task for Day, different trigger.
    case '[Planner] Transfer Task': {
      var trId = actionPayload.task && actionPayload.task.id;
      if (trId) {
        mergeTaskChanges(tasks, trId, {
          dueDay: actionPayload.newDay,
          dueWithTime: undefined,
        });
      }
      break;
    }

    default:
      // Upsert Planner Day/Move In List/Move Before Task only reorder -
      // no dueDay/membership effect to mirror.
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

// The real app has no "project notes" field - a project has a *list* of
// separate Note entities (project.noteIds), entityType 'NOTE'
// (note.actions.ts/note.reducer.ts), riding the same generic op-log capture
// path TASK/PROJECT/SIMPLE_COUNTER do. The watch has no UI for a list of
// notes per project though - it treats a project's oldest Note (by
// `created`, see firstNoteForProject in index.js) as the one synthetic
// "project note" it shows/appends to, same "one note, view + append" shape
// task.notes already has. This just needs to replay the real Note entity
// faithfully; which note the watch picks is index.js's concern, not this
// replay's.
function applyNoteAction(op, actionPayload, state) {
  var notes = ensureCollection(state, 'note');
  if (!actionPayload) {
    return;
  }
  switch (op.actionType) {
    case '[Note] Add Note':
      if (actionPayload.note && actionPayload.note.id) {
        notes[actionPayload.note.id] = actionPayload.note;
      }
      break;

    case '[Note] Update Note':
      if (actionPayload.note && actionPayload.note.id) {
        notes[actionPayload.note.id] =
          Object.assign({}, notes[actionPayload.note.id], actionPayload.note.changes);
      }
      break;

    case '[Note] Delete Note':
      if (actionPayload.id) {
        delete notes[actionPayload.id];
      }
      break;

    case '[Note] Move to other project':
      if (actionPayload.note && actionPayload.note.id && actionPayload.targetProjectId) {
        notes[actionPayload.note.id] =
          Object.assign({}, notes[actionPayload.note.id], { projectId: actionPayload.targetProjectId });
      }
      break;

    default:
      // Update Note Order only reorders (todayOrder, or a project's own
      // note order) - nothing about title/content to mirror.
      break;
  }
}

// Resolves task.tagIds against the TAG entity collection replayed here
// (tag.actions.ts, entityType 'TAG' - same generic op-log capture path
// PROJECT/NOTE already use, no entity-specific meta-reducer). Only `title`
// is needed for the watch's read-only tags overlay - see main.c's
// show_tags_overlay/MSG_TASK_TAGS.
function applyTagAction(op, actionPayload, state) {
  var tags = ensureCollection(state, 'tag');
  if (!actionPayload) {
    return;
  }
  switch (op.actionType) {
    case '[Tag] Add Tag':
      if (actionPayload.tag && actionPayload.tag.id) {
        tags[actionPayload.tag.id] = actionPayload.tag;
      }
      break;

    case '[Tag] Update Tag':
      if (actionPayload.tag && actionPayload.tag.id) {
        tags[actionPayload.tag.id] = Object.assign({}, tags[actionPayload.tag.id], actionPayload.tag.changes);
      }
      break;

    case '[Tag] Delete Tag':
      if (actionPayload.id) {
        delete tags[actionPayload.id];
      }
      break;

    // NOT "...Delete Tags" - mirrors deleteSimpleCounters' own real action
    // string literal, confirmed against the actual createAction() call
    // rather than assumed from the plural naming pattern.
    case '[Tag] Delete multiple Tags':
      (actionPayload.ids || []).forEach(function (id) { delete tags[id]; });
      break;

    default:
      // Reorder and advanced-config actions don't touch title - nothing to
      // mirror.
      break;
  }
}

// "Habits" in the real app's UI are actually the SimpleCounter feature
// (src/app/features/simple-counter/), entityType 'SIMPLE_COUNTER' - there is
// no separate "HABIT" entity type. Confirmed against
// simple-counter.actions.ts/reducer.ts: unlike TASK's bespoke
// actionPayload shapes, every persistent SimpleCounter action rides the
// same generic op-log capture path (no entity-specific meta-reducer), but
// the actionPayload shapes themselves still vary per action type just like
// TASK's do.
function applySimpleCounterAction(op, actionPayload, state) {
  var counters = ensureCollection(state, 'simpleCounter');
  if (!actionPayload) {
    return;
  }
  switch (op.actionType) {
    case '[SimpleCounter] Add SimpleCounter':
      if (actionPayload.simpleCounter && actionPayload.simpleCounter.id) {
        counters[actionPayload.simpleCounter.id] = actionPayload.simpleCounter;
      }
      break;

    case '[SimpleCounter] Update SimpleCounter':
      if (actionPayload.simpleCounter && actionPayload.simpleCounter.id) {
        var scId = actionPayload.simpleCounter.id;
        counters[scId] = Object.assign({}, counters[scId], actionPayload.simpleCounter.changes);
      }
      break;

    // Confirmed against the real reducer (setSimpleCounterCounterToday/
    // ForDate cases): a plain REPLACE of that single day's count
    // (Math.max(0, newVal)), not additive - unlike task time-tracking's
    // delta semantics. This is the "mark a habit done for today" action.
    case '[SimpleCounter] Set SimpleCounter Counter Today':
    case '[SimpleCounter] Set SimpleCounter Counter For Date': {
      var cId = actionPayload.id;
      var day = actionPayload.today || actionPayload.date;
      if (cId && day && typeof actionPayload.newVal === 'number' && counters[cId]) {
        var countOnDay = Object.assign({}, counters[cId].countOnDay);
        countOnDay[day] = Math.max(0, actionPayload.newVal);
        counters[cId] = Object.assign({}, counters[cId], { countOnDay: countOnDay });
      }
      break;
    }

    // StopWatch-type counters' batched time sync - confirmed additive
    // (currentVal + duration), mirroring task time-tracking exactly.
    case '[SimpleCounter] Sync counter time': {
      var stId = actionPayload.id;
      var stDate = actionPayload.date;
      var stDuration = actionPayload.duration;
      if (stId && stDate && typeof stDuration === 'number' && counters[stId]) {
        var stCountOnDay = Object.assign({}, counters[stId].countOnDay);
        stCountOnDay[stDate] = (stCountOnDay[stDate] || 0) + stDuration;
        counters[stId] = Object.assign({}, counters[stId], { countOnDay: stCountOnDay });
      }
      break;
    }

    case '[SimpleCounter] Delete SimpleCounter':
      if (actionPayload.id) {
        delete counters[actionPayload.id];
      }
      break;

    // NOT "...Delete SimpleCounters" - the real action's string literal is
    // "Delete multiple SimpleCounters" (deleteSimpleCounters action
    // creator), confirmed by reading the actual createAction() call rather
    // than assuming the plural naming pattern TASK's deleteTasks uses.
    case '[SimpleCounter] Delete multiple SimpleCounters':
      (actionPayload.ids || []).forEach(function (id) { delete counters[id]; });
      break;

    default:
      // Reorder, upsert (sync/import only), and other SimpleCounter-entity
      // actions don't affect title/isEnabled/type/countOnDay - nothing to do.
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
    if (entityType === 'simple_counter') {
      applySimpleCounterAction(op, payload && payload.actionPayload, state);
      return;
    }
    if (entityType === 'planner') {
      applyPlannerAction(op, payload && payload.actionPayload, state);
      return;
    }
    if (entityType === 'note') {
      applyNoteAction(op, payload && payload.actionPayload, state);
      return;
    }
    if (entityType === 'tag') {
      applyTagAction(op, payload && payload.actionPayload, state);
      return;
    }

    // Everything else (GLOBAL_CONFIG, PLUGIN_USER_DATA, ...) is unused by
    // the watch's task list - kept as a best-effort flat CRUD merge (this
    // project's original, unverified assumption) purely so unrelated
    // entity types don't spam the "unhandled" log.
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
        if (payload && payload.simpleCounter && payload.simpleCounter.entities) {
          state.simpleCounter = payload.simpleCounter.entities;
        }
        if (payload && payload.note && payload.note.entities) {
          state.note = payload.note.entities;
        }
        if (payload && payload.tag && payload.tag.entities) {
          state.tag = payload.tag.entities;
        }
        break;
      default:
        console.log('[task-store] unhandled op type: ' + op.opType);
    }
  } catch (err) {
    console.log('[task-store] failed to apply op ' + (op && op.id) + ': ' + err.message);
  }
}

// onProgress, if given, is called after every entry as (doneCount, total) -
// used by doSync()'s pullPage() to surface decrypt progress on a slow page
// instead of leaving the watch's status frozen (see its own call site).
function applyOperations(entries, state, crypto, onProgress) {
  entries.forEach(function (entry, index) {
    applyOperation(entry, state, crypto);
    if (onProgress) {
      onProgress(index + 1, entries.length);
    }
  });
}

function isMainTask(t) {
  return !t.parentId;
}

function projectTitleFor(state, task) {
  var project = task.projectId && state.project && state.project[task.projectId];
  return (project && project.title) || 'No Project';
}

// Resolves task.tagIds against state.tag, joined for the watch's read-only
// tags overlay (long-select Back on a task row - see main.c's
// show_tags_overlay/MSG_TASK_TAGS). A tag id with no matching entity (not
// yet synced, or deleted) is silently skipped rather than surfacing a
// blank/placeholder name.
function tagTitlesFor(state, task) {
  var ids = task.tagIds || [];
  var tags = state.tag || {};
  var names = [];
  ids.forEach(function (id) {
    if (tags[id] && tags[id].title) {
      names.push(tags[id].title);
    }
  });
  return names.join(', ');
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

// Sort order within one visual group: not-done before done, then by title.
// Module-level (not nested in getActiveTasks) so getProjectTasks can reuse
// the exact same ordering for a project browser's regular / backlog lists.
function withinGroupSort(a, b) {
  if (!!a.isDone !== !!b.isDone) {
    return a.isDone ? 1 : -1;
  }
  return titleCompare(a.title, b.title);
}

// Sentinel project id for the synthetic "No Project" entry getProjectList
// emits when project-less active tasks exist - getProjectTasks maps it back
// to "tasks with no projectId". A real project id is a plain nanoid(), so
// this can't collide.
var NO_PROJECT_ID = '__NO_PROJECT__';

// Every non-archived project, plus a synthetic "No Project" entry when
// there are project-less active main tasks to reach through it. Sorted by
// title. Shape: [{ id, title, color }] where color is 0xRRGGBB parsed from the
// project's theme colour (0 if none). Feeds the watch's Projects browser (a
// pinned row -> project list -> that project's tasks), which - unlike the
// today list - is not date-filtered.
function projectColorRgb(p) {
  // super-productivity Project.theme.primary is a "#rrggbb" string
  // (WorkContextThemeCfg); older data used a flat themeColor. We quantise it
  // to Pebble's packed GColor8 byte here (2 bits per channel + opaque alpha)
  // so the watch just assigns it - no colour maths in the draw path, which
  // matters for emery's tight code budget. 0 = no colour -> no swatch.
  var hex = (p.theme && p.theme.primary) || p.themeColor || '';
  var m = /^#?([0-9a-fA-F]{6})$/.exec(String(hex).trim());
  if (!m) {
    return 0;
  }
  var n = parseInt(m[1], 16);
  var r = (n >> 16) & 0xff, g = (n >> 8) & 0xff, b = n & 0xff;
  return 0xc0 | ((r >> 6) << 4) | ((g >> 6) << 2) | (b >> 6); // GColor8.argb, always non-zero (alpha bits set)
}

function getProjectList(state) {
  var projects = state.project || {};
  var allTasks = state.task || {};
  var out = Object.keys(projects)
    .map(function (id) { return projects[id]; })
    .filter(function (p) { return p && p.id && p.title && !p.isArchived; })
    .map(function (p) { return { id: p.id, title: p.title, color: projectColorRgb(p) }; });
  out.sort(function (a, b) { return titleCompare(a.title, b.title); });
  var hasNoProject = Object.keys(allTasks).some(function (id) {
    var t = allTasks[id];
    return t && t.title && isMainTask(t) && !t.projectId && !t.isDone;
  });
  if (hasNoProject) {
    out.push({ id: NO_PROJECT_ID, title: 'No Project', color: 0 });
  }
  return out;
}

// One project's whole active task list, split into its regular list and its
// backlog - { regular: [rows], backlog: [rows] }, each row the same shape
// getActiveTasks produces (id, title, isDone, project, projectId, tags,
// dueWithTime, timeSpent, timeEstimate) with subtasks nested under their
// parent the same way. projectId === NO_PROJECT_ID (or falsy) selects tasks
// with no project. No date filter - a project browser shows everything, not
// just today. Done tasks still obey hideDone's grace period. Each list is
// capped at `limit` rows.
function getProjectTasks(state, projectId, limit, hideDone) {
  var allTasks = state.task || {};
  var wantNoProject = !projectId || projectId === NO_PROJECT_ID;
  var projName = wantNoProject
    ? 'No Project'
    : ((state.project && state.project[projectId] && state.project[projectId].title) || 'No Project');
  var pid = wantNoProject ? '' : projectId;
  var mains = Object.keys(allTasks)
    .map(function (id) { return allTasks[id]; })
    .filter(function (t) { return t && t.title && isMainTask(t); })
    .filter(function (t) { return !isHiddenDone(t, hideDone); })
    .filter(function (t) {
      return wantNoProject ? !t.projectId : t.projectId === projectId;
    });
  var regularMains = mains
    .filter(function (t) { return !t.__inBacklog; })
    .sort(withinGroupSort);
  var backlogMains = mains
    .filter(function (t) { return t.__inBacklog; })
    .sort(withinGroupSort);
  var regular = [];
  var backlog = [];
  regularMains.forEach(function (t) {
    pushTaskAndSubtasks(regular, state, allTasks, t, projName, pid, 0, hideDone);
  });
  backlogMains.forEach(function (t) {
    pushTaskAndSubtasks(backlog, state, allTasks, t, projName, pid, 0, hideDone);
  });
  return { regular: regular.slice(0, limit), backlog: backlog.slice(0, limit) };
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
// Mirrors computeOrderedTaskIdsForToday in the real app's
// work-context.selectors.ts: dueWithTime takes priority when set (checked
// against today's calendar day) - dueDay is only consulted as a fallback
// when dueWithTime is NOT set. This isn't just an arbitrary tie-break: it's
// how the real selector resolves legacy data that (pre dueDay/dueWithTime
// mutual exclusivity - see task-shared-scheduling.reducer.ts) can carry
// both fields, where a stale leftover dueDay must not override a dueWithTime
// that says otherwise.
function taskIsPlannedForToday(t, today) {
  if (t.dueWithTime) {
    return msIsToday(t.dueWithTime);
  }
  return t.dueDay === today;
}

// A task marked done stays visible for this long after completion even
// with hideDone on, so completing it on the watch doesn't make it vanish
// before the user can see it happen - the very next auto-sync
// (runAutoSyncAfterOp, on by default) used to land within a second or two
// of the toggle and immediately exclude it. Only meaningful for a task
// completed VIA THE WATCH: doneOn is stamped by handleTaskToggle's own
// optimistic update in index.js, which is the only place this replay path
// sets it reliably - a real op from another client (task.service.ts's own
// `update(id, { isDone: true })` call, confirmed via the real source,
// never includes doneOn in the dispatched changes; the real reducer
// computes ITS OWN Date.now() fallback at replay time, which this app's
// generic mergeTaskChanges() doesn't replicate) generally won't carry a
// fresh-enough doneOn through this app's own replay to matter here - which
// is also the right scope: nobody's watching the watch in real time for a
// completion that happened on a different device.
var HIDE_DONE_GRACE_MS = 10000;

// Shared by getActiveTasks' own main-task filter and pushTaskAndSubtasks'
// per-subtask filter below - a done task/subtask with no doneOn at all
// (never set - e.g. done before this grace period existed, or done by
// another client per the comment above) hides immediately, same as this
// app's original behavior, rather than being treated as "just completed".
function isHiddenDone(t, hideDone) {
  if (!hideDone || !t.isDone) {
    return false;
  }
  return !t.doneOn || Date.now() - t.doneOn >= HIDE_DONE_GRACE_MS;
}

// When todayOnly is true, tasks are further restricted to ones actually
// planned for today (see taskIsPlannedForToday) - not undated, overdue, or
// future-dated ones. This mirrors the real app's virtual TODAY_TAG, whose
// membership is likewise derived from dueDay/dueWithTime rather than a
// synced list (boards.util.ts: "TODAY_TAG is virtual: membership derives
// from dueDay/dueWithTime"). A main task with no due date of its own but a
// SUBTASK due today still qualifies - the real selector evaluates every
// task/subtask independently and would otherwise list that subtask as its
// own top-level Today entry; this app always nests subtasks under their
// parent (see pushTaskAndSubtasks), so the parent has to be included for
// the subtask to have somewhere to nest. Doesn't consider
// deadlineDay/deadlineWithTime or explicit tag assignment - not a full
// port, just enough to match what the desktop's Today page actually shows
// for the common case.
// alwaysIncludeId, when given, names one task that must appear in the result
// even if todayOnly / the backlog filter would drop it - the watch's
// currently-tracked task, which may have been started from the Projects
// browser and so be neither planned for today nor in the regular list. The
// watch needs it here to render its pinned "TRACKING" row (main.c's
// pinned_task_index scans exactly this list). A tracked SUBTASK pulls in its
// parent instead, so it still nests (same reason todayOnly pulls in a parent
// for a today-due subtask).
function getActiveTasks(state, limit, groupByProject, todayOnly, hideDone, alwaysIncludeId) {
  var allTasks = state.task || {};
  var today = todayStr();
  var mainTasks = Object.keys(allTasks)
    .map(function (id) { return allTasks[id]; })
    // t.title is the tell for a "ghost" record: mergeTaskChanges()
    // deliberately creates a bare { ...changes } entry (no throw) when an
    // update-style op references a task id this replay has never seen a
    // create/snapshot for - e.g. a stray/out-of-order op, or a real task
    // that was deleted/archived before this account's visible history
    // began. A real task always has a title; nothing about that intentional
    // no-throw behavior was ever meant to make ghosts user-visible, so they
    // never got a title fallback - filtered here instead of leaving them to
    // surface as a literal "(untitled)" row in "No Project" (see
    // pushTaskAndSubtasks, which no longer substitutes placeholder text).
    .filter(function (t) { return t && t.title && isMainTask(t); })
    // Hiding a done MAIN task hides its whole subtask block along with it
    // (pushTaskAndSubtasks is never called for a task that's filtered out
    // here) - same "the subtask has nowhere to nest" reasoning already
    // used for todayOnly above. A done SUBTASK under a still-open parent is
    // handled separately, per-subtask, in pushTaskAndSubtasks - the parent
    // staying visible is exactly the case that reasoning doesn't apply to.
    .filter(function (t) { return !isHiddenDone(t, hideDone); })
    .filter(function (t) {
      if (!todayOnly) {
        // Mirrors project.taskIds vs project.backlogTaskIds
        // (project.model.ts): with no date filter, this is "this project's
        // regular list", which excludes backlog by definition.
        return !t.__inBacklog;
      }
      // The real Today selector (computeOrderedTaskIdsForToday in
      // work-context.selectors.ts) has NO concept of backlog membership at
      // all - it's driven purely by dueDay/dueWithTime. A task can be BOTH
      // still-listed in its project's backlogTaskIds AND explicitly pulled
      // into today: planTasksForToday never touches backlogTaskIds
      // (confirmed against handlePlanTasksForToday in
      // task-shared-scheduling.reducer.ts - it only updates dueDay/
      // remindAt/dueWithTime and the TODAY tag's own taskIds). Excluding it
      // here just because __inBacklog is still (correctly, per the real
      // data model) true would hide a task the real Today page shows -
      // confirmed live: a GitHub-issue task created straight into the
      // backlog, later planned for today, stayed excluded from this list
      // forever even though the desktop's own Today view showed it
      // normally. todayOnly intentionally ignores __inBacklog entirely,
      // matching the real selector's own total independence from it.
      if (taskIsPlannedForToday(t, today)) {
        return true;
      }
      return (t.subTaskIds || []).some(function (subId) {
        var sub = allTasks[subId];
        return sub && taskIsPlannedForToday(sub, today);
      });
    });

  if (alwaysIncludeId && !mainTasks.some(function (t) { return t.id === alwaysIncludeId; })) {
    var forced = allTasks[alwaysIncludeId];
    if (forced && forced.parentId) {
      forced = allTasks[forced.parentId];
    }
    if (forced && forced.title && isMainTask(forced) && !isHiddenDone(forced, hideDone) &&
        !mainTasks.some(function (t) { return t.id === forced.id; })) {
      mainTasks.push(forced);
    }
  }

  var rows = [];
  if (groupByProject) {
    var byProject = {};
    // Grouped by project TITLE (not id) - see projectTitleFor's own "No
    // Project" fallback - so groupProjectIds takes the first task's own
    // projectId seen for that title as the whole visual group's id (used by
    // the watch's project-notes row - see TASK_PROJECT_ID in index.js's
    // sendTaskAt). Two distinct projects sharing a display name would
    // already visually merge into one group before this existed; this just
    // means the merged group's notes button points at whichever of them was
    // seen first, same negligible edge case.
    var groupProjectIds = {};
    var groupColors = {};
    mainTasks.forEach(function (t) {
      var name = projectTitleFor(state, t);
      if (!byProject[name]) {
        byProject[name] = [];
        groupProjectIds[name] = t.projectId || '';
        groupColors[name] = t.projectId ? projectColorRgb((state.project && state.project[t.projectId]) || {}) : 0;
      }
      byProject[name].push(t);
    });
    Object.keys(byProject).sort(titleCompare).forEach(function (name) {
      byProject[name].sort(withinGroupSort);
      byProject[name].forEach(function (t) {
        pushTaskAndSubtasks(rows, state, allTasks, t, name, groupProjectIds[name], groupColors[name], hideDone);
      });
    });
  } else {
    mainTasks.sort(withinGroupSort);
    mainTasks.forEach(function (t) {
      pushTaskAndSubtasks(rows, state, allTasks, t, '', '', 0, hideDone);
    });
  }

  return rows.slice(0, limit);
}

// Pebble's MenuLayer has no per-row indent control, so nesting is baked
// into the title string itself. Plain leading spaces alone read as barely
// different from a regular row at this font size - a leading marker plus
// wider indentation reads unambiguously as "sub-item of the row above".
// U+00BB (RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK, "»") - confirmed
// rendering correctly on this app's system font in the emulator, unlike an
// earlier attempt at U+2514 (BOX DRAWINGS LIGHT UP AND RIGHT, "└"), which
// showed as an empty missing-glyph box on every platform (confirmed twice).
// Not every non-ASCII codepoint fails the way U+2514 did - » (plus ›, ·,
// also tried) rendered fine, it was specifically that one glyph missing
// from the font, not a blanket Unicode limitation. Previously plain ASCII
// (~) for exactly that reason, before this was re-tested more thoroughly.
var SUBTASK_PREFIX = '    » ';

function pushTaskAndSubtasks(rows, state, allTasks, t, groupName, groupProjectId, groupColor, hideDone) {
  // t is already guaranteed a real title here - getActiveTasks filters
  // ghost (title-less) records out of mainTasks before this is ever
  // called (hideDone's own done-main-task filtering happens there too, for
  // the same reason - see its comment). Subtasks aren't filtered upstream
  // (pulled straight from allTasks by id), so a ghost subtask - same
  // "update referenced an id this replay never saw a create for" cause as
  // a ghost main task - is skipped here instead of surfacing as a
  // placeholder-titled row; a done one is skipped here too when hideDone
  // is on, independently of whatever state its (necessarily not-done, or
  // this whole block would never run) parent is in.
  // No `notes` field here - the watch fetches a task's full notes on demand
  // (MSG_NOTE_REQUEST, see index.js's sendFullNotesForTask) only for
  // whichever one task's overlay is currently open, rather than every row
  // carrying a preview whether or not it's ever viewed. projectId likewise
  // rides along on every row (not just once per group) so main.c's
  // recompute_groups() - which derives its per-group TaskGroup from
  // whichever task happens to be group.start - can read it off any task
  // rather than needing a separate carrier. tags, unlike notes, IS sent
  // directly (not fetched on demand) - resolved tag names are short and
  // already fully available locally once TAG entities have replayed, so
  // there's no fetch round-trip worth avoiding the way there is for a
  // task's full notes text.
  rows.push({ id: t.id, title: t.title, isDone: !!t.isDone, project: groupName, projectId: groupProjectId || undefined, projectColor: groupColor || undefined, tags: tagTitlesFor(state, t) || undefined, dueWithTime: t.dueWithTime || undefined, timeSpent: t.timeSpent || undefined, timeEstimate: t.timeEstimate || undefined });
  (t.subTaskIds || []).forEach(function (subId) {
    var sub = allTasks[subId];
    if (sub && sub.title && !isHiddenDone(sub, hideDone)) {
      rows.push({ id: sub.id, title: SUBTASK_PREFIX + sub.title, isDone: !!sub.isDone, project: groupName, projectId: groupProjectId || undefined, projectColor: groupColor || undefined, tags: tagTitlesFor(state, sub) || undefined, dueWithTime: sub.dueWithTime || undefined, timeSpent: sub.timeSpent || undefined, timeEstimate: sub.timeEstimate || undefined });
    }
  });
}

// Returns up to `limit` enabled, manipulable SimpleCounters ("habits" in the
// real app's own UI labeling), each with today's progress. "Done today"
// mirrors the majority of the real UI's own comparisons
// (habit-tracker.component.ts's getProgress/isSimpleCompletion,
// EMPTY_SIMPLE_COUNTER's own default): goal defaults to 1 when
// streakMinValue is unset, done means countOnDay[today] >= goal - this
// holds for StopWatch-type counters too (value/goal are both milliseconds
// there, not a plain count), which the watch shows with a live-ticking
// timer (long-select to start/stop) instead of the Select/long-select
// increment/decrement a plain ClickCounter row uses - see isStopwatch below
// and main.c's habits_menu_select_long_click. A RepeatedCountdownReminder
// counter (isCountdown) gets its own long-select-to-start/stop countdown
// timer too, but its value/goal stay a plain completed-rounds count, same
// units as ClickCounter - confirmed against the real
// simple-counter-button.component.ts: toggleStopwatch() (its click handler,
// shared with StopWatch) only starts/stops the countdown; the count itself
// only advances via countUpAndNextRepeatCountdownSession(), fired when the
// countdown reaches zero, not by any per-tick accumulation the way a
// StopWatch's ms-valued countOnDay works. countdownMs carries
// countdownDuration (the configured length of one round) for exactly that
// timer - 0/absent for every other type. Only isEnabled counters are
// included, matching selectEnabledSimpleCounters. Sorted plain
// alphabetically by title - done/not-done doesn't split the list into two
// blocks, since a habit's position jumping around as soon as it crosses its
// goal for the day makes a specific habit harder to find at a glance than a
// fixed alphabetical spot does.
function getActiveHabits(state, limit) {
  var counters = state.simpleCounter || {};
  var today = todayStr();
  var rows = Object.keys(counters)
    .map(function (id) { return counters[id]; })
    .filter(function (c) { return c && c.id && c.title && c.isEnabled; })
    .map(function (c) {
      var goal = c.streakMinValue || 1;
      var value = (c.countOnDay && c.countOnDay[today]) || 0;
      var isCountdown = c.type === 'RepeatedCountdownReminder';
      return {
        id: c.id,
        title: c.title,
        value: value,
        goal: goal,
        done: value >= goal,
        isStopwatch: c.type === 'StopWatch',
        isCountdown: isCountdown,
        countdownMs: isCountdown ? (c.countdownDuration || 0) : 0,
      };
    });
  rows.sort(function (a, b) { return titleCompare(a.title, b.title); });
  return rows.slice(0, limit);
}

// The watch's Stats page (a pinned row, non-aplite) - the headline numbers
// the desktop's Today panel shows, plus every project's task count.
//
//   estimateRemainingMs - over today's undone tasks, the sum of
//     max(0, timeEstimate - timeSpent). A parent that has subtasks is
//     summed from its own undone subtasks (SP treats the parent estimate
//     as a roll-up of them); a task with no subtasks uses its own figures.
//   workedTodayMs - the sum of timeSpentOnDay[today] across every leaf
//     task and subtask. Roll-up parents (those with subtasks) are skipped
//     so their children's time isn't counted twice.
//   projects - getProjectList order, each with taskCount = its undone,
//     non-backlog main tasks (matching the desktop sidebar's badge).
//
// The desktop's third headline, "time without a break", is local runtime
// state from its TakeABreakService (it resets on idle detection) and never
// enters the SuperSync op log - the watch fills that slot with its own
// current tracking-session length instead, computed on the watch.
function computeStats(state) {
  var tasks = state.task || {};
  var today = todayStr();
  var estimateRemainingMs = 0;
  var workedTodayMs = 0;
  var completedTodayCount = 0;

  Object.keys(tasks).forEach(function (id) {
    var t = tasks[id];
    if (!t || !t.title) {
      return;
    }
    var subIds = (t.subTaskIds || []).filter(function (s) { return tasks[s]; });
    var hasSubs = subIds.length > 0;

    if (!hasSubs) {
      workedTodayMs += (t.timeSpentOnDay && t.timeSpentOnDay[today]) || 0;
    }

    if (!isMainTask(t)) {
      return;
    }
    var plannedToday = taskIsPlannedForToday(t, today) || subIds.some(function (sid) {
      return taskIsPlannedForToday(tasks[sid], today);
    });
    if (!plannedToday) {
      return;
    }
    // For a task with subtasks, count/estimate each subtask (SP treats the
    // parent as a roll-up); otherwise the task itself. "completed today" is
    // the done items among today's list - the desktop's "N of M done" - not
    // doneOn-dated, since doneOn only survives this replay for tasks
    // completed via the watch (see isHiddenDone's own comment).
    if (hasSubs) {
      subIds.forEach(function (sid) {
        var s = tasks[sid];
        if (!s) {
          return;
        }
        if (s.isDone) {
          completedTodayCount++;
        } else {
          estimateRemainingMs += Math.max(0, (s.timeEstimate || 0) - (s.timeSpent || 0));
        }
      });
    } else if (t.isDone) {
      completedTodayCount++;
    } else {
      estimateRemainingMs += Math.max(0, (t.timeEstimate || 0) - (t.timeSpent || 0));
    }
  });

  var projects = getProjectList(state).map(function (p) {
    var wantNoProject = p.id === NO_PROJECT_ID;
    var count = 0;
    Object.keys(tasks).forEach(function (id) {
      var t = tasks[id];
      if (!t || !t.title || !isMainTask(t) || t.isDone || t.__inBacklog) {
        return;
      }
      if (wantNoProject ? !t.projectId : t.projectId === p.id) {
        count++;
      }
    });
    return { id: p.id, title: p.title, taskCount: count };
  });

  return {
    estimateRemainingMs: estimateRemainingMs,
    workedTodayMs: workedTodayMs,
    completedTodayCount: completedTodayCount,
    projects: projects,
  };
}

module.exports = {
  emptyState: emptyState,
  applyOperation: applyOperation,
  applyOperations: applyOperations,
  getActiveTasks: getActiveTasks,
  getActiveHabits: getActiveHabits,
  getProjectList: getProjectList,
  getProjectTasks: getProjectTasks,
  computeStats: computeStats,
  NO_PROJECT_ID: NO_PROJECT_ID,
  todayStr: todayStr,
  dateToDateStr: dateToDateStr,
  HIDE_DONE_GRACE_MS: HIDE_DONE_GRACE_MS,
};
