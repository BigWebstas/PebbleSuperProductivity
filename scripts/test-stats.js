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
const yesterday = store.yesterdayStr();

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

check('week: 7 buckets oldest-first, last labelled "Today", workedWeekMs is their sum', () => {
  const iso = (n) => { const d = new Date(); d.setDate(d.getDate() - n); return d.toISOString().slice(0, 10); };
  const s = build([
    addTask({ id: 'a', title: 'A', timeSpentOnDay: { [iso(0)]: 600000, [iso(3)]: 1200000, [iso(9)]: 999 } }),
    addTask({ id: 'b', title: 'B', timeSpentOnDay: { [iso(0)]: 300000 } }),
  ]);
  const stats = store.computeStats(s);
  assert.strictEqual(stats.week.length, 7);
  assert.strictEqual(stats.week[6].label, 'Today');
  assert.strictEqual(stats.week[6].ms, 900000);   // a + b today
  assert.strictEqual(stats.week[3].ms, 1200000);  // a, 3 days ago
  assert.strictEqual(stats.workedWeekMs, 2100000); // the iso(9) entry is outside the window
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

check('worked / completed yesterday come from the yesterday day-key and doneOn', () => {
  const py = new Date(); py.setDate(py.getDate() - 1);
  const s = build([
    addTask({ id: 'a', title: 'A', timeSpentOnDay: (function () { const m = {}; m[yesterday] = 700000; m[today] = 100000; return m; })() }),
    addTask({ id: 'b', title: 'B', isDone: true, doneOn: py.getTime() }),
    addTask({ id: 'c', title: 'C', isDone: true, doneOn: Date.now() }),
  ]);
  const stats = store.computeStats(s);
  assert.strictEqual(stats.workedYesterdayMs, 700000);
  assert.strictEqual(stats.completedYesterdayCount, 1);
});

check('week entries carry session start/end + break count from timeTracking', () => {
  const start = new Date(); start.setHours(9, 12, 0, 0);
  const end = new Date(); end.setHours(17, 40, 0, 0);
  const s = build([
    entry('TIME_TRACKING', '[TimeTracking] Sync sessions', {
      contextType: 'PROJECT', contextId: 'p1', date: today,
      data: { s: start.getTime(), e: end.getTime(), b: 2, bt: 1200000 },
    }),
    // a second context the same day - earliest start / latest end / summed breaks
    entry('TIME_TRACKING', '[TimeTracking] Update Work Context Data', {
      ctx: { type: 'TAG', id: 't1' }, date: today,
      data: undefined, updates: { s: new Date(new Date().setHours(8, 30, 0, 0)).getTime(), b: 1 },
    }),
  ]);
  const wk = store.computeStats(s).week;
  const todayRow = wk[wk.length - 1];
  assert.strictEqual(todayRow.startMin, 8 * 60 + 30);
  assert.strictEqual(todayRow.endMin, 17 * 60 + 40);
  assert.strictEqual(todayRow.breaks, 3);
  // a day with no session data
  assert.strictEqual(wk[0].startMin, -1);
  assert.strictEqual(wk[0].breaks, 0);
});

check('statsToMarkdown: renders the sections + mermaid fences, escapes labels', () => {
  const s = build([
    addProject({ id: 'p1', title: 'Wo|rk' }),
    addTask({ id: 'a', title: 'A', projectId: 'p1' }),
    addTask({ id: 'b', title: 'B', dueDay: today, isDone: true, timeSpentOnDay: dayMap(600000) }),
  ]);
  const habits = [
    { id: 'h1', title: 'Read "daily"', streak: 4, bestStreak: 9 },
    { id: 'h2', title: 'Nothing', streak: 0, bestStreak: 0 },
  ];
  const md = store.statsToMarkdown(store.computeStats(s), habits);
  assert.ok(md.indexOf('# Super Productivity') === 0);
  assert.ok(md.indexOf('## Today & yesterday') !== -1);
  assert.ok(md.indexOf('```mermaid\nxychart-beta') !== -1);
  assert.ok(md.indexOf('```mermaid\npie showData') !== -1);
  assert.ok(md.indexOf('## Habit streaks') !== -1);
  // habit with no streak is dropped; the quotes in a label are sanitised
  assert.ok(md.indexOf('Nothing') === -1);
  assert.ok(md.indexOf('Read \'daily\'') !== -1 || md.indexOf('Read  daily') !== -1);
  // the pie label keeps the raw pipe (it is not a table there)
  assert.ok(md.indexOf('"Wo|rk" : 1') !== -1);
});

check('statsToMarkdown: no habits section when none have a streak', () => {
  const md = store.statsToMarkdown(store.computeStats(store.emptyState()), []);
  assert.ok(md.indexOf('## Habit streaks') === -1);
  assert.ok(md.indexOf('_No open tasks._') !== -1);
});

check('computeSearch: token AND match across every task, undone first', () => {
  const s = build([
    addProject({ id: 'p1', title: 'Work' }),
    addTask({ id: 'a', title: 'buy milk carton', projectId: 'p1' }),
    addTask({ id: 'b', title: 'Milk the cow', isDone: true }),
    addTask({ id: 'c', title: 'Paint the fence' }),
    addTask({ id: 'd', title: 'buy milk again', __inBacklog: true }),
  ]);
  const hits = store.computeSearch(s, 'BUY milk', 40);
  assert.deepStrictEqual(hits.map((h) => h.title), ['buy milk again', 'buy milk carton']);
  assert.strictEqual(hits[0].project, 'No project'); // 'again' has no projectId
  assert.strictEqual(hits[1].project, 'Work');       // 'carton' lives in Work
  // a lone "milk" token also finds the done one, and it sorts last
  const all = store.computeSearch(s, 'milk', 40);
  assert.strictEqual(all.length, 3);
  assert.strictEqual(all[all.length - 1].done, true);
});

check('computeSearch: empty / whitespace query returns nothing', () => {
  const s = build([addTask({ id: 'a', title: 'Anything' })]);
  assert.deepStrictEqual(store.computeSearch(s, '   ', 40), []);
  assert.deepStrictEqual(store.computeSearch(s, '', 40), []);
});

check('computeSearch: dictated punctuation in the query still matches', () => {
  const s = build([
    addTask({ id: 'a', title: 'Buy milk' }),
    addTask({ id: 'b', title: 'Call the plumber' }),
  ]);
  // a trailing period / stray comma the way dictation returns it
  assert.strictEqual(store.computeSearch(s, 'milk.', 40).length, 1);
  assert.strictEqual(store.computeSearch(s, 'buy, milk', 40).length, 1);
  assert.strictEqual(store.computeSearch(s, 'Plumber!', 40).length, 1);
});

console.log('');
if (failures > 0) {
  console.log(`${failures} check(s) FAILED`);
  process.exit(1);
} else {
  console.log('All checks passed.');
}
