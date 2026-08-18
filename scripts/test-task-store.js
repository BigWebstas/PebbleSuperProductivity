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
// { serverSeq, op: {...}, receivedAt } with an uppercase entityType and an
// opType field (not type) - entry() below builds that real shape.
function entry(opType, entityType, entityId, payload) {
  return { serverSeq: 1, op: { opType, entityType, entityId, payload }, receivedAt: 1 };
}

check('CRT then UPD builds a merged task', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      entry('CRT', 'TASK', 't1', { id: 't1', title: 'Buy milk', isDone: false, dueDay: today }),
      entry('UPD', 'TASK', 't1', { isDone: true }),
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
      entry('CRT', 'TASK', 't1', { id: 't1', title: 'X', dueDay: today }),
      entry('DEL', 'TASK', 't1', undefined),
    ],
    state
  );
  assert.strictEqual(state.task.t1, undefined);
});

check('DEL with batch ids removes multiple', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      entry('CRT', 'TASK', 't1', { id: 't1', title: 'X', dueDay: today }),
      entry('CRT', 'TASK', 't2', { id: 't2', title: 'Y', dueDay: today }),
      entry('DEL', 'TASK', undefined, { ids: ['t1', 't2'] }),
    ],
    state
  );
  assert.strictEqual(Object.keys(state.task).length, 0);
});

check('malformed op does not throw', () => {
  const state = store.emptyState();
  assert.doesNotThrow(() => {
    store.applyOperations([entry('UPD', 'TASK', 't1', null)], state);
    store.applyOperations([entry('WEIRD_FUTURE_TYPE', 'TASK', 't1', undefined)], state);
    store.applyOperations([{ serverSeq: 1, receivedAt: 1 }], state); // missing op entirely
  });
});

check('getTodayTasks prefers dueDay===today, not-done first, respects limit', () => {
  const state = store.emptyState();
  store.applyOperations(
    [
      entry('CRT', 'TASK', 'a', { id: 'a', title: 'Zebra', isDone: false, dueDay: today }),
      entry('CRT', 'TASK', 'b', { id: 'b', title: 'Apple', isDone: true, dueDay: today }),
      entry('CRT', 'TASK', 'c', { id: 'c', title: 'Later task', isDone: false, dueDay: '2099-01-01' }),
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
      entry('CRT', 'TASK', 'a', { id: 'a', title: 'No due date', isDone: false }),
      entry('CRT', 'TASK', 'b', { id: 'b', title: 'Done already', isDone: true }),
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
    ops.push(entry('CRT', 'TASK', 't' + i, { id: 't' + i, title: 'Task ' + i, isDone: false, dueDay: today }));
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
