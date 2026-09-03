// Sanity checks for store.computeStats() (the watch Stats page data) in
// src/pkjs/lib/task-store.js. Run with: node scripts/test-stats.js
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

function entry(entityType, actionType, actionPayload) {
  return {
    serverSeq: 1,
    op: { opType: 'UPD', entityType: entityType, actionType: actionType, payload: { actionPayload: actionPayload } },
    receivedAt: 1,
  };
}
function addTask(task) {
  return entry('TASK', '[Task Shared] addTask', { task: task });
}
function addProject(project) {
  return entry('PROJECT', '[Project] Add Project', { project: project });
}
function build(ops) {
  const s = store.emptyState();
  store.applyOperations(ops, s);
  return s;
}
function dayMap(ms) {
  const m = {};
  m[today] = ms;
  return m;
}

check('empty state -> zeros and no projects', () => {
  const stats = store.computeStats(store.emptyState());
  assert.strictEqual(stats.estimateRemainingMs, 0);
  assert.strictEqual(stats.workedTodayMs, 0);
  assert.strictEqual(stats.completedTodayCount, 0);
  assert.deepStrictEqual(stats.projects, []);
});

check('estimate remaining sums (estimate - spent), clamped at 0, today undone only', () => {
  const s = build([
    addTask({ id: 'a', title: 'A', dueDay: today, timeEstimate: 3600000, timeSpent: 600000 }),
    addTask({ id: 'b', title: 'B', dueDay: today, timeEstimate: 1800000, timeSpent: 0 }),
    addTask({ id: 'c', title: 'C', dueDay: today, timeEstimate: 1000, timeSpent: 5000 }),
    addTask({ id: 'd', title: 'D', timeEstimate: 9999999 }),
  ]);
  assert.strictEqual(store.computeStats(s).estimateRemainingMs, 3000000 + 1800000);
});

check('a done today task: worked-time yes, estimate no, completed-today +1', () => {
  const s = build([
    addTask({ id: 'a', title: 'A', dueDay: today, isDone: true, timeEstimate: 3600000, timeSpent: 600000, timeSpentOnDay: dayMap(600000) }),
    addTask({ id: 'b', title: 'B', dueDay: today, isDone: false, timeEstimate: 900000, timeSpent: 0 }),
  ]);
  const stats = store.computeStats(s);
  assert.strictEqual(stats.estimateRemainingMs, 900000);
  assert.strictEqual(stats.workedTodayMs, 600000);
  assert.strictEqual(stats.completedTodayCount, 1);
});

check('completed-today counts done subtasks of a today parent, not the parent itself', () => {
  const s = build([
    addTask({ id: 'p', title: 'P', dueDay: today, subTaskIds: ['s1', 's2', 's3'] }),
    addTask({ id: 's1', title: 'S1', parentId: 'p', isDone: true }),
    addTask({ id: 's2', title: 'S2', parentId: 'p', isDone: true }),
    addTask({ id: 's3', title: 'S3', parentId: 'p', isDone: false, timeEstimate: 300000 }),
  ]);
  const stats = store.computeStats(s);
  assert.strictEqual(stats.completedTodayCount, 2);
  assert.strictEqual(stats.estimateRemainingMs, 300000);
});

check('a done task not on today list is not counted', () => {
  const s = build([
    addTask({ id: 'a', title: 'A', isDone: true }),
  ]);
  assert.strictEqual(store.computeStats(s).completedTodayCount, 0);
});

check('worked today sums timeSpentOnDay[today] across leaf tasks only', () => {
  const s = build([
    addTask({ id: 'a', title: 'A', timeSpentOnDay: dayMap(1200000) }),
    addTask({ id: 'b', title: 'B', timeSpentOnDay: dayMap(300000) }),
    addTask({ id: 'c', title: 'C', timeSpentOnDay: { '2000-01-01': 999 } }),
  ]);
  assert.strictEqual(store.computeStats(s).workedTodayMs, 1500000);
});

check('a roll-up parent is skipped for worked-time; its subtasks are counted', () => {
  const s = build([
    addTask({ id: 'p', title: 'P', subTaskIds: ['s1', 's2'], timeSpentOnDay: dayMap(9999999) }),
    addTask({ id: 's1', title: 'S1', parentId: 'p', timeSpentOnDay: dayMap(100000) }),
    addTask({ id: 's2', title: 'S2', parentId: 'p', timeSpentOnDay: dayMap(200000) }),
  ]);
  assert.strictEqual(store.computeStats(s).workedTodayMs, 300000);
});

check('parent estimate-remaining comes from its undone subtasks, not its own estimate', () => {
  const s = build([
    addTask({ id: 'p', title: 'P', dueDay: today, subTaskIds: ['s1', 's2'], timeEstimate: 9999999 }),
    addTask({ id: 's1', title: 'S1', parentId: 'p', timeEstimate: 600000, timeSpent: 100000 }),
    addTask({ id: 's2', title: 'S2', parentId: 'p', isDone: true, timeEstimate: 600000, timeSpent: 0 }),
  ]);
  assert.strictEqual(store.computeStats(s).estimateRemainingMs, 500000);
});

check('a today-due subtask pulls its parent into the estimate even if the parent has no date', () => {
  const s = build([
    addTask({ id: 'p', title: 'P', subTaskIds: ['s1'] }),
    addTask({ id: 's1', title: 'S1', parentId: 'p', dueDay: today, timeEstimate: 900000, timeSpent: 0 }),
  ]);
  assert.strictEqual(store.computeStats(s).estimateRemainingMs, 900000);
});

check('projects list carries undone non-done task counts, in title order', () => {
  const s = build([
    addProject({ id: 'p1', title: 'Work' }),
    addProject({ id: 'p2', title: 'Garden' }),
    addTask({ id: 'a', title: 'A', projectId: 'p1' }),
    addTask({ id: 'b', title: 'B', projectId: 'p1', isDone: true }),
    addTask({ id: 'c', title: 'C', projectId: 'p2' }),
    addTask({ id: 'd', title: 'D', projectId: 'p2' }),
  ]);
  const stats = store.computeStats(s);
  assert.deepStrictEqual(stats.projects.map((p) => p.title), ['Garden', 'Work']);
  const byTitle = {};
  stats.projects.forEach((p) => { byTitle[p.title] = p.taskCount; });
  assert.strictEqual(byTitle.Work, 1);
  assert.strictEqual(byTitle.Garden, 2);
});

console.log('');
if (failures > 0) {
  console.log(`${failures} check(s) FAILED`);
  process.exit(1);
} else {
  console.log('All checks passed.');
}
