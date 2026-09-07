// Sanity checks for src/pkjs/lib/timeline.js - the task-store -> PebbleOS
// timeline-pin projection and the diff against what was last pushed.
// Run with: node scripts/test-timeline.js
'use strict';

const assert = require('assert');
const timeline = require('../src/pkjs/lib/timeline.js');

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

const NOW = Date.parse('2026-09-06T12:00:00Z');
const HOUR = 3600 * 1000;
const DAY = 24 * HOUR;

function state(tasks, projects) {
  return { task: tasks || {}, project: projects || {} };
}

// ---- desiredPins ----

check('a task scheduled tomorrow becomes one soonest-first pin', () => {
  const s = state({
    a: { id: 'a', title: 'Water tomatoes', dueWithTime: NOW + 26 * HOUR },
    b: { id: 'b', title: 'Standup', dueWithTime: NOW + 2 * HOUR },
  });
  const pins = timeline.desiredPins(s, NOW, {});
  assert.strictEqual(pins.length, 2);
  assert.strictEqual(pins[0].id, 'sp-task-b'); // sooner first
  assert.strictEqual(pins[1].id, 'sp-task-a');
  assert.strictEqual(pins[0].layout.type, 'genericPin');
  assert.strictEqual(pins[0].layout.title, 'Standup');
  assert.strictEqual(pins[0].time, new Date(NOW + 2 * HOUR).toISOString());
});

check('done / undated / past / beyond-horizon tasks are all excluded', () => {
  const s = state({
    done: { id: 'done', title: 'D', dueWithTime: NOW + HOUR, isDone: true },
    undated: { id: 'undated', title: 'U' },
    dayOnly: { id: 'dayOnly', title: 'Day', dueDay: '2026-09-07' },
    past: { id: 'past', title: 'P', dueWithTime: NOW - HOUR },
    far: { id: 'far', title: 'F', dueWithTime: NOW + 20 * DAY },
    ghost: { id: 'ghost', dueWithTime: NOW + HOUR },
    ok: { id: 'ok', title: 'OK', dueWithTime: NOW + HOUR },
  });
  const pins = timeline.desiredPins(s, NOW, {});
  assert.deepStrictEqual(pins.map((p) => p.id), ['sp-task-ok']);
});

check('project name rides along as the pin body', () => {
  const s = state(
    { a: { id: 'a', title: 'T', dueWithTime: NOW + HOUR, projectId: 'p1' } },
    { p1: { id: 'p1', title: 'Garden' } }
  );
  assert.strictEqual(timeline.desiredPins(s, NOW, {})[0].layout.body, 'Garden');
});

check('reminder lead: default 10 min, opt override, 0 suppresses', () => {
  const s = state({ a: { id: 'a', title: 'T', dueWithTime: NOW + 3 * HOUR } });

  const def = timeline.desiredPins(s, NOW, {})[0];
  assert.strictEqual(def.reminders[0].time, new Date(NOW + 3 * HOUR - 10 * 60000).toISOString());
  assert.strictEqual(def.reminders[0].layout.type, 'genericReminder');

  const wide = timeline.desiredPins(s, NOW, { leadMin: 30 })[0];
  assert.strictEqual(wide.reminders[0].time, new Date(NOW + 3 * HOUR - 30 * 60000).toISOString());

  const none = timeline.desiredPins(s, NOW, { leadMin: 0 })[0];
  assert.ok(!none.reminders);
});

check('a reminder that would fall in the past is dropped, pin still made', () => {
  const s = state({ a: { id: 'a', title: 'Soon', dueWithTime: NOW + 5 * 60000 } });
  const pin = timeline.desiredPins(s, NOW, { leadMin: 10 })[0];
  assert.ok(pin);
  assert.ok(!pin.reminders);
});

check('MAX_PINS caps the set, keeping the soonest', () => {
  const tasks = {};
  for (let i = 0; i < timeline.MAX_PINS + 10; i++) {
    tasks['t' + i] = { id: 't' + i, title: 'T' + i, dueWithTime: NOW + HOUR + i * 60000 };
  }
  const pins = timeline.desiredPins(state(tasks), NOW, {});
  assert.strictEqual(pins.length, timeline.MAX_PINS);
  assert.strictEqual(pins[0].id, 'sp-task-t0');
});

// ---- plan (diff) ----

check('plan: new pin -> put, unchanged -> skip, vanished -> delete', () => {
  const s = state({
    keep: { id: 'keep', title: 'Keep', dueWithTime: NOW + HOUR },
    fresh: { id: 'fresh', title: 'Fresh', dueWithTime: NOW + 2 * HOUR },
  });
  const desired = timeline.desiredPins(s, NOW, {});
  const keepPin = desired.find((p) => p.id === 'sp-task-keep');
  const pushed = {
    'sp-task-keep': timeline.fingerprint(keepPin),
    'sp-task-old': 'whatever',
  };
  const work = timeline.plan(desired, pushed);
  assert.deepStrictEqual(work.puts.map((p) => p.id), ['sp-task-fresh']);
  assert.deepStrictEqual(work.deletes, ['sp-task-old']);
});

check('plan: a retimed task re-puts (fingerprint changed)', () => {
  const before = timeline.desiredPins(
    state({ a: { id: 'a', title: 'T', dueWithTime: NOW + HOUR } }), NOW, {});
  const pushed = { 'sp-task-a': timeline.fingerprint(before[0]) };
  const after = timeline.desiredPins(
    state({ a: { id: 'a', title: 'T', dueWithTime: NOW + 5 * HOUR } }), NOW, {});
  const work = timeline.plan(after, pushed);
  assert.deepStrictEqual(work.puts.map((p) => p.id), ['sp-task-a']);
  assert.deepStrictEqual(work.deletes, []);
});

check('plan: everything gone (feature off) -> delete all, no puts', () => {
  const pushed = { 'sp-task-a': 'x', 'sp-task-b': 'y' };
  const work = timeline.plan([], pushed);
  assert.deepStrictEqual(work.puts, []);
  assert.deepStrictEqual(work.deletes.sort(), ['sp-task-a', 'sp-task-b']);
});

// ---- read / write ----

check('read tolerates missing, junk, and non-object values', () => {
  const mk = (v) => ({ getItem: () => v });
  assert.deepStrictEqual(timeline.read(mk(null)), {});
  assert.deepStrictEqual(timeline.read(mk('not json')), {});
  assert.deepStrictEqual(timeline.read(mk('[1,2]')), {});
  assert.deepStrictEqual(timeline.read(mk('{"sp-task-a":"fp"}')), { 'sp-task-a': 'fp' });
});

check('write round-trips through a fake storage', () => {
  let box = null;
  const storage = { getItem: () => box, setItem: (_k, v) => { box = v; } };
  timeline.write(storage, { 'sp-task-a': 'fp' });
  assert.deepStrictEqual(timeline.read(storage), { 'sp-task-a': 'fp' });
});

console.log('');
if (failures > 0) {
  console.log(`${failures} check(s) FAILED`);
  process.exit(1);
} else {
  console.log('All checks passed.');
}
