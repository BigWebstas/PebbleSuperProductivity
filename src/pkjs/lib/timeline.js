// PebbleKit JS: turns the replayed task store into a set of PebbleOS
// timeline pins - one pin per task that is scheduled to a specific time of
// day (task.dueWithTime) inside a forward window, each with an optional
// single reminder. index.js's syncTimelinePins() does the actual HTTP
// PUT/DELETE against Rebble's timeline service; everything here is pure and
// storage-injected so it unit-tests the same way lib/op-queue.js does.
//
// What reaches the timeline service is NOT end-to-end encrypted: the pin
// carries the task title and project name in clear, outside the SuperSync
// E2EE boundary. That is why the feature is opt-in (config.enableTimeline,
// default off) - see the pairing page's own hint.
//
// Pins are keyed by task id (pinIdFor), so re-pushing a changed task
// overwrites its pin instead of adding a second one. A fingerprint map of
// what was last pushed (STORAGE_KEY) lets an unchanged pin cost zero
// requests, and lets a task that was completed / unscheduled / pushed out
// past the horizon have its stale pin deleted on the next sync. Turning the
// feature off deletes every pin this app created, one sync later.
'use strict';

var STORAGE_KEY = 'sp_timeline_pins';
var PIN_ID_PREFIX = 'sp-task-';

// How far ahead to pin. Rebble's timeline only holds a limited future
// window and a limited pin count anyway; two weeks keeps the working set
// small while covering every realistically-scheduled task.
var HORIZON_DAYS = 14;
var MAX_PINS = 50;

// Cap the PUT/DELETE calls made in one sync so a first run over a big
// backlog of scheduled tasks doesn't fire dozens of requests back to back -
// the rest are picked up by the next sync, by which point the diff is
// normally near-empty.
var MAX_OPS_PER_SYNC = 25;

// Reminder lead time when the user hasn't set "Notify before a task is due"
// (config.dueReminderMin) to anything - that same setting doubles as the
// pin's reminder offset. 0 there means "no reminder on the pin".
var DEFAULT_LEAD_MIN = 10;

var DAY_MS = 24 * 60 * 60 * 1000;

function pinIdFor(taskId) {
  return PIN_ID_PREFIX + String(taskId);
}

function clip(s, n) {
  s = String(s == null ? '' : s);
  return s.length > n ? s.slice(0, n) : s;
}

// state -> array of pin objects ready to PUT, soonest first, capped at
// MAX_PINS. nowMs is passed in (not read from Date.now()) so tests are
// deterministic. opts.leadMin overrides DEFAULT_LEAD_MIN; opts.leadMin === 0
// suppresses the reminder entirely.
function desiredPins(state, nowMs, opts) {
  opts = opts || {};
  var leadMin = opts.leadMin == null ? DEFAULT_LEAD_MIN : opts.leadMin;
  var horizonMs = nowMs + HORIZON_DAYS * DAY_MS;
  var tasks = (state && state.task) || {};
  var projects = (state && state.project) || {};
  var out = [];

  Object.keys(tasks).forEach(function (id) {
    var t = tasks[id];
    // A ghost record (an update-style op for a task this replay never saw
    // created) has no title - same filter task-store's getActiveTasks uses.
    if (!t || !t.title || t.isDone) {
      return;
    }
    var due = t.dueWithTime;
    if (typeof due !== 'number' || due < nowMs || due > horizonMs) {
      return;
    }

    var pin = {
      id: pinIdFor(t.id),
      time: new Date(due).toISOString(),
      layout: {
        type: 'genericPin',
        title: clip(t.title, 128),
        tinyIcon: 'system://images/SCHEDULED_EVENT',
      },
    };

    var projTitle = t.projectId && projects[t.projectId] && projects[t.projectId].title;
    if (projTitle) {
      pin.layout.body = clip(projTitle, 128);
    }

    var remindAt = due - leadMin * 60000;
    // Skip a reminder that would land in the past (or within the next
    // minute) - the timeline service rejects those.
    if (leadMin > 0 && remindAt > nowMs + 60000) {
      pin.reminders = [{
        time: new Date(remindAt).toISOString(),
        layout: {
          type: 'genericReminder',
          title: clip(t.title, 128),
          tinyIcon: 'system://images/NOTIFICATION_REMINDER',
        },
      }];
    }

    out.push(pin);
  });

  out.sort(function (a, b) {
    return a.time < b.time ? -1 : (a.time > b.time ? 1 : 0);
  });
  return out.slice(0, MAX_PINS);
}

// A short string standing for every part of a pin that, if it changed,
// means the pin must be re-PUT. Stored as the value in the pushed-pins map.
function fingerprint(pin) {
  var remind = (pin.reminders && pin.reminders[0] && pin.reminders[0].time) || '';
  return [
    pin.time,
    pin.layout.title,
    pin.layout.body || '',
    remind,
  ].join('');
}

function read(storage) {
  try {
    var raw = storage.getItem(STORAGE_KEY);
    var obj = raw ? JSON.parse(raw) : null;
    return (obj && typeof obj === 'object' && !Array.isArray(obj)) ? obj : {};
  } catch (e) {
    return {};
  }
}

function write(storage, map) {
  try {
    storage.setItem(STORAGE_KEY, JSON.stringify(map || {}));
  } catch (e) {
    // localStorage full - the map just stays stale, which at worst means a
    // redundant PUT or a missed DELETE next sync. Nothing useful to do here.
  }
}

// desired pins + the map of what was last pushed ({ pinId: fingerprint })
// -> { puts: [pin, ...], deletes: [pinId, ...] }. A pin whose fingerprint
// already matches is left alone. Any pushed id not in `desired` is a task
// that was completed, unscheduled, or aged past the horizon: delete it.
function plan(desired, pushed) {
  pushed = pushed || {};
  var puts = [];
  var wanted = {};
  desired.forEach(function (pin) {
    wanted[pin.id] = true;
    if (pushed[pin.id] !== fingerprint(pin)) {
      puts.push(pin);
    }
  });
  var deletes = Object.keys(pushed).filter(function (id) {
    return !wanted[id];
  });
  return { puts: puts, deletes: deletes };
}

module.exports = {
  STORAGE_KEY: STORAGE_KEY,
  PIN_ID_PREFIX: PIN_ID_PREFIX,
  HORIZON_DAYS: HORIZON_DAYS,
  MAX_PINS: MAX_PINS,
  MAX_OPS_PER_SYNC: MAX_OPS_PER_SYNC,
  DEFAULT_LEAD_MIN: DEFAULT_LEAD_MIN,
  pinIdFor: pinIdFor,
  desiredPins: desiredPins,
  fingerprint: fingerprint,
  plan: plan,
  read: read,
  write: write,
};
