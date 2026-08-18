// Sanity checks for the operation-replay / today-filter logic in
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
// { serverSeq, op: {...}, receivedAt }, and for TASK entities op.payload is
// a Redux-action envelope { actionPayload: {...} } whose shape depends on
// op.actionType - NOT a flat entity record. entry() below builds that real
// shape for the TASK action types confirmed against
// root-store/meta/task-shared.actions.ts in the super-productivity repo.
function entry(actionType, actionPayload) {
  return {
    serverSeq: 1,
    op: { opType: 'UPD', entityType: 'TASK', actionType: actionType, payload: { actionPayload: actionPayload } },
    receivedAt: 1,
  };
}

function addTask(task) {
  return entry('[Task Shared] addTask', { task: task });
}

function updateTask(id, changes) {
  return entry('[Task Shared] updateTask', { task: { id: id, changes: changes } });
}

function planTasksForToday(date, taskIds) {
  return entry('[Task Shared] planTasksForToday', { today: date, taskIds: taskIds });
}

check('addTask then updateTask builds a merged task', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      addTask({ id: 't1', title: 'Buy milk', isDone: false, dueDay: today }),
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
            a: { id: 'a', title: 'Pre-existing task', isDone: false, dueDay: today },
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

check('updateTask on an unknown id creates a bare record rather than throwing', () => {
  const state = store.emptyState();
  assert.doesNotThrow(() => {
    store.applyOperations([updateTask('ghost', { isDone: true })], state);
  });
  assert.strictEqual(state.task.ghost.isDone, true);
});

check('planTasksForToday sets dueDay on existing tasks (the real today-membership field)', () => {
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
      entry('[Task Shared] unscheduleTask', { id: 'a' }),
    ],
    state
  );
  assert.strictEqual(state.task.a.dueDay, undefined);
  assert.strictEqual(state.task.a.remindAt, undefined);
});

check('unscheduleTask with isLeaveInToday keeps the task in today', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      addTask({ id: 'a', title: 'X', dueWithTime: Date.now() }),
      entry('[Task Shared] unscheduleTask', { id: 'a', isLeaveInToday: true, today: today }),
    ],
    state
  );
  assert.strictEqual(state.task.a.dueDay, today);
  assert.strictEqual(state.task.a.dueWithTime, undefined);
});

check('deleteTask removes the task entirely', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      addTask({ id: 'a', title: 'X', dueDay: today }),
      entry('[Task Shared] deleteTask', { task: { id: 'a' } }),
    ],
    state
  );
  assert.strictEqual(state.task.a, undefined);
});

check('deleteTasks (bulk) removes all listed tasks', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      addTask({ id: 'a', title: 'X', dueDay: today }),
      addTask({ id: 'b', title: 'Y', dueDay: today }),
      entry('[Task Shared] deleteTasks', { taskIds: ['a', 'b'] }),
    ],
    state
  );
  assert.deepStrictEqual(state.task, {});
});

check('moveToArchive removes tasks from the active view even if dueDay still says today', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      addTask({ id: 'a', title: 'X', dueDay: today, isDone: true }),
      entry('[Task Shared] moveToArchive', { tasks: [{ id: 'a' }] }),
    ],
    state
  );
  assert.strictEqual(state.task.a, undefined);
});

check('unrecognized TASK actionType is ignored, not thrown', () => {
  const state = store.emptyState();
  assert.doesNotThrow(() => {
    store.applyOperations([entry('[Task Shared] dismissReminderOnly', { id: 't1' })], state);
    store.applyOperations([entry('[TimeTracking] Sync time spent', { taskId: 't1', date: today, duration: 100 })], state);
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

check('getTodayTasks only returns tasks actually due today - no backlog fallback', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      addTask({ id: 'a', title: 'Zebra', isDone: false, dueDay: today }),
      addTask({ id: 'b', title: 'Apple', isDone: true, dueDay: today }),
      addTask({ id: 'c', title: 'Backlog, no due date', isDone: false }),
      addTask({ id: 'd', title: 'Due some other day', isDone: false, dueDay: '2099-01-01' }),
    ],
    state
  );
  const tasks = store.getTodayTasks(state, 30);
  assert.deepStrictEqual(tasks.map((t) => t.id), ['a', 'b']); // not-done ('a') before done ('b'); c and d excluded
});

check('getTodayTasks returns nothing when nothing is due today (no fallback)', () => {
  const state = store.emptyState();
  store.applyOperations([addTask({ id: 'a', title: 'Backlog', isDone: false })], state);
  assert.deepStrictEqual(store.getTodayTasks(state, 30), []);
});

check('getTodayTasks falls back to dueWithTime (calendar-imported tasks) when dueDay is absent', () => {
  const state = store.emptyState();
  const todayNoon = new Date();
  todayNoon.setHours(12, 0, 0, 0);
  store.applyOperations(
    [addTask({ id: 'a', title: 'Calendar event', isDone: false, dueWithTime: todayNoon.getTime() })],
    state
  );
  const tasks = store.getTodayTasks(state, 30);
  assert.deepStrictEqual(tasks.map((t) => t.id), ['a']);
});

check('getTodayTasks respects limit', () => {
  const state = store.emptyState();
  const ops = [];
  for (let i = 0; i < 50; i++) {
    ops.push(addTask({ id: 't' + i, title: 'Task ' + i, isDone: false, dueDay: today }));
  }
  store.applyOperations(ops, state);
  assert.strictEqual(store.getTodayTasks(state, 30).length, 30);
});

console.log('');
if (failures > 0) {
  console.log(`${failures} check(s) FAILED`);
  process.exit(1);
} else {
  console.log('All checks passed.');
}
