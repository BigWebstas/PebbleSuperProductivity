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

check('CRT then UPD builds a merged task', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      { type: 'CRT', entityType: 'task', entityId: 't1', payload: { id: 't1', title: 'Buy milk', isDone: false, dueDay: today } },
      { type: 'UPD', entityType: 'task', entityId: 't1', payload: { isDone: true } },
    ],
    state
  );
  assert.strictEqual(state.task.t1.title, 'Buy milk');
  assert.strictEqual(state.task.t1.isDone, true);
});

check('DEL removes an entity', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      { type: 'CRT', entityType: 'task', entityId: 't1', payload: { id: 't1', title: 'X', dueDay: today } },
      { type: 'DEL', entityType: 'task', entityId: 't1' },
    ],
    state
  );
  assert.strictEqual(state.task.t1, undefined);
});

check('DEL with batch ids removes multiple', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      { type: 'CRT', entityType: 'task', entityId: 't1', payload: { id: 't1', title: 'X', dueDay: today } },
      { type: 'CRT', entityType: 'task', entityId: 't2', payload: { id: 't2', title: 'Y', dueDay: today } },
      { type: 'DEL', entityType: 'task', payload: { ids: ['t1', 't2'] } },
    ],
    state
  );
  assert.strictEqual(Object.keys(state.task).length, 0);
});

check('malformed op does not throw', () => {
  const state = store.emptyState();
  assert.doesNotThrow(() => {
    store.applyOperations([{ type: 'UPD', entityType: 'task', entityId: 't1', payload: null }], state);
    store.applyOperations([{ type: 'WEIRD_FUTURE_TYPE' }], state);
  });
});

check('getTodayTasks prefers dueDay===today, not-done first, respects limit', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      { type: 'CRT', entityType: 'task', entityId: 'a', payload: { id: 'a', title: 'Zebra', isDone: false, dueDay: today } },
      { type: 'CRT', entityType: 'task', entityId: 'b', payload: { id: 'b', title: 'Apple', isDone: true, dueDay: today } },
      { type: 'CRT', entityType: 'task', entityId: 'c', payload: { id: 'c', title: 'Later task', isDone: false, dueDay: '2099-01-01' } },
    ],
    state
  );
  const tasks = store.getTodayTasks(state, 30);
  assert.deepStrictEqual(tasks.map((t) => t.id), ['a', 'b']); // 'c' excluded, not-done ('a') before done ('b')
});

check('getTodayTasks falls back to not-done tasks when nothing matches dueDay', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      { type: 'CRT', entityType: 'task', entityId: 'a', payload: { id: 'a', title: 'No due date', isDone: false } },
      { type: 'CRT', entityType: 'task', entityId: 'b', payload: { id: 'b', title: 'Done already', isDone: true } },
    ],
    state
  );
  const tasks = store.getTodayTasks(state, 30);
  assert.deepStrictEqual(tasks.map((t) => t.id), ['a']);
});

check('getTodayTasks respects limit', () => {
  const state = store.emptyState();
  const ops = [];
  for (let i = 0; i < 50; i++) {
    ops.push({ type: 'CRT', entityType: 'task', entityId: 't' + i, payload: { id: 't' + i, title: 'Task ' + i, isDone: false, dueDay: today } });
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
