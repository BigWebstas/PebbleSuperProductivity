// Sanity checks for the operation-replay / active-task-list logic in
// src/pkjs/lib/task-store.js. Run with: node scripts/test-task-store.js
'use strict';

const assert = require('assert');
const store = require('../src/pkjs/lib/task-store.js');

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

const today = store.todayStr();

// GET /api/sync/ops entries, confirmed against a live account, are shaped
// { serverSeq, op: {...}, receivedAt }, and for TASK/PROJECT entities
// op.payload is a Redux-action envelope { actionPayload: {...} } whose
// shape depends on op.actionType - NOT a flat entity record. entry() below
// builds that real shape for the action types confirmed against
// root-store/meta/task-shared.actions.ts and
// features/project/store/project.actions.ts in the super-productivity repo.
function entry(entityType, actionType, actionPayload) {
  return {
    serverSeq: 1,
    op: { opType: 'UPD', entityType: entityType, actionType: actionType, payload: { actionPayload: actionPayload } },
    receivedAt: 1,
  };
}

function taskEntry(actionType, actionPayload) {
  return entry('TASK', actionType, actionPayload);
}

function addTask(task, extra) {
  return taskEntry('[Task Shared] addTask', Object.assign({ task: task }, extra));
}

function updateTask(id, changes) {
  return taskEntry('[Task Shared] updateTask', { task: { id: id, changes: changes } });
}

function planTasksForToday(date, taskIds) {
  return taskEntry('[Task Shared] planTasksForToday', { today: date, taskIds: taskIds });
}

function plannerEntry(actionType, actionPayload) {
  return entry('PLANNER', actionType, actionPayload);
}

function planTaskForDay(taskId, day) {
  return plannerEntry('[Planner] Plan Task for Day', { task: { id: taskId }, day: day });
}

function transferTask(taskId, prevDay, newDay) {
  return plannerEntry('[Planner] Transfer Task', {
    task: { id: taskId },
    prevDay: prevDay,
    newDay: newDay,
  });
}

function active(state, limit, groupByProject, todayOnly) {
  return store.getActiveTasks(state, limit == null ? 30 : limit, !!groupByProject, !!todayOnly);
}

function counterEntry(actionType, actionPayload) {
  return entry('SIMPLE_COUNTER', actionType, actionPayload);
}

function addCounter(simpleCounter) {
  return counterEntry('[SimpleCounter] Add SimpleCounter', { simpleCounter: simpleCounter });
}

function habits(state, limit) {
  return store.getActiveHabits(state, limit == null ? 30 : limit);
}

check('addTask then updateTask builds a merged task', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      addTask({ id: 't1', title: 'Buy milk', isDone: false }),
      updateTask('t1', { isDone: true }),
    ],
    state
  );
  assert.strictEqual(state.task.t1.title, 'Buy milk');
  assert.strictEqual(state.task.t1.isDone, true);
});

check('a watch task-toggle upload (index.js\'s handleTaskToggle shape) round-trips through replay', () => {
  // Regression test for a real bug: handleTaskToggle in index.js used to
  // upload a flat { isDone } payload with no actionType, which
  // task-store.js's applyTaskAction (keyed entirely on actionType) simply
  // couldn't decode - a watch toggle uploaded fine but was silently inert
  // on replay, by any client including this one's own next sync. This
  // mirrors the exact op shape index.js now constructs and confirmed
  // it's genuinely decodable, not just written to look plausible.
  const state = store.emptyState();
  store.applyOperations([addTask({ id: 't1', title: 'Buy milk', isDone: false })], state);
  const uploadedOp = {
    id: 'op-1',
    op: {
      opType: 'UPD',
      actionType: '[Task Shared] updateTask',
      entityType: 'TASK',
      entityId: 't1',
      payload: { actionPayload: { task: { id: 't1', changes: { isDone: true } } } },
      isPayloadEncrypted: false,
    },
    serverSeq: 2,
    receivedAt: 1,
  };
  store.applyOperations([uploadedOp], state);
  assert.strictEqual(state.task.t1.isDone, true);
});

check('SYNC_IMPORT replaces state.task with the imported EntityState snapshot', () => {
  const state = store.emptyState();
  const importEntry = {
    serverSeq: 1,
    op: {
      opType: 'SYNC_IMPORT',
      entityType: 'ALL',
      payload: {
        task: {
          ids: ['a', 'b'],
          entities: {
            a: { id: 'a', title: 'Pre-existing task', isDone: false },
            b: { id: 'b', title: 'Another pre-existing task', isDone: true },
          },
        },
      },
    },
    receivedAt: 1,
  };
  store.applyOperations([importEntry, updateTask('a', { isDone: true })], state);
  assert.strictEqual(state.task.a.title, 'Pre-existing task');
  assert.strictEqual(state.task.a.isDone, true); // updateTask after the import still applies
  assert.strictEqual(state.task.b.title, 'Another pre-existing task');
});

check('SYNC_IMPORT seeds __inBacklog from each project\'s backlogTaskIds', () => {
  const state = store.emptyState();
  const importEntry = {
    serverSeq: 1,
    op: {
      opType: 'SYNC_IMPORT',
      entityType: 'ALL',
      payload: {
        task: {
          ids: ['a', 'b'],
          entities: {
            a: { id: 'a', title: 'Active task', isDone: false, projectId: 'p1' },
            b: { id: 'b', title: 'Backlogged task', isDone: false, projectId: 'p1' },
          },
        },
        project: {
          ids: ['p1'],
          entities: {
            p1: { id: 'p1', title: 'Work', taskIds: ['a'], backlogTaskIds: ['b'] },
          },
        },
      },
    },
    receivedAt: 1,
  };
  store.applyOperations([importEntry], state);
  const tasks = active(state);
  assert.deepStrictEqual(tasks.map((t) => t.id), ['a']); // b excluded - it's in the backlog
});

check('updateTask on an unknown id creates a bare record rather than throwing', () => {
  const state = store.emptyState();
  assert.doesNotThrow(() => {
    store.applyOperations([updateTask('ghost', { isDone: true })], state);
  });
  assert.strictEqual(state.task.ghost.isDone, true);
});

check('planTasksForToday sets dueDay on existing tasks', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      addTask({ id: 'a', title: 'Zebra', isDone: false, dueDay: '2099-01-01' }),
      addTask({ id: 'b', title: 'Apple', isDone: true }),
      planTasksForToday(today, ['a', 'b']),
    ],
    state
  );
  assert.strictEqual(state.task.a.dueDay, today);
  assert.strictEqual(state.task.b.dueDay, today);
});

check('planTasksForToday clears a stale dueWithTime so it does not shadow the new dueDay', () => {
  // Regression test for a real bug: "Add to Today" on a task that still
  // carried a dueWithTime from an earlier, non-today scheduling (e.g.
  // previously scheduled for a specific time on some other day) set dueDay
  // correctly but left the old dueWithTime in place. taskIsPlannedForToday()
  // checks dueWithTime FIRST, so the task stayed invisible under Today Only
  // forever even though the desktop app - whose real reducer conditionally
  // clears dueWithTime here too (handlePlanTasksForToday /
  // shouldClearDueTimeForToday) - showed it normally.
  const state = store.emptyState();
  const staleTimestamp = new Date('2099-01-01T09:00:00').getTime();
  store.applyOperations(
    [
      addTask({ id: 'a', title: 'Old scheduled task', isDone: false, dueWithTime: staleTimestamp }),
      planTasksForToday(today, ['a']),
    ],
    state
  );
  assert.strictEqual(state.task.a.dueDay, today);
  assert.strictEqual(state.task.a.dueWithTime, undefined);
  assert.deepStrictEqual(active(state, 30, false, true).map((t) => t.id), ['a']);
});

check('planTasksForToday keeps dueWithTime when it already lands on today', () => {
  const state = store.emptyState();
  const todayNoon = new Date();
  todayNoon.setHours(12, 0, 0, 0);
  store.applyOperations(
    [
      addTask({ id: 'a', title: 'Already scheduled for today', isDone: false, dueWithTime: todayNoon.getTime() }),
      planTasksForToday(today, ['a']),
    ],
    state
  );
  assert.strictEqual(state.task.a.dueWithTime, todayNoon.getTime());
});

check('planTasksForToday ignores ids that do not exist yet (matches the real reducer)', () => {
  const state = store.emptyState();
  store.applyOperations([planTasksForToday(today, ['ghost'])], state);
  assert.strictEqual(state.task.ghost, undefined);
});

check('[Planner] Plan Task for Day sets dueDay and clears dueWithTime/remindAt (Schedule dialog\'s date-only picker / its "Today" quick-access button)', () => {
  // Regression test for a real bug: scheduling a task via the desktop's
  // Schedule dialog with no specific time (including its "Today"
  // quick-access button) dispatches PlannerActions.planTaskForDay, synced
  // as an entityType 'PLANNER' op - NOT entityType 'TASK'. This used to
  // fall into applyOperation's generic flat-merge fallback (writing into
  // state.planner, which taskIsPlannedForToday never reads) instead of
  // updating task.dueDay, so a task showing "Planned for: Today" on
  // desktop never appeared on the watch under Today Only.
  const state = store.emptyState();
  store.applyOperations(
    [
      addTask({ id: 'a', title: 'Spray roses with insecticide', isDone: false, dueWithTime: 1234, remindAt: 999 }),
      planTaskForDay('a', today),
    ],
    state
  );
  assert.strictEqual(state.task.a.dueDay, today);
  assert.strictEqual(state.task.a.dueWithTime, undefined);
  assert.strictEqual(state.task.a.remindAt, undefined);
  assert.deepStrictEqual(active(state, 30, false, true).map((t) => t.id), ['a']);
});

check('[Planner] Transfer Task (drag reschedule in the Planner week view) sets dueDay and clears dueWithTime', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      addTask({ id: 'a', title: 'Mess with Claude', isDone: false, dueWithTime: 1234 }),
      transferTask('a', '2099-01-01', today),
    ],
    state
  );
  assert.strictEqual(state.task.a.dueDay, today);
  assert.strictEqual(state.task.a.dueWithTime, undefined);
  assert.deepStrictEqual(active(state, 30, false, true).map((t) => t.id), ['a']);
});

check('[Planner] Plan Task for Day on an unknown id creates a bare record rather than throwing (same ghost pattern as updateTask)', () => {
  const state = store.emptyState();
  assert.doesNotThrow(() => {
    store.applyOperations([planTaskForDay('ghost', today)], state);
  });
  assert.strictEqual(state.task.ghost.dueDay, today);
});

check('unscheduleTask clears dueDay/dueWithTime/remindAt', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      addTask({ id: 'a', title: 'X', dueDay: today, remindAt: 123 }),
      taskEntry('[Task Shared] unscheduleTask', { id: 'a' }),
    ],
    state
  );
  assert.strictEqual(state.task.a.dueDay, undefined);
  assert.strictEqual(state.task.a.remindAt, undefined);
});

check('deleteTask removes the task entirely', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      addTask({ id: 'a', title: 'X' }),
      taskEntry('[Task Shared] deleteTask', { task: { id: 'a' } }),
    ],
    state
  );
  assert.strictEqual(state.task.a, undefined);
});

check('deleteTasks (bulk) removes all listed tasks', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      addTask({ id: 'a', title: 'X' }),
      addTask({ id: 'b', title: 'Y' }),
      taskEntry('[Task Shared] deleteTasks', { taskIds: ['a', 'b'] }),
    ],
    state
  );
  assert.deepStrictEqual(state.task, {});
});

check('moveToArchive removes tasks from the active view', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      addTask({ id: 'a', title: 'X', isDone: true }),
      taskEntry('[Task Shared] moveToArchive', { tasks: [{ id: 'a' }] }),
    ],
    state
  );
  assert.strictEqual(state.task.a, undefined);
});

check('applyShortSyntax applies plain taskChanges and scheduling info together', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      addTask({ id: 'a', title: 'X', isDone: false }),
      taskEntry('[Task Shared] applyShortSyntax', {
        task: { id: 'a' },
        taskChanges: { title: 'X today' },
        schedulingInfo: { day: today },
      }),
    ],
    state
  );
  assert.strictEqual(state.task.a.dueDay, today);
  assert.strictEqual(state.task.a.title, 'X today');
});

check('applyShortSyntax with isMoveToBacklog moves the task out of the active list', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      addTask({ id: 'a', title: 'X' }),
      taskEntry('[Task Shared] applyShortSyntax', {
        task: { id: 'a' },
        taskChanges: {},
        schedulingInfo: { isMoveToBacklog: true },
      }),
    ],
    state
  );
  assert.deepStrictEqual(active(state), []);
});

check('convertToMainTask clears parentId so the task becomes eligible as a main task', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      addTask({ id: 'parent', title: 'Parent', subTaskIds: ['sub'] }),
      addTask({ id: 'sub', title: 'Sub', parentId: 'parent' }),
      taskEntry('[Task Shared] convertToMainTask', { task: { id: 'sub', dueWithTime: undefined }, isPlanForToday: true, today }),
    ],
    state
  );
  assert.strictEqual(state.task.sub.parentId, undefined);
  assert.strictEqual(state.task.sub.dueDay, today);
  const tasks = active(state);
  assert(tasks.some((t) => t.id === 'sub'), 'promoted task should now be selectable as a main task');
});

check('convertToSubTask sets parentId so the task stops being eligible as a main task', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      addTask({ id: 'a', title: 'A' }),
      taskEntry('[Task Shared] convertToSubTask', { taskId: 'a', targetParentId: 'parent' }),
    ],
    state
  );
  assert.strictEqual(state.task.a.parentId, 'parent');
  assert.deepStrictEqual(active(state), []);
});

check('addTask with isAddToBacklog excludes the task from the active list', () => {
  const state = store.emptyState();
  store.applyOperations([addTask({ id: 'a', title: 'X' }, { isAddToBacklog: true })], state);
  assert.strictEqual(state.task.a.__inBacklog, true);
  assert.deepStrictEqual(active(state), []);
});

check('a watch-dictated Add Task upload (index.js\'s handleAddTask shape) round-trips through replay', () => {
  // Regression test for the voice-add-task feature: handleAddTask in
  // index.js builds the actionPayload as { task, workContextId,
  // workContextType, isAddToBacklog, isAddToBottom, isIgnoreShortSyntax } -
  // this mirrors that exact shape (not just { task: ... } like the addTask()
  // helper above) to confirm it's genuinely decodable by the real replay
  // path, not just written to look plausible.
  const state = store.emptyState();
  const task = {
    id: 'task-abc123',
    subTaskIds: [],
    timeSpentOnDay: {},
    timeSpent: 0,
    timeEstimate: 0,
    isDone: false,
    title: 'Mix insecticide for fruit trees',
    tagIds: [],
    created: Date.now(),
    attachments: [],
    projectId: 'INBOX_PROJECT',
  };
  store.applyOperations(
    [
      taskEntry('[Task Shared] addTask', {
        task: task,
        workContextId: 'INBOX_PROJECT',
        workContextType: 'PROJECT',
        isAddToBacklog: false,
        isAddToBottom: false,
        isIgnoreShortSyntax: true,
      }),
    ],
    state
  );
  assert.strictEqual(state.task['task-abc123'].title, 'Mix insecticide for fruit trees');
  assert.strictEqual(state.task['task-abc123'].projectId, 'INBOX_PROJECT');
  assert.strictEqual(state.task['task-abc123'].__inBacklog, false);
  assert.deepStrictEqual(active(state).map((t) => t.id), ['task-abc123']);
});

check('"[Project] Move Task from regular to backlog" removes the task from the active list', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      addTask({ id: 'a', title: 'X' }),
      taskEntry('[Project] Move Task from regular to backlog', { taskId: 'a', afterTaskId: null, workContextId: 'p1' }),
    ],
    state
  );
  assert.deepStrictEqual(active(state), []);
});

check('"[Project] Move Task from backlog to regular" restores the task to the active list', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      addTask({ id: 'a', title: 'X' }, { isAddToBacklog: true }),
      taskEntry('[Project] Move Task from backlog to regular', { taskId: 'a', afterTaskId: null, workContextId: 'p1' }),
    ],
    state
  );
  assert.deepStrictEqual(active(state).map((t) => t.id), ['a']);
});

check('reordering within the backlog (moveProjectTask...InBacklogList) does not change membership', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      addTask({ id: 'a', title: 'X' }, { isAddToBacklog: true }),
      taskEntry('[Project] Move Task Up in Backlog', { taskId: 'a', workContextId: 'p1', doneBacklogTaskIds: [] }),
    ],
    state
  );
  // Still in the backlog - this action only reorders, it's not one of the
  // two membership-changing actions.
  assert.deepStrictEqual(active(state), []);
});

check('"[Project] Move all backlog tasks to regular" clears the flag for every task in that project', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      addTask({ id: 'a', title: 'X', projectId: 'p1' }, { isAddToBacklog: true }),
      addTask({ id: 'b', title: 'Y', projectId: 'p1' }, { isAddToBacklog: true }),
      addTask({ id: 'c', title: 'Z', projectId: 'p2' }, { isAddToBacklog: true }),
      entry('PROJECT', '[Project] Move all backlog tasks to regular', { projectId: 'p1' }),
    ],
    state
  );
  const ids = active(state).map((t) => t.id).sort();
  assert.deepStrictEqual(ids, ['a', 'b']); // c is a different project, stays in backlog
});

check('a full-replace action (addTask/restoreTask/restoreDeletedTask) preserves a previously-set __inBacklog flag', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      addTask({ id: 'a', title: 'X' }, { isAddToBacklog: true }),
      // restoreTask replaces the whole task record - the real payload
      // never mentions __inBacklog since we invented it, so a naive
      // replace would silently un-backlog this task.
      taskEntry('[Task Shared] restoreTask', { task: { id: 'a', title: 'X' } }),
    ],
    state
  );
  assert.strictEqual(state.task.a.__inBacklog, true);
});

check('scheduleTaskWithTime sets dueWithTime from its own top-level field, not actionPayload.task', () => {
  // Reproduces the real task-repeat-cfg.service.ts call shape for a
  // recurring task with a start time: `task` is a pre-schedule snapshot
  // with no dueWithTime of its own (createRepeatableTask dispatches
  // addTask, then a SEPARATE scheduleTaskWithTime whose `task` field is
  // the same pre-schedule object) - dueWithTime/remindAt are sibling
  // fields on the action payload, and the real reducer
  // (handleScheduleTaskWithTime) only ever reads `task.id`, never
  // `task.dueWithTime`. A previous version of this replay read
  // actionPayload.task.dueWithTime instead and silently dropped the
  // schedule whenever a caller didn't happen to pre-merge it in - which
  // is exactly why a recurring task with a time never showed "@ ..." like
  // a normal scheduled task did.
  const state = store.emptyState();
  const at9am = Date.now();
  store.applyOperations(
    [
      addTask({ id: 'a', title: 'Daily standup', dueDay: today }),
      taskEntry('[Task Shared] scheduleTaskWithTime', {
        task: { id: 'a' }, // no dueWithTime here - matches the real recurring-task call site
        dueWithTime: at9am,
        remindAt: at9am,
        isMoveToBacklog: false,
      }),
    ],
    state
  );
  assert.strictEqual(state.task.a.dueWithTime, at9am);
  assert.strictEqual(state.task.a.dueDay, undefined); // dueDay/dueWithTime mutual exclusivity
  assert.strictEqual(state.task.a.title, 'Daily standup'); // untouched - a merge, not a replace
});

check('scheduleTaskWithTime with isMoveToBacklog moves the task out of the active list', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      addTask({ id: 'a', title: 'X' }),
      taskEntry('[Task Shared] scheduleTaskWithTime', {
        task: { id: 'a' },
        dueWithTime: Date.now(),
        isMoveToBacklog: true,
      }),
    ],
    state
  );
  assert.strictEqual(state.task.a.__inBacklog, true);
});

check('"[Project] Add Project" then "[Project] Update Project" track project titles', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      entry('PROJECT', '[Project] Add Project', { project: { id: 'p1', title: 'Groceries' } }),
      entry('PROJECT', '[Project] Update Project', { project: { id: 'p1', changes: { title: 'Shopping' } } }),
    ],
    state
  );
  assert.strictEqual(state.project.p1.title, 'Shopping');
});

check('unrecognized TASK actionType is ignored, not thrown', () => {
  const state = store.emptyState();
  assert.doesNotThrow(() => {
    store.applyOperations([taskEntry('[Task Shared] dismissReminderOnly', { id: 't1' })], state);
  });
  assert.deepStrictEqual(state.task, {});
});

check('[TimeTracking] Sync time spent applies additively to timeSpentOnDay, recomputes timeSpent', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      addTask({ id: 'a', title: 'X', isDone: false }),
      taskEntry('[TimeTracking] Sync time spent', { taskId: 'a', date: today, duration: 60000 }),
      // A second delta for the same day, e.g. a later 5-minute flush -
      // must ADD, not replace (this is how the real reducer merges
      // concurrent contributions from other clients too).
      taskEntry('[TimeTracking] Sync time spent', { taskId: 'a', date: today, duration: 30000 }),
      taskEntry('[TimeTracking] Sync time spent', { taskId: 'a', date: '2020-01-01', duration: 5000 }),
    ],
    state
  );
  assert.strictEqual(state.task.a.timeSpentOnDay[today], 90000);
  assert.strictEqual(state.task.a.timeSpentOnDay['2020-01-01'], 5000);
  assert.strictEqual(state.task.a.timeSpent, 95000); // sum across every day
});

check('[TimeTracking] Sync time spent on an unknown task is a no-op', () => {
  const state = store.emptyState();
  assert.doesNotThrow(() => {
    store.applyOperations([taskEntry('[TimeTracking] Sync time spent', { taskId: 'ghost', date: today, duration: 100 })], state);
  });
  assert.deepStrictEqual(state.task, {});
});

check('malformed op does not throw', () => {
  const state = store.emptyState();
  assert.doesNotThrow(() => {
    store.applyOperations([{ serverSeq: 1, receivedAt: 1 }], state); // missing op entirely
    store.applyOperations([{ serverSeq: 1, op: { opType: 'UPD', entityType: 'TASK', actionType: '[Task Shared] updateTask', payload: null }, receivedAt: 1 }], state);
  });
});

check('getActiveTasks excludes backlog tasks but includes everything else, any due date', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      addTask({ id: 'a', title: 'Zebra', isDone: false, dueDay: today }),
      addTask({ id: 'b', title: 'Apple', isDone: true }), // no due date at all - still included
      addTask({ id: 'c', title: 'Someday', isDone: false, dueDay: '2099-01-01' }), // due far in the future - still included
      addTask({ id: 'd', title: 'Backlogged', isDone: false }, { isAddToBacklog: true }), // excluded
    ],
    state
  );
  const tasks = active(state);
  // not-done first (sorted by title: "Someday" < "Zebra"), then done ('b')
  assert.deepStrictEqual(tasks.map((t) => t.id), ['c', 'a', 'b']);
});

check('getActiveTasks never shows a "ghost" task created by an update op for an unseen id', () => {
  // mergeTaskChanges() deliberately creates a bare, title-less record
  // rather than throwing when an update-style op references a task id
  // this replay never saw a create/snapshot for (see "updateTask on an
  // unknown id creates a bare record rather than throwing" above) - real
  // symptom this caused: a task showing up on the watch as literally
  // "(untitled)" in "No Project". getActiveTasks now filters on t.title
  // as the tell for "this is a ghost, not a real task" instead of
  // surfacing it with placeholder text.
  const state = store.emptyState();
  store.applyOperations(
    [
      addTask({ id: 'real', title: 'Real task', isDone: false }),
      updateTask('ghost', { isDone: true }),
    ],
    state
  );
  assert.strictEqual(state.task.ghost.isDone, true); // still tolerated internally, not thrown
  const tasks = active(state);
  assert.deepStrictEqual(tasks.map((t) => t.id), ['real']);
});

check('getActiveTasks skips a ghost subtask referenced by a real parent\'s subTaskIds', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      addTask({ id: 'main', title: 'Plan trip', isDone: false, subTaskIds: ['sub1', 'ghost-sub'] }),
      addTask({ id: 'sub1', title: 'Book flights', isDone: false, parentId: 'main' }),
      // 'ghost-sub' is listed in main's subTaskIds but its own create op
      // was never seen - e.g. an update for it arrived without one.
      updateTask('ghost-sub', { isDone: true }),
    ],
    state
  );
  const tasks = active(state);
  assert.deepStrictEqual(tasks.map((t) => t.id), ['main', 'sub1']);
});

check('getActiveTasks(todayOnly=true) keeps only tasks due exactly today, drops undated, overdue, and future', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      addTask({ id: 'a', title: 'Due today', isDone: false, dueDay: today }),
      addTask({ id: 'b', title: 'Overdue', isDone: false, dueDay: '2000-01-01' }),
      addTask({ id: 'c', title: 'No due date', isDone: false }),
      addTask({ id: 'd', title: 'Future', isDone: false, dueDay: '2099-01-01' }),
    ],
    state
  );
  const tasks = active(state, null, false, true);
  assert.deepStrictEqual(tasks.map((t) => t.id), ['a']);
});

check('getActiveTasks(todayOnly=true) includes a task created into the backlog but later planned for today', () => {
  // Confirmed against the real handlePlanTasksForToday reducer
  // (task-shared-scheduling.reducer.ts): planning a task for today never
  // touches project.backlogTaskIds, so a task can be BOTH still-listed in
  // the backlog AND due today - the real Today selector
  // (computeOrderedTaskIdsForToday) has no concept of backlog at all, so
  // todayOnly must not exclude it just because __inBacklog is (correctly)
  // still true. Reproduces a real account's data exactly: a GitHub-issue
  // task added straight into the backlog, later pulled into today.
  const state = store.emptyState();
  store.applyOperations(
    [
      addTask({ id: 'a', title: 'Backlog task planned for today', isDone: false }, { isAddToBacklog: true }),
      planTasksForToday(today, ['a']),
    ],
    state
  );
  assert.strictEqual(state.task.a.__inBacklog, true);
  const todayTasks = active(state, null, false, true);
  assert.deepStrictEqual(todayTasks.map((t) => t.id), ['a']);
  // The non-today-filtered view is a different, project-scoped concept
  // (project.taskIds vs backlogTaskIds) - it should still exclude it.
  const allActiveTasks = active(state, null, false, false);
  assert.deepStrictEqual(allActiveTasks.map((t) => t.id), []);
});

check('getActiveTasks(todayOnly=true) also keeps a dueWithTime-only task scheduled for today', () => {
  const state = store.emptyState();
  const now = new Date();
  const todayAt9am = new Date(now.getFullYear(), now.getMonth(), now.getDate(), 9, 0, 0).getTime();
  const tomorrowAt9am = todayAt9am + 24 * 60 * 60 * 1000;
  store.applyOperations(
    [
      // No dueDay at all - only a specific-time schedule, still "today".
      addTask({ id: 'a', title: 'Scheduled today', isDone: false, dueWithTime: todayAt9am }),
      addTask({ id: 'b', title: 'Scheduled tomorrow', isDone: false, dueWithTime: tomorrowAt9am }),
      addTask({ id: 'c', title: 'No schedule at all', isDone: false }),
    ],
    state
  );
  const tasks = active(state, null, false, true);
  assert.deepStrictEqual(tasks.map((t) => t.id), ['a']);
});

check('getActiveTasks(todayOnly=true) includes a main task whose SUBTASK (not the parent) is due today', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      // Parent has no due date of its own - only its subtask does.
      addTask({ id: 'main', title: 'Undated parent', isDone: false, subTaskIds: ['sub1', 'sub2'] }),
      addTask({ id: 'sub1', title: 'Due today subtask', isDone: false, dueDay: today, parentId: 'main' }),
      addTask({ id: 'sub2', title: 'Undated subtask', isDone: false, parentId: 'main' }),
      addTask({ id: 'other', title: 'Unrelated undated task', isDone: false }),
    ],
    state
  );
  const tasks = active(state, null, false, true);
  assert.deepStrictEqual(tasks.map((t) => t.id), ['main', 'sub1', 'sub2']);
});

check('getActiveTasks(todayOnly=true) prefers dueWithTime over a stale dueDay on legacy dual-field data', () => {
  const state = store.emptyState();
  const now = new Date();
  const tomorrowAt9am = new Date(now.getFullYear(), now.getMonth(), now.getDate() + 1, 9, 0, 0).getTime();
  store.applyOperations(
    [
      // Legacy data: dueDay says today, but dueWithTime (which takes
      // priority once set) says tomorrow - the real app's selector would
      // exclude this from today, not include it via the dueDay fallback.
      addTask({ id: 'a', title: 'Stale dueDay, real time is tomorrow', isDone: false, dueDay: today, dueWithTime: tomorrowAt9am }),
    ],
    state
  );
  const tasks = active(state, null, false, true);
  assert.deepStrictEqual(tasks.map((t) => t.id), []);
});

check('getActiveTasks(todayOnly=true) includes a recurring task\'s materialized today instance', () => {
  // Recurring tasks have no synced representation of their own on "today" -
  // the desktop/mobile app's TaskRepeatCfgService materializes each day's
  // occurrence as an ordinary addTask op with dueDay baked in
  // (task-repeat-cfg.service.ts's _getTaskRepeatTemplate/
  // _getActionsForTaskRepeatCfg: dispatches TaskSharedActions.addTask with
  // isAddToBacklog: false and the task's `additional.dueDay` set to the
  // target day), carrying a repeatCfgId field the real Task model has but
  // this replay never reads. So once another client has actually created
  // today's instance, it's structurally indistinguishable here from any
  // other task with dueDay === today - no repeat-specific handling needed,
  // just confirming the ordinary addTask + dueDay path already covers it.
  const state = store.emptyState();
  store.applyOperations(
    [
      addTask({
        id: 'repeat-cfg-1_2026-08-18',
        title: 'Daily standup',
        isDone: false,
        dueDay: today,
        repeatCfgId: 'repeat-cfg-1',
        created: Date.now(),
      }, { isAddToBacklog: false, isAddToBottom: true }),
    ],
    state
  );
  const tasks = active(state, null, false, true);
  assert.deepStrictEqual(tasks.map((t) => t.id), ['repeat-cfg-1_2026-08-18']);
});

check('getActiveTasks nests subtasks under their main task, indented', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      addTask({ id: 'main', title: 'Plan trip', isDone: false, subTaskIds: ['sub1', 'sub2'] }),
      addTask({ id: 'sub1', title: 'Book flights', isDone: false, parentId: 'main' }),
      addTask({ id: 'sub2', title: 'Book hotel', isDone: true, parentId: 'main' }),
    ],
    state
  );
  const tasks = active(state);
  assert.deepStrictEqual(tasks.map((t) => t.id), ['main', 'sub1', 'sub2']);
  assert.strictEqual(tasks[1].title, '    ~ Book flights');
  assert.strictEqual(tasks[2].title, '    ~ Book hotel');
});

check('getActiveTasks never lists a subtask as a top-level row on its own', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      addTask({ id: 'main', title: 'Parent', isDone: false }, { isAddToBacklog: true }), // parent excluded (backlog)
      addTask({ id: 'sub1', title: 'Orphan-ish subtask', isDone: false, parentId: 'main' }),
    ],
    state
  );
  assert.deepStrictEqual(active(state), []);
});

check('getActiveTasks respects limit', () => {
  const state = store.emptyState();
  const ops = [];
  for (let i = 0; i < 50; i++) {
    ops.push(addTask({ id: 't' + i, title: 'Task ' + i, isDone: false }));
  }
  store.applyOperations(ops, state);
  assert.strictEqual(active(state, 30).length, 30);
});

check('getActiveTasks(groupByProject=false) tags every row with an empty project - a single implicit group', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      entry('PROJECT', '[Project] Add Project', { project: { id: 'p1', title: 'Work' } }),
      addTask({ id: 'a', title: 'X', projectId: 'p1' }),
      addTask({ id: 'b', title: 'Y' }),
    ],
    state
  );
  const tasks = active(state, 30, false);
  assert(tasks.every((t) => t.project === ''), 'every row should have an empty project field when grouping is off');
});

check('getActiveTasks(groupByProject=true) groups by project title, sorted, with a "No Project" bucket', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      entry('PROJECT', '[Project] Add Project', { project: { id: 'p1', title: 'Work' } }),
      entry('PROJECT', '[Project] Add Project', { project: { id: 'p2', title: 'Groceries' } }),
      addTask({ id: 'w1', title: 'Finish report', isDone: false, projectId: 'p1' }),
      addTask({ id: 'g1', title: 'Buy milk', isDone: false, projectId: 'p2' }),
      addTask({ id: 'n1', title: 'Random task', isDone: false }),
    ],
    state
  );
  const tasks = active(state, 30, true);
  // Groups sorted by title: "Groceries" < "No Project" < "Work"
  assert.deepStrictEqual(tasks.map((t) => t.id), ['g1', 'n1', 'w1']);
  assert.deepStrictEqual(tasks.map((t) => t.project), ['Groceries', 'No Project', 'Work']);
});

check('getActiveTasks(groupByProject=true) keeps a subtask in its parent\'s group', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      entry('PROJECT', '[Project] Add Project', { project: { id: 'p1', title: 'Work' } }),
      addTask({ id: 'main', title: 'Plan launch', isDone: false, projectId: 'p1', subTaskIds: ['sub1'] }),
      addTask({ id: 'sub1', title: 'Write docs', isDone: false, parentId: 'main' }),
    ],
    state
  );
  const tasks = active(state, 30, true);
  assert.deepStrictEqual(tasks.map((t) => t.project), ['Work', 'Work']);
});

check('moveToOtherProject reassigns projectId on the task and its subtasks, clears backlog', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      entry('PROJECT', '[Project] Add Project', { project: { id: 'p1', title: 'Personal' } }),
      entry('PROJECT', '[Project] Add Project', { project: { id: 'p2', title: 'Work' } }),
      addTask({ id: 'main', title: 'Ship feature', isDone: false, projectId: 'p1', subTaskIds: ['sub1'] }, { isAddToBacklog: true }),
      addTask({ id: 'sub1', title: 'Write tests', isDone: false, parentId: 'main', projectId: 'p1' }),
      taskEntry('[Task Shared] moveToOtherProject', {
        task: { id: 'main', subTaskIds: ['sub1'] },
        targetProjectId: 'p2',
      }),
    ],
    state
  );
  assert.strictEqual(state.task.main.projectId, 'p2');
  assert.strictEqual(state.task.sub1.projectId, 'p2');
  // Moving out of the backlog project also clears backlog membership -
  // the task should now be active, not still hidden.
  const tasks = active(state, 30, true);
  assert.deepStrictEqual(tasks.map((t) => t.id), ['main', 'sub1']);
  assert.deepStrictEqual(tasks.map((t) => t.project), ['Work', 'Work']);
});

check('moveToOtherProject on an undated, no-project task fixes it away from "No Project"', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      entry('PROJECT', '[Project] Add Project', { project: { id: 'p1', title: 'Errands' } }),
      addTask({ id: 'a', title: 'Buy milk', isDone: false }),
      taskEntry('[Task Shared] moveToOtherProject', { task: { id: 'a' }, targetProjectId: 'p1' }),
    ],
    state
  );
  const tasks = active(state, 30, true);
  assert.deepStrictEqual(tasks.map((t) => t.project), ['Errands']);
});

check('getActiveHabits includes an enabled click-counter, not done below goal', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      addCounter({ id: 'h1', title: 'Drink water', isEnabled: true, type: 'ClickCounter', streakMinValue: 3, countOnDay: { [today]: 1 } }),
    ],
    state
  );
  const rows = habits(state);
  assert.deepStrictEqual(rows, [{ id: 'h1', title: 'Drink water', value: 1, goal: 3, done: false }]);
});

check('getActiveHabits marks done once today\'s count reaches goal', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      addCounter({ id: 'h1', title: 'Read', isEnabled: true, type: 'ClickCounter', streakMinValue: 1, countOnDay: { [today]: 1 } }),
    ],
    state
  );
  assert.strictEqual(habits(state)[0].done, true);
});

check('getActiveHabits defaults goal to 1 when streakMinValue is unset', () => {
  const state = store.emptyState();
  store.applyOperations(
    [addCounter({ id: 'h1', title: 'Meditate', isEnabled: true, type: 'ClickCounter', countOnDay: {} })],
    state
  );
  const row = habits(state)[0];
  assert.strictEqual(row.goal, 1);
  assert.strictEqual(row.done, false);
});

check('getActiveHabits excludes a disabled counter', () => {
  const state = store.emptyState();
  store.applyOperations(
    [addCounter({ id: 'h1', title: 'Old habit', isEnabled: false, type: 'ClickCounter', countOnDay: {} })],
    state
  );
  assert.deepStrictEqual(habits(state), []);
});

check('getActiveHabits excludes a StopWatch-type counter entirely (not manipulable from the watch)', () => {
  const state = store.emptyState();
  store.applyOperations(
    [addCounter({ id: 'h1', title: 'Deep work', isEnabled: true, type: 'StopWatch', streakMinValue: 1, countOnDay: { [today]: 5000 } })],
    state
  );
  assert.deepStrictEqual(habits(state), []);
});

check('[SimpleCounter] Set SimpleCounter Counter Today replaces (not adds to) today\'s count', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      addCounter({ id: 'h1', title: 'Pushups', isEnabled: true, type: 'ClickCounter', streakMinValue: 1, countOnDay: { [today]: 5 } }),
      counterEntry('[SimpleCounter] Set SimpleCounter Counter Today', { id: 'h1', newVal: 1, today: today }),
    ],
    state
  );
  assert.strictEqual(state.simpleCounter.h1.countOnDay[today], 1);
});

check('[SimpleCounter] Sync counter time applies additively to countOnDay', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      addCounter({ id: 'h1', title: 'Focus', isEnabled: true, type: 'StopWatch', countOnDay: { [today]: 1000 } }),
      counterEntry('[SimpleCounter] Sync counter time', { id: 'h1', date: today, duration: 500 }),
    ],
    state
  );
  assert.strictEqual(state.simpleCounter.h1.countOnDay[today], 1500);
});

check('[SimpleCounter] Delete multiple SimpleCounters removes the counters', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      addCounter({ id: 'h1', title: 'A', isEnabled: true, type: 'ClickCounter', countOnDay: {} }),
      addCounter({ id: 'h2', title: 'B', isEnabled: true, type: 'ClickCounter', countOnDay: {} }),
      counterEntry('[SimpleCounter] Delete multiple SimpleCounters', { ids: ['h1', 'h2'] }),
    ],
    state
  );
  assert.deepStrictEqual(habits(state), []);
});

console.log('');
if (failures > 0) {
  console.log(`${failures} check(s) FAILED`);
  process.exit(1);
} else {
  console.log('All checks passed.');
}
