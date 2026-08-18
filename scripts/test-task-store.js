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

function active(state, limit, groupByProject) {
  return store.getActiveTasks(state, limit == null ? 30 : limit, !!groupByProject);
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

check('planTasksForToday sets dueDay on existing tasks (real task state, even though it no longer drives the active-list filter)', () => {
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

check('planTasksForToday ignores ids that do not exist yet (matches the real reducer)', () => {
  const state = store.emptyState();
  store.applyOperations([planTasksForToday(today, ['ghost'])], state);
  assert.strictEqual(state.task.ghost, undefined);
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

check('a full-replace action (addTask/scheduleTaskWithTime/...) preserves a previously-set __inBacklog flag', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      addTask({ id: 'a', title: 'X' }, { isAddToBacklog: true }),
      // scheduleTaskWithTime replaces the whole task record - the real
      // payload never mentions __inBacklog since we invented it, so a
      // naive replace would silently un-backlog this task.
      taskEntry('[Task Shared] scheduleTaskWithTime', {
        task: { id: 'a', title: 'X', dueWithTime: Date.now() },
        dueWithTime: Date.now(),
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
    store.applyOperations([taskEntry('[TimeTracking] Sync time spent', { taskId: 't1', date: today, duration: 100 })], state);
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
  assert.strictEqual(tasks[1].title, '    - Book flights');
  assert.strictEqual(tasks[2].title, '    - Book hotel');
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

console.log('');
if (failures > 0) {
  console.log(`${failures} check(s) FAILED`);
  process.exit(1);
} else {
  console.log('All checks passed.');
}
