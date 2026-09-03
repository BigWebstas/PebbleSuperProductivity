// PebbleKit JS component: runs inside the Pebble mobile app and is the only
// part of this project with real internet access (see README.md for why -
// the Pebble C SDK has no networking API of its own). Bridges AppMessage
// to/from the watch and the SuperSync REST API.
'use strict';

var supersync = require('./lib/supersync-client.js');
var store = require('./lib/task-store.js');
var pairingPage = require('./lib/pairing-page.js');
var sha256lib = require('./lib/sha256.js');
var presence = require('./lib/presence-client.js');
var opQueue = require('./lib/op-queue.js');

// Keep in sync with the enums at the top of src/c/main.c.
var MSG_TASK_SYNC_START = 1;
var MSG_TASK_ITEM = 2;
var MSG_TASK_SYNC_END = 3;
var MSG_SYNC_STATUS = 4;
var MSG_REQUEST_SYNC = 5;
var MSG_TASK_TOGGLE = 6;
var MSG_TRACK_TIME_STOP = 7;
var MSG_HABIT_SYNC_START = 8;
var MSG_HABIT_ITEM = 9;
var MSG_HABIT_SYNC_END = 10;
var MSG_HABIT_ADJUST = 11; // watch -> phone: HABIT_ID + HABIT_DELTA (+1 or -1)
var MSG_HABIT_TRACK_STOP = 13; // watch -> phone: HABIT_ID + TRACKED_MS (this session's tracked ms, StopWatch-type only)
var MSG_FINISH_DAY = 14; // watch -> phone: archive every currently-done task (no extra keys)
var MSG_TASK_ADD = 12; // watch -> phone: TASK_TITLE (new task's dictated title)
var MSG_NOTE_APPEND = 15; // watch -> phone: TASK_ID + NOTE_TEXT (dictated text to append to this task's notes)
var MSG_NOTE_REQUEST = 16; // watch -> phone: TASK_ID (ask for this task's full notes, chunked reply)
var MSG_NOTE_SYNC_START = 17; // phone -> watch: TASK_ID + NOTE_TOTAL_LEN (bytes about to follow, 0 = no notes)
var MSG_NOTE_CHUNK = 18; // phone -> watch: TASK_ID + NOTE_CHUNK_TEXT (append this chunk)
var MSG_NOTE_SYNC_END = 19; // phone -> watch: TASK_ID (all chunks sent, render now)
// Project notes - same fetch/append shape as the TASK_ID versions above,
// just keyed by PROJECT_ID (see main.c's own comment on these enum values
// for what "a project's notes" means given the real app has no single notes
// field on a Project).
var MSG_PROJECT_NOTE_APPEND = 20; // watch -> phone: PROJECT_ID + NOTE_TEXT
var MSG_PROJECT_NOTE_REQUEST = 21; // watch -> phone: PROJECT_ID
var MSG_PROJECT_NOTE_SYNC_START = 22; // phone -> watch: PROJECT_ID + NOTE_TOTAL_LEN
var MSG_PROJECT_NOTE_CHUNK = 23; // phone -> watch: PROJECT_ID + NOTE_CHUNK_TEXT
var MSG_PROJECT_NOTE_SYNC_END = 24; // phone -> watch: PROJECT_ID
var MSG_TASK_PLAN_TOMORROW = 25; // watch -> phone: TASK_ID (set the task's dueDay to tomorrow)
var MSG_TASK_UNSCHEDULE = 26; // watch -> phone: TASK_ID (clear the task's scheduling)
// Live tracking presence (SuperSync WebSocket, see lib/presence-client.js).
// PRESENCE_STATE: 0 none, 1 tracking, 2 paused, 3 was-tracking (producer went
// silent), 4 stopped (brief linger, mirrors the desktop chip). When != 0 the
// message also carries PRESENCE_TASK_TITLE / PRESENCE_DEVICE / PRESENCE_ELAPSED_S
// / PRESENCE_CAN_STOP. Both aplite-ignored (the watch has no case for them).
var MSG_PRESENCE_UPDATE = 27; // phone -> watch: PRESENCE_* (0 = hide the LIVE UI)
var MSG_PRESENCE_STOP = 28;   // watch -> phone: stop the session currently shown
// Phase 2 - the watch broadcasts its OWN time-tracking as presence ("Pebble").
var MSG_TRACK_TIME_START = 29;    // watch -> phone: TASK_ID + TRACKED_MS (elapsed so far, 0 on a fresh start)
var MSG_PRESENCE_STOP_LOCAL = 30; // phone -> watch: a remote device stopped the watch's timer, stop it here
// Projects browser (config.enableProjects, non-aplite on the watch). The
// watch asks for the project list, then for one project's tasks; both
// replies are START / ITEM* / END sequences, same retrying send pattern as
// the task/habit lists. Project-task ITEMs reuse the TASK_* keys plus
// PROJECT_TASK_BACKLOG (0 = regular list, 1 = that project's backlog).
var MSG_PROJECT_LIST_REQUEST = 31;  // watch -> phone: (no keys)
var MSG_PROJECT_LIST_START = 32;    // phone -> watch: PROJECT_TOTAL
var MSG_PROJECT_LIST_ITEM = 33;     // phone -> watch: PROJECT_INDEX, PROJECT_ID, PROJECT_TITLE, PROJECT_COLOR
var MSG_PROJECT_LIST_END = 34;      // phone -> watch: (no keys)
var MSG_PROJECT_TASKS_REQUEST = 35; // watch -> phone: PROJECT_ID
var MSG_PROJECT_TASKS_START = 36;   // phone -> watch: PROJECT_ID, TASK_TOTAL
var MSG_PROJECT_TASKS_ITEM = 37;    // phone -> watch: PROJECT_ID, TASK_INDEX, TASK_*, PROJECT_TASK_BACKLOG
var MSG_PROJECT_TASKS_END = 38;     // phone -> watch: PROJECT_ID
var MSG_TASK_PLAN_TODAY = 39;       // watch -> phone: TASK_ID (set the task's dueDay to today)
// Stats page (config.enableStats, non-aplite). One request, one reply: the
// two headline durations as ms, plus STATS_TEXT - the project list
// preformatted as "Title\tcount" lines the watch prints verbatim.
var MSG_STATS_REQUEST = 40;         // watch -> phone: (no keys)
var MSG_STATS_DATA = 41;            // phone -> watch: STATS_EST_REMAINING_MS + STATS_WORKED_TODAY_MS + STATS_DONE_TODAY + STATS_TEXT
// Per-message chunk size for the full-notes fetch (see sendNoteChunk below).
// Well under any platform's AppMessage dictionary budget - app_message_open
// in main.c already requests the platform's own max, and this is one string
// field in an otherwise-tiny dict, not competing with a TASK_ITEM's several
// other keys. JS string .length is UTF-16 code units, not UTF-8 bytes, so a
// chunk full of multi-byte characters can encode to somewhat more than this
// many bytes on the wire - the margin below is generous enough to absorb
// that without needing to size chunks by encoded byte length instead.
var NOTE_CHUNK_LEN = 256;

var STATUS_OK = 0;
var STATUS_SYNCING = 1;
var STATUS_NOT_PAIRED = 2;
var STATUS_ERROR = 3;

// Matches the generous (emery) MAX_TASKS in main.c - every other platform
// compiles with a smaller one and safely clamps/ignores anything beyond its
// own array bound (see MAX_TASKS's own comment there), so this can just be
// the ceiling the most generous platform actually uses; no need to know
// which platform is paired.
var MAX_TASKS = 50;
// Matches the generous (emery) MAX_HABITS in main.c - every other platform
// compiles with a smaller one and safely clamps/ignores anything beyond its
// own array bound (see MAX_HABITS's own comment there), so this can just be
// the ceiling the most generous platform actually uses; no need to know
// which platform is paired. This used to be stuck at 8 (the pre-emery-bump
// ceiling) after main.c's own MAX_HABITS went to 16 for emery - silently
// truncating the habit list to 8 THIS side, before it was ever sent to the
// watch, regardless of what the watch itself could display. A habit whose
// title happened to sort past position 8 (e.g. one starting late in the
// alphabet) would vanish the moment total habit count crossed 8, with
// nothing on the watch even hinting why - confirmed as a real, reported bug
// ("Water" going missing once enough other habits existed, unrelated to any
// watch-side limit).
var MAX_HABITS = 16;

// Separates a voice-dictated note append (see handleNoteAppend) from
// whatever notes text already existed, both when re-read on the watch's own
// notes overlay (TASK_NOTES, see sendTaskAt below) and on the real app's
// desktop UI. U+2022 BULLET - confirmed live on-device (all platforms use
// the same font resource) alongside ~, ∆ (U+2206 INCREMENT), § (U+00A7),
// and ° (U+00B0), all rendering correctly on the notes overlay's system
// font (FONT_KEY_GOTHIC_18); only ✓ (U+2713 CHECK MARK, Dingbats block)
// came back as an empty missing-glyph box. Bullet reads more clearly as a
// divider than a mark that could be confused for actual note content (~ or
// literal punctuation) - same live-testing precedent as task-store.js's own
// SUBTASK_PREFIX (» confirmed working, U+2514 BOX DRAWINGS confirmed not).
var NOTE_APPEND_DIVIDER = '••••••••••••••••••••';

// Matches the real client's CURRENT_SCHEMA_VERSION (schema-version.ts) at
// time of writing. The server only validates this field's range when it's
// present (validation.service.ts treats a missing schemaVersion as "skip the
// check"), so omitting it isn't what was breaking uploads - but every op the
// real app writes carries it, and the field is required (non-optional,
// non-nullable) in both the shared request schema and the operations table,
// so it belongs on outgoing ops for correctness regardless.
var SCHEMA_VERSION = 4;

// Opening the watchapp (Pebble's 'ready' event - see the bottom of this
// file) used to always trigger a full doSync() no matter how recently one
// had already completed, including the ordinary case of just backing out
// to the watchface and reopening the app a few seconds later - a real
// network round-trip (and, on an E2EE account, real decrypt work) for data
// that can't meaningfully have changed. If the last sync finished within
// this window, the 'ready' handler pushes the already-fresh cached list
// straight to the watch instead (see pushCachedStateToWatch) rather than
// hitting the network again. Any watch-initiated action (toggle, resync
// row, etc.) still triggers its own real sync regardless of this window -
// only the passive "app just opened" trigger is throttled.
var RECENT_SYNC_SKIP_MS = 5 * 60 * 1000;

// ---------------- local storage helpers ----------------

function loadConfig() {
  try {
    return JSON.parse(localStorage.getItem('sp_config') || 'null');
  } catch (e) {
    return null;
  }
}

function saveConfig(config) {
  localStorage.setItem('sp_config', JSON.stringify(config));
}

function loadState() {
  try {
    return JSON.parse(localStorage.getItem('sp_entities') || 'null') || store.emptyState();
  } catch (e) {
    return store.emptyState();
  }
}

function saveState(state) {
  localStorage.setItem('sp_entities', JSON.stringify(state));
}

function loadLastSeq() {
  var v = localStorage.getItem('sp_last_seq');
  return v ? parseInt(v, 10) : 0;
}

function saveLastSeq(seq) {
  localStorage.setItem('sp_last_seq', String(seq));
}

// When doSync() last actually completed (successfully reached the server) -
// see RECENT_SYNC_SKIP_MS's own comment for what this gates. Deliberately
// separate from sp_last_seq: that tracks sync PROGRESS (how far through the
// account's op history), not WHEN it last ran - a long-idle watch with an
// already-fully-caught-up lastSeq would otherwise look "recently synced"
// forever, which is exactly the stale-data case this is meant to avoid.
function loadLastSyncedAt() {
  var v = localStorage.getItem('sp_last_synced_at');
  return v ? parseInt(v, 10) : 0;
}

function saveLastSyncedAt(ms) {
  localStorage.setItem('sp_last_synced_at', String(ms));
}

function getOrCreateClientId() {
  var id = localStorage.getItem('sp_client_id');
  if (!id) {
    id = 'pebble-' + Date.now().toString(36) + '-' + Math.floor(Math.random() * 1e9).toString(36);
    localStorage.setItem('sp_client_id', id);
  }
  return id;
}

// A vector clock ({ clientId: counter, ... }) is REQUIRED on every uploaded
// op - confirmed by reading the real server's source
// (packages/super-sync-server/src/sync/services/validation.service.ts's
// sanitizeVectorClock(), called from validateOp()): a missing/non-object
// vectorClock fails validation outright (INVALID_VECTOR_CLOCK) and the
// whole op upload is rejected, never stored, so no other client - including
// the real desktop app - ever sees it. This project's watch-toggle upload
// didn't send one at all until this was found; see handleTaskToggle().
//
// Tracked locally (not recomputed from scratch each time) so an upload's
// clock reflects both our own prior increments and whatever other clients'
// components we've observed in downloaded ops - mirroring (in simplified
// form) VectorClockService.getCurrentVectorClock()/incrementVectorClock()
// in the real client's src/app/core/util/vector-clock.ts. A perfectly
// pruned/merged clock isn't required for the server to ACCEPT the op
// (sanitizeVectorClock only checks shape/size, not completeness) - just a
// valid plain object - but keeping ours reasonably accurate avoids
// needlessly flagging every one of our uploads as CONCURRENT with
// everything else during the server's conflict comparison.
function loadVectorClock() {
  try {
    return JSON.parse(localStorage.getItem('sp_vector_clock') || 'null') || {};
  } catch (e) {
    return {};
  }
}

function saveVectorClock(clock) {
  localStorage.setItem('sp_vector_clock', JSON.stringify(clock));
}

function mergeVectorClocks(a, b) {
  var merged = Object.assign({}, a);
  Object.keys(b || {}).forEach(function (id) {
    merged[id] = Math.max(merged[id] || 0, b[id] || 0);
  });
  return merged;
}

function incrementVectorClock(clock, clientId) {
  var next = Object.assign({}, clock);
  next[clientId] = (next[clientId] || 0) + 1;
  return next;
}

// The Argon2id KDF is multi-second at production parameters (see
// argon2id.js), and its own derived keys are cached per-salt inside the
// crypto object - but that cache is worthless if we throw the whole object
// away and rebuild it on every doSync() call, re-deriving the same salt's
// key every sync. Cache the crypto object itself at module scope instead,
// for the lifetime of this pkjs session, invalidating only when the
// password actually changes (re-pairing).
var cachedCrypto = null;
var cachedPassword = null;

// The per-salt derived-key cache, persisted across pkjs sessions (the JS VM
// dies every time the watchapp closes, so the module-scope cache above only
// covers repeat doSync() calls within one app session). Cleared alongside
// sp_entities whenever credentials change - see the config-page handler.
// Capped so a heavily multi-device account can't grow it without bound:
// one entry per distinct client-session salt ever seen, ~70 bytes each.
var KDF_KEY_CACHE_MAX = 100;

function loadKdfKeys() {
  try {
    return JSON.parse(localStorage.getItem('sp_kdf_keys') || 'null') || {};
  } catch (e) {
    return {};
  }
}

function saveKdfKey(b64salt, b64key) {
  var map = loadKdfKeys();
  if (map[b64salt] === b64key) {
    return;
  }
  map[b64salt] = b64key;
  var salts = Object.keys(map);
  if (salts.length > KDF_KEY_CACHE_MAX) {
    // Drop oldest-inserted entries (V8 preserves string-key insertion
    // order) down to the cap - a re-derive is the only cost of a miss.
    for (var i = 0; i < salts.length - KDF_KEY_CACHE_MAX; i++) {
      delete map[salts[i]];
    }
  }
  localStorage.setItem('sp_kdf_keys', JSON.stringify(map));
}

function loadKdfEncryptSalt() {
  return localStorage.getItem('sp_kdf_encrypt_salt');
}

function saveKdfEncryptSalt(b64salt) {
  localStorage.setItem('sp_kdf_encrypt_salt', b64salt);
}

function clearKdfCache() {
  localStorage.removeItem('sp_kdf_keys');
  localStorage.removeItem('sp_kdf_encrypt_salt');
}

function getCrypto() {
  var password = localStorage.getItem('sp_password');
  if (!password) {
    return null;
  }
  if (!cachedCrypto || cachedPassword !== password) {
    cachedCrypto = supersync.createCrypto(password, {
      loadKeys: loadKdfKeys,
      saveKey: saveKdfKey,
      loadEncryptSalt: loadKdfEncryptSalt,
      saveEncryptSalt: saveKdfEncryptSalt,
    });
    cachedPassword = password;
  }
  return cachedCrypto;
}

function generateOpId() {
  return 'op-' + Date.now().toString(36) + '-' + Math.floor(Math.random() * 1e9).toString(36);
}

// Real task ids are nanoid() (~21 chars) server-side, but the server only
// validates entityId is a non-empty string <= 255 chars (validation.service.ts)
// - no nanoid-specific format required - so this just needs to be unique,
// same ad-hoc shape generateOpId() above already uses for op ids.
function generateTaskId() {
  return 'task-' + Date.now().toString(36) + '-' + Math.floor(Math.random() * 1e9).toString(36);
}

// Same ad-hoc shape as generateTaskId above, for a newly-created Note (see
// handleProjectNoteAppend) - real note ids are nanoid() (project.service.ts)
// but the server has no format requirement beyond a non-empty string.
function generateNoteId() {
  return 'note-' + Date.now().toString(36) + '-' + Math.floor(Math.random() * 1e9).toString(36);
}

// ---------------- AppMessage out ----------------

function sendStatus(code, message) {
  var config = loadConfig() || {};
  var dict = {
    MSG_TYPE: MSG_SYNC_STATUS,
    STATUS_CODE: code,
    // Always included (not conditionally), unlike STATUS_MSG below - this is
    // what lets the watch's own row layout self-correct within the very
    // next sync cycle after a settings change, since sendStatus() already
    // fires at both the start and end of every doSync() (including the one
    // webviewclosed triggers right after every settings save).
    HABITS_ENABLED: config.enableHabits !== false ? 1 : 0,
    ADD_TASK_ENABLED: config.enableAddTask !== false ? 1 : 0,
    // Projects browser row (default on, like Habits). Drives main.c's
    // s_projects_enabled / SECTION0_ROW_PROJECTS. Always included, same
    // next-sync-cycle self-correction reasoning as the two flags above.
    PROJECTS_ENABLED: config.enableProjects !== false ? 1 : 0,
    // Stats page row (default on, like Projects). Drives main.c's
    // s_stats_enabled / SECTION0_ROW_STATS. Same always-included,
    // next-sync-cycle self-correction reasoning as the flags above.
    STATS_ENABLED: config.enableStats !== false ? 1 : 0,
    // Schedule page row (default on, like Stats). Drives main.c's
    // s_schedule_enabled / SECTION0_ROW_SCHEDULE - a time-ordered view of
    // today's tasks that carry a dueWithTime. Same always-included,
    // next-sync-cycle self-correction reasoning as the flags above.
    SCHEDULE_ENABLED: config.enableSchedule !== false ? 1 : 0,
    // 0 = system default, -1 = always on, N>0 = relight-and-hold for N
    // seconds after any button press - see main.c's own s_backlight_mode
    // comment. Always included (not conditionally), same reasoning as the
    // two flags above: this is what lets a settings change reach the watch
    // within the very next sync cycle for free, since this message already
    // fires at both ends of every doSync().
    BACKLIGHT_MODE: config.backlightMode || 0,
    // Minutes between syncs, 0 = off - same field this pairing setting has
    // always stored, but now also drives the watch's own
    // schedule_next_wakeup() (main.c): PebbleKit JS only runs while this
    // watchapp is the one currently open, so the setInterval in
    // scheduleAutoSync() below can't fire once the watch has moved on to
    // the watchface or another app - the watch waking itself back up
    // periodically (via Pebble's wakeup_schedule API) is what makes this
    // setting actually work while the app is closed, not just while it's
    // open. Always included, same reasoning as the two fields above.
    AUTO_SYNC_INTERVAL_MIN: config.autoSyncIntervalMin || 0,
    // Touch navigation on/off, mirrored to the watch's s_touch_nav_enabled.
    // Off by default (opt-in) - the first-gen Pebble Time 2 touch driver
    // isn't reliable enough yet, see main.c's touch block. Ignored entirely
    // by the button-only builds. Always included, same reasoning as above.
    TOUCH_NAV_ENABLED: config.touchNav ? 1 : 0,
    // "Notify when a task runs over its estimate" on/off, mirrored to the
    // watch's s_overtime_notify_enabled. Off by default (opt-in). Drives
    // the over-estimate banner in main.c (aplite ignores it - see that
    // flag's comment). Always included, same reasoning as above.
    OVERTIME_NOTIFY_ENABLED: config.overtimeNotify ? 1 : 0,
    // Sub-option of OVERTIME_NOTIFY_ENABLED: when on, the watch re-fires the
    // over-estimate banner every 5 minutes for as long as the tracked task
    // stays over, instead of just once per session. Inert on the watch when
    // overtimeNotify itself is off. Always included, same reasoning as above.
    OVERTIME_REPEAT_ENABLED: config.overtimeRepeat ? 1 : 0,
    // Sub-option of OVERTIME_NOTIFY_ENABLED: also play an audible ping with the
    // over-estimate banner. Only speaker-equipped watches (Pebble Time 2) can
    // sound it - the others silently ignore it, as does aplite. Inert when
    // overtimeNotify itself is off. Always included, same reasoning as above.
    OVERTIME_SOUND_ENABLED: config.overtimeSound ? 1 : 0,
    // "Remind me to take a break" - minutes of tracked time on the watch,
    // banked across sessions until a real pause, before it shows a "time for a
    // break" banner. 0 = off. Drives main.c's s_break_reminder_min /
    // maybe_notify_break (aplite ignores it). Always included, same
    // next-sync-cycle self-correction reasoning as the flags above.
    BREAK_REMINDER_MIN: config.breakReminderMin || 0,
  };
  if (message) {
    dict.STATUS_MSG = String(message).slice(0, 60);
  }
  Pebble.sendAppMessage(dict, function () {}, function (e) {
    console.log('[pkjs] sendStatus failed: ' + JSON.stringify(e));
  });
}

// Sends one AppMessage dict, retrying a few times (short delay) before
// giving up - Pebble.sendAppMessage's own failure callback fires on the
// same kind of transient watch-inbox congestion the C-side begin_send()/
// outbox_failed_handler retry (see main.c) exists to paper over, just in
// the phone->watch direction instead. This matters far more here than it
// would for a one-off message: sendTaskAt/sendHabitAt below used to just
// skip straight to the next index on a dropped item instead of retrying
// it, which - since the watch keeps its whole habit list in a single
// reused buffer (s_habits in main.c, not double-buffered the way tasks
// are) - left that item's watch-side slot holding whatever was there from
// the PREVIOUS sync while HABIT_TOTAL still claimed the full new count.
// Reported live as a habit going missing (or showing stale data) that got
// MORE likely the more habits were being sent in one burst (more back-to-
// back sends, more chances for one to be dropped) and went away entirely
// once there were fewer total habits to send - exactly the "goes missing
// when I add habits, comes back when I remove them" symptom this fixes.
var ITEM_SEND_RETRIES = 3;
var ITEM_SEND_RETRY_DELAY_MS = 400;
function sendWithRetry(dict, onSuccess, onGiveUp, attemptsLeft) {
  if (attemptsLeft === undefined) {
    attemptsLeft = ITEM_SEND_RETRIES;
  }
  Pebble.sendAppMessage(dict, onSuccess, function (e) {
    if (attemptsLeft > 0) {
      setTimeout(function () {
        sendWithRetry(dict, onSuccess, onGiveUp, attemptsLeft - 1);
      }, ITEM_SEND_RETRY_DELAY_MS);
    } else {
      onGiveUp(e);
    }
  });
}

// Every give-up path below aborts the rest of THIS list send (does not
// move on to the next index, does not send SYNC_END) rather than pushing
// a partial/corrupt list - the watch just keeps showing whatever it had
// before, which is stale but at least internally consistent, and the next
// triggered sync (interval timer, manual Resync, or the follow-up sync any
// watch-initiated action already triggers) gets a clean run at the whole
// list again. Surfaced via sendStatus so it's visible on the watch, same
// "every failure should be visible" reasoning as the watch's own
// outbox_failed_handler.
function sendTaskListToWatch(tasks) {
  var start = { MSG_TYPE: MSG_TASK_SYNC_START, TASK_TOTAL: tasks.length };
  sendWithRetry(start, function () {
    sendTaskAt(tasks, 0);
  }, function (e) {
    console.log('[pkjs] giving up on TASK_SYNC_START after retries: ' + JSON.stringify(e));
    sendStatus(STATUS_ERROR, 'task sync interrupted, will retry');
  });
}

// The task list the watch renders, built from this app's config. Every
// call site that pushes the list to the watch goes through here so the
// currently-tracked task (presenceBroadcasting.taskId - what the watch last
// told us via handleTrackStart) is force-included even when todayOnly or
// the backlog would drop it: a task tracked from the Projects browser still
// has to show in the watch's pinned "TRACKING" section, which reads exactly
// this list (main.c's pinned_task_index).
function watchTaskList(state, config) {
  var trackedId = (presenceBroadcasting && presenceBroadcasting.taskId) || null;
  return store.getActiveTasks(
    state,
    MAX_TASKS,
    !!config.groupByProject,
    !!config.todayOnly,
    !!config.hideDoneTasks,
    trackedId
  );
}

// Fills the shared TASK_* fields (id, title, done, project name/id, tags,
// due time, spent/estimate) onto `dict` from a task-store row. Used by both
// the today-list send (sendTaskAt) and the Projects browser's per-project
// task send (sendProjectTaskAt) so the two stay identical field-for-field.
function fillTaskFields(dict, t) {
  dict.TASK_ID = String(t.id);
  dict.TASK_TITLE = String(t.title).slice(0, 63);
  dict.TASK_DONE = t.isDone ? 1 : 0;
  dict.TASK_PROJECT = String(t.project || '').slice(0, 31);
  if (t.projectId) {
    // Lets the watch's project row (when grouping is on - see
    // getActiveTasks' own groupProjectIds) ask for THIS project's notes
    // without the watch needing any other notion of "which project is
    // this" - TASK_PROJECT above is only ever the display name. Omitted
    // when grouping is off (t.projectId is '' there - see
    // pushTaskAndSubtasks), same "absent means nothing to show" convention
    // as TASK_DUE_MIN/TASK_TIME_SPENT_MS below.
    dict.TASK_PROJECT_ID = String(t.projectId).slice(0, 31);
  }
  if (t.projectColor) {
    // Packed GColor8 byte (task-store's projectColorRgb) - the grouped today
    // view draws it as a swatch on the project row, same as the browser list.
    dict.TASK_PROJECT_COLOR = t.projectColor;
  }
  if (t.tags) {
    // Comma-joined tag names (see task-store.js's tagTitlesFor) for the
    // watch's read-only tags overlay - long-select Back on a task row, see
    // main.c's show_tags_overlay. Sent directly (not fetched on demand the
    // way notes are) since resolved names are short and already fully
    // available locally.
    dict.TASK_TAGS = String(t.tags).slice(0, 63);
  }
  if (t.dueWithTime) {
    // Minutes since local midnight, not the raw ms timestamp or a
    // pre-formatted string - the watch has its own 12h/24h clock
    // preference (clock_is_24h_style()) and no reliable timezone info of
    // its own, so formatting happens on the C side from this, using the
    // phone's local time (matching todayStr()'s own local-day convention).
    var dueDate = new Date(t.dueWithTime);
    dict.TASK_DUE_MIN = dueDate.getHours() * 60 + dueDate.getMinutes();
  }
  if (t.timeSpent) {
    // AppMessage ints are 32-bit signed - cap well under the ~24.8 days
    // that would overflow, rather than let a very-long-lived task's total
    // wrap into a negative/garbage duration on the watch.
    dict.TASK_TIME_SPENT_MS = Math.min(t.timeSpent, 2000000000);
  }
  if (t.timeEstimate) {
    dict.TASK_TIME_ESTIMATE_MS = Math.min(t.timeEstimate, 2000000000);
  }
}

function sendTaskAt(tasks, index) {
  if (index >= tasks.length) {
    sendWithRetry({ MSG_TYPE: MSG_TASK_SYNC_END }, function () {}, function (e) {
      console.log('[pkjs] giving up on TASK_SYNC_END after retries: ' + JSON.stringify(e));
      sendStatus(STATUS_ERROR, 'task sync interrupted, will retry');
    });
    return;
  }
  var t = tasks[index];
  var dict = { MSG_TYPE: MSG_TASK_ITEM, TASK_INDEX: index };
  fillTaskFields(dict, t);
  // No TASK_NOTES here - the watch fetches a task's full notes on demand
  // (MSG_NOTE_REQUEST, see sendFullNotesForTask below) only for whichever
  // one task's overlay is currently open, rather than every row in every
  // sync carrying a preview whether or not it's ever viewed.
  sendWithRetry(dict, function () {
    sendTaskAt(tasks, index + 1);
  }, function (e) {
    console.log('[pkjs] giving up on TASK_ITEM ' + index + ' after retries: ' + JSON.stringify(e));
    sendStatus(STATUS_ERROR, 'task sync interrupted, will retry');
  });
}

function sendHabitListToWatch(habits) {
  var start = { MSG_TYPE: MSG_HABIT_SYNC_START, HABIT_TOTAL: habits.length };
  sendWithRetry(start, function () {
    sendHabitAt(habits, 0);
  }, function (e) {
    console.log('[pkjs] giving up on HABIT_SYNC_START after retries: ' + JSON.stringify(e));
    sendStatus(STATUS_ERROR, 'habit sync interrupted, will retry');
  });
}

function sendHabitAt(habits, index) {
  if (index >= habits.length) {
    sendWithRetry({ MSG_TYPE: MSG_HABIT_SYNC_END }, function () {}, function (e) {
      console.log('[pkjs] giving up on HABIT_SYNC_END after retries: ' + JSON.stringify(e));
      sendStatus(STATUS_ERROR, 'habit sync interrupted, will retry');
    });
    return;
  }
  var h = habits[index];
  var dict = {
    MSG_TYPE: MSG_HABIT_ITEM,
    HABIT_INDEX: index,
    HABIT_ID: String(h.id),
    HABIT_TITLE: String(h.title).slice(0, 63),
    HABIT_DONE: h.done ? 1 : 0,
    HABIT_VALUE: h.value,
    HABIT_GOAL: h.goal,
    // 0 = ClickCounter, 1 = StopWatch, 2 = RepeatedCountdownReminder - keep
    // in sync with main.c's MSG_HABIT_ITEM parsing.
    HABIT_TYPE: h.isStopwatch ? 1 : (h.isCountdown ? 2 : 0),
  };
  if (h.isCountdown && h.countdownMs) {
    dict.HABIT_COUNTDOWN_MS = Math.min(h.countdownMs, 2000000000);
  }
  sendWithRetry(dict, function () {
    sendHabitAt(habits, index + 1);
  }, function (e) {
    console.log('[pkjs] giving up on HABIT_ITEM ' + index + ' after retries: ' + JSON.stringify(e));
    sendStatus(STATUS_ERROR, 'habit sync interrupted, will retry');
  });
}

// ---------------- projects browser ----------------

// Answers MSG_PROJECT_LIST_REQUEST: sends the project list as
// START(PROJECT_TOTAL) / ITEM* / END, same retrying, abort-on-give-up shape
// as the task/habit list sends. Read-only - no server call, just the
// replayed op-log state the phone already holds. A give-up leaves the watch
// showing whatever it had; the watch can re-request on the next open.
function handleProjectListRequest() {
  var config = loadConfig();
  if (!config || !config.jwt) {
    sendStatus(STATUS_NOT_PAIRED);
    return;
  }
  if (config.enableProjects === false) {
    return;
  }
  var projects = store.getProjectList(loadState());
  sendWithRetry({ MSG_TYPE: MSG_PROJECT_LIST_START, PROJECT_TOTAL: projects.length }, function () {
    sendProjectListAt(projects, 0);
  }, function (e) {
    console.log('[pkjs] giving up on PROJECT_LIST_START after retries: ' + JSON.stringify(e));
  });
}

function sendProjectListAt(projects, index) {
  if (index >= projects.length) {
    sendWithRetry({ MSG_TYPE: MSG_PROJECT_LIST_END }, function () {}, function (e) {
      console.log('[pkjs] giving up on PROJECT_LIST_END after retries: ' + JSON.stringify(e));
    });
    return;
  }
  var p = projects[index];
  var dict = {
    MSG_TYPE: MSG_PROJECT_LIST_ITEM,
    PROJECT_INDEX: index,
    PROJECT_ID: String(p.id).slice(0, 31),
    PROJECT_TITLE: String(p.title).slice(0, 63),
  };
  if (p.color) {
    // Packed Pebble GColor8 byte (see projectColorRgb); the watch assigns it
    // straight to the swatch fill. Omitted when 0 = no theme colour.
    dict.PROJECT_COLOR = p.color;
  }
  sendWithRetry(dict, function () {
    sendProjectListAt(projects, index + 1);
  }, function (e) {
    console.log('[pkjs] giving up on PROJECT_LIST_ITEM ' + index + ', aborting: ' + JSON.stringify(e));
  });
}

// Answers MSG_PROJECT_TASKS_REQUEST for one project: its regular list then
// its backlog, concatenated, as START(PROJECT_ID, TASK_TOTAL) / ITEM* /
// END(PROJECT_ID). Each ITEM carries PROJECT_TASK_BACKLOG so the watch can
// draw the backlog as its own divided section. PROJECT_ID rides every
// message so a reply the watch has already navigated away from is easy to
// ignore. Each list is capped at MAX_TASKS (matching the watch's buffer).
function handleProjectTasksRequest(projectId) {
  var config = loadConfig();
  if (!config || !config.jwt) {
    sendStatus(STATUS_NOT_PAIRED);
    return;
  }
  if (config.enableProjects === false) {
    return;
  }
  if (projectId === undefined || projectId === null) {
    return;
  }
  sendProjectTasks(String(projectId), loadState(), config);
}

// Pushes one project's task list to the watch (regular rows then backlog rows).
// Also called after a reschedule that pulled a task out of that project's
// backlog, so the browser's task view updates in place.
function sendProjectTasks(pid, state, config) {
  var split = store.getProjectTasks(state, pid, MAX_TASKS, !!config.hideDoneTasks);
  var rows = split.regular.map(function (t) { return { t: t, backlog: 0 }; })
    .concat(split.backlog.map(function (t) { return { t: t, backlog: 1 }; }));
  var idField = pid.slice(0, 31);
  sendWithRetry({ MSG_TYPE: MSG_PROJECT_TASKS_START, PROJECT_ID: idField, TASK_TOTAL: rows.length }, function () {
    sendProjectTaskAt(idField, rows, 0);
  }, function (e) {
    console.log('[pkjs] giving up on PROJECT_TASKS_START after retries: ' + JSON.stringify(e));
  });
}

function sendProjectTaskAt(idField, rows, index) {
  if (index >= rows.length) {
    sendWithRetry({ MSG_TYPE: MSG_PROJECT_TASKS_END, PROJECT_ID: idField }, function () {}, function (e) {
      console.log('[pkjs] giving up on PROJECT_TASKS_END after retries: ' + JSON.stringify(e));
    });
    return;
  }
  var row = rows[index];
  var dict = {
    MSG_TYPE: MSG_PROJECT_TASKS_ITEM,
    PROJECT_ID: idField,
    TASK_INDEX: index,
    PROJECT_TASK_BACKLOG: row.backlog,
  };
  fillTaskFields(dict, row.t);
  sendWithRetry(dict, function () {
    sendProjectTaskAt(idField, rows, index + 1);
  }, function (e) {
    console.log('[pkjs] giving up on PROJECT_TASKS_ITEM ' + index + ', aborting: ' + JSON.stringify(e));
  });
}

// Answers MSG_STATS_REQUEST (the watch's Stats page, non-aplite). One
// message: the two headline durations in ms, plus STATS_TEXT - the project
// list preformatted as "Title\tcount" lines the watch prints verbatim under
// its "PROJECTS" heading. Read-only, no server call - just the replayed
// op-log state the phone already holds. STATS_TEXT is capped so it fits a
// single AppMessage.
function handleStatsRequest() {
  var config = loadConfig();
  if (!config || !config.jwt) {
    sendStatus(STATUS_NOT_PAIRED);
    return;
  }
  if (config.enableStats === false) {
    return;
  }
  var stats = store.computeStats(loadState());
  var lines = stats.projects.map(function (p) {
    return String(p.title).replace(/[\t\n]/g, ' ').slice(0, 40) + ' - ' + p.taskCount;
  }).join('\n');
  if (lines.length > 600) {
    lines = lines.slice(0, 600);
  }
  sendWithRetry({
    MSG_TYPE: MSG_STATS_DATA,
    STATS_EST_REMAINING_MS: Math.min(stats.estimateRemainingMs, 2e9),
    STATS_WORKED_TODAY_MS: Math.min(stats.workedTodayMs, 2e9),
    STATS_DONE_TODAY: stats.completedTodayCount,
    STATS_TEXT: lines,
  }, function () {}, function (e) {
    console.log('[pkjs] giving up on STATS_DATA after retries: ' + JSON.stringify(e));
  });
}

// ---------------- sync engine ----------------

var syncInFlight = false;
var autoSyncTimerId = null;

// (Re)starts the periodic background sync timer from config.autoSyncIntervalMin
// (minutes; 0/unset means off - this is an opt-in feature, so an existing
// user who's never touched this setting shouldn't suddenly start polling
// the server in the background). Always clears any previously running timer
// first, since this is called both at startup and after every settings
// save - a stale timer from a since-changed (or since-disabled) interval
// would otherwise keep firing alongside, or instead of, the new one.
function scheduleAutoSync(config) {
  if (autoSyncTimerId) {
    clearInterval(autoSyncTimerId);
    autoSyncTimerId = null;
  }
  var minutes = config && config.autoSyncIntervalMin;
  if (minutes > 0) {
    autoSyncTimerId = setInterval(function () {
      doSync();
    }, minutes * 60 * 1000);
  }
}

// Pushes whatever's already in the local cache (loadState()) straight to
// the watch - no network involved. Shared by doSync()'s own success path
// (after a real sync just refreshed that cache) and the 'ready' handler's
// recent-sync skip path (see RECENT_SYNC_SKIP_MS), so the two can't drift
// out of sync with each other on what "push the current state" means.
function pushCachedStateToWatch(config) {
  var state = loadState();
  var tasks = watchTaskList(state, config);
  sendTaskListToWatch(tasks);
  if (config.enableHabits !== false) {
    var habits = store.getActiveHabits(state, MAX_HABITS);
    sendHabitListToWatch(habits);
  }
  sendStatus(STATUS_OK);
}

function doSync() {
  if (syncInFlight) {
    return Promise.resolve();
  }
  var config = loadConfig();
  if (!config || !config.jwt) {
    sendStatus(STATUS_NOT_PAIRED);
    return Promise.resolve();
  }

  syncInFlight = true;
  sendStatus(STATUS_SYNCING);

  var client = new supersync.SuperSyncClient({ baseUrl: config.baseUrl, token: config.jwt });
  var crypto = getCrypto();
  var state = loadState();
  var lastSeq = loadLastSeq();
  var vectorClock = loadVectorClock();
  var isFirstSync = lastSeq === 0 && Object.keys(state.task).length === 0;
  // Tracked across the whole multi-page pullPage() loop, not reset per
  // page - each page's own done/total ratio starts back near 0% (it's a
  // fresh, smaller batch of ops), so resetting this per page let the
  // displayed percent visibly drop back down every time a new page's
  // decrypt work began, right after it had just reached 100% on the
  // previous one (confirmed live as a reported bug: hits 100%, then
  // reverts to a lower number). Never letting the displayed value
  // decrease means it holds at 100% through any later page's own lower
  // ratio instead, all the way to the sync's actual completion.
  var decryptLastPercentSent = -1;

  var pullPage = function () {
    // Deliberately NOT passing clientId as excludeClient here (unlike every
    // other wire-format/route detail in this file, that query param was
    // never checked against live traffic - see supersync-client.js's
    // downloadOps). A completed-task change made on another device wasn't
    // coming back down on the next sync; skipping ops by clientId, if the
    // real server's matching semantics differ at all from this guess, is
    // exactly the kind of bug that would produce that symptom silently.
    // Downloading (and re-replaying) this client's own already-applied ops
    // instead is harmless: applyOperations()'s merges are idempotent, and
    // saveLastSeq() after each of our own uploads already keeps us from
    // requesting them again in the first place.
    return client.downloadOps(lastSeq, null, 500).then(function (res) {
      // applyOperations() below is synchronous and, for an E2EE account,
      // can be genuinely slow - each op's decrypt only reuses a cached
      // Argon2id key if it shares a salt with one already seen this
      // session, and other real clients don't necessarily share this
      // client's own salt (see createCrypto's own comment on cost). On a
      // first/full resync of a large account this can take a while with no
      // other status update in between (the percent below is skipped
      // entirely on the last page, since hasMore is already false by
      // then) - without this, that stretch looked identical to a hang
      // (confirmed live: "sits at 99%" on a full-clear resync of a large
      // account). Gated on there actually being ops to decrypt so an
      // empty/no-op page doesn't flicker the status for nothing.
      //
      // Throttled to every 10 percentage points (not every single op) -
      // sendStatus() fires a real AppMessage each call, and a page can hold
      // up to 500 ops; queuing one Bluetooth send per op would build a
      // backlog that delays the eventual "done" status behind it, which is
      // worse than the staleness this is trying to fix.
      if (res.ops && res.ops.length) {
        store.applyOperations(res.ops, state, crypto, function (done, total) {
          var percent = Math.floor(100 * done / total);
          // percent > decryptLastPercentSent (not just the throttle check
          // below) is what keeps this from ever regressing - see
          // decryptLastPercentSent's own comment.
          if (percent > decryptLastPercentSent && (percent >= decryptLastPercentSent + 10 || done === total)) {
            decryptLastPercentSent = percent;
            sendStatus(STATUS_SYNCING, 'Decrypting ' + percent + '%');
          }
        });
      } else {
        store.applyOperations([], state, crypto);
      }
      (res.ops || []).forEach(function (entry) {
        if (entry.op && entry.op.vectorClock) {
          vectorClock = mergeVectorClocks(vectorClock, entry.op.vectorClock);
        }
      });
      // NOT res.latestSeq: confirmed against the real server source
      // (operation-download.service.ts's getOpsSinceWithSeq - `const
      // latestSeq = seqRow?.lastSeq ?? 0`) that field is the ACCOUNT'S
      // overall high-water mark, completely unrelated to pagination
      // position - it is NOT "the seq to resume from". Using it as the
      // next page's sinceSeq was confirmed live (against a real ~3600-op
      // account) to jump straight to the account's current end on the
      // very first page whenever hasMore is true, silently skipping every
      // op in between forever (lastSeq gets persisted at that
      // artificially-inflated value, so the skipped range is never
      // revisited on any later sync either). The correct resume point is
      // the highest serverSeq actually seen in THIS page.
      (res.ops || []).forEach(function (entry) {
        if (entry.serverSeq > lastSeq) {
          lastSeq = entry.serverSeq;
        }
      });
      if (res.hasMore) {
        return pullPage();
      }
    });
  };

  var bootstrapFromSnapshot = function () {
    return client.getRestorePoints(1).then(function (res) {
      var points = res && res.restorePoints;
      if (!points || points.length === 0) {
        return; // Brand-new account with nothing synced yet - not an error.
      }
      return client.restoreSnapshot(points[0].serverSeq).then(function (snapshot) {
        var payload = snapshot && snapshot.encrypted && crypto ? crypto.decrypt(snapshot.payload) : snapshot;
        if (payload && payload.task) {
          state.task = payload.task.entities || payload.task;
        }
        lastSeq = points[0].serverSeq;
      });
    }).catch(function (err) {
      // The snapshot endpoints are best-effort. An E2EE account gets a
      // documented 400 ENCRYPTED_OPS_NOT_SUPPORTED ("Use the client app's
      // Sync Now button to decrypt and restore locally"), but the server has
      // also been seen to 500 on these same requests right after a
      // clear-data resync. The fallback is identical either way and is the
      // actually-supported path for an encrypted account: replay the full op
      // history from lastSeq 0 in pullPage() below. So ANY HTTP-level error
      // here (err.status set) is just a signal to skip straight to that, not
      // a sync failure - only a genuine network/timeout error (no status)
      // propagates.
      if (err && err.status) {
        console.log('[pkjs] snapshot bootstrap unavailable (' + err.message + '); replaying ops from seq 0');
        return;
      }
      throw err;
    });
  };

  var work = (isFirstSync ? bootstrapFromSnapshot() : Promise.resolve())
    .then(function () {
      // Drain any ops queued while offline before pulling - so the pull
      // below already reflects them, and so their order is preserved.
      return flushPendingOps(config);
    })
    .then(function () {
      return pullPage();
    });

  // Returned so callers that trigger a sync as a side effect (e.g.
  // handleTaskToggle's autoSyncOnComplete) can tell once it's actually
  // finished, instead of it racing whatever status message they send next.
  return work
    .then(function () {
      saveState(state);
      saveLastSeq(lastSeq);
      saveVectorClock(vectorClock);
      saveLastSyncedAt(Date.now());
      pushCachedStateToWatch(config);
    })
    .catch(function (err) {
      console.log('[pkjs] sync failed: ' + (err && err.message));
      sendStatus(STATUS_ERROR, err && err.message);
    })
    .then(function () {
      syncInFlight = false;
    });
}

// Uploads exactly one op and normalizes success/failure regardless of HTTP
// status. A rejected op still comes back as an HTTP 200 - the server
// reports per-op acceptance in res.results[].accepted, not the HTTP status
// (confirmed against sync.routes.ops-handler.ts / operation-upload.service.ts:
// a conflict, quota, or duplicate-op-id rejection still calls
// reply.send(...), never reply.status(4xx)). Only checking res.latestSeq
// used to mean an accepted upload and a rejected one were indistinguishable
// from this code's point of view - this was very likely THE actual cause of
// "completed tasks aren't syncing to the desktop app": a real rejection
// (e.g. a vector-clock conflict against a newer desktop-side edit) was
// silently swallowed as success, forever, with nothing to ever retry it.
// Resolves on acceptance (after saving lastSeq); rejects with an Error
// otherwise. On a conflict rejection specifically, the server reports the
// entity's real current vector clock (existingClock) precisely so a client
// can build a dominating clock next time instead of colliding the same way
// again - merged in here even on failure, before rejecting, so the very
// next op against this entity starts from a clock that beats what the
// server actually has.
function uploadSingleOp(op, config, clientId) {
  return uploadOps([op], config, clientId);
}

// Uploads one or more ops in a single request. Rejection of any op throws with
// the first rejected result attached (so the vectorClock-merge recovery below
// still runs); accepted ops in the same batch still land server-side.
function uploadOps(ops, config, clientId) {
  var client = new supersync.SuperSyncClient({ baseUrl: config.baseUrl, token: config.jwt });
  return client
    .uploadOps(ops, clientId, loadLastSeq())
    .then(function (res) {
      var results = (res && res.results) || [];
      var rejected = null;
      for (var i = 0; i < results.length; i++) {
        if (results[i] && !results[i].accepted) {
          rejected = results[i];
          break;
        }
      }
      if (rejected) {
        var err = new Error((rejected.errorCode || 'REJECTED') + ': ' + (rejected.error || 'op rejected by server'));
        err.rejectedResult = rejected;
        throw err;
      }
      // Deliberately NOT saveLastSeq(res.latestSeq) here: confirmed against
      // the real server source that this response's latestSeq is the
      // ACCOUNT'S overall high-water mark (same field/meaning as
      // downloadOps's own latestSeq - see pullPage()'s comment), not "how
      // far this upload's piggybacked res.newOps actually got us". Saving
      // it jumps our cursor to the account's current end without ever
      // downloading/applying anything in between - including res.newOps
      // itself, which this code never even looked at - silently losing any
      // ops from other clients that arrived since our last real sync,
      // every single time this ran. lastSeq now only ever advances via
      // pullPage()'s own per-page-max tracking; runAutoSyncAfterOp's
      // follow-up sync (on by default) picks up whatever res.newOps would
      // have piggybacked, correctly this time.
    })
    .catch(function (err) {
      if (err && err.rejectedResult && err.rejectedResult.existingClock) {
        saveVectorClock(mergeVectorClocks(loadVectorClock(), err.rejectedResult.existingClock));
      }
      // A transport failure - no server response at all (offline), a
      // timeout, or a 5xx - queues these ops for re-upload at the top of
      // the next sync (see lib/op-queue.js and flushPendingOps). A
      // server-side per-op rejection (err.rejectedResult set: conflict,
      // quota, duplicate id, validation) is NOT queued - it won't pass on
      // a retry, and SuperSync's op-log ordering is this client's whole
      // conflict story. A 4xx that isn't per-op (auth, a malformed
      // request) is left for the user to fix by re-pairing, not retried.
      if (err && !err.rejectedResult && (!err.status || err.status >= 500)) {
        opQueue.enqueue(localStorage, ops);
      }
      throw err;
    });
}

// Re-uploads ops queued by an earlier offline session (lib/op-queue.js),
// oldest first, as the first step of every doSync() - before the pull, so
// the watch's offline changes land server-side in the order they happened.
//
// A transport failure (statusless network error, timeout, or 5xx) pauses
// the flush: that op and every later one stay queued, in order, for the
// next attempt. Any real per-op server response - accepted, or rejected for
// a conflict / quota / duplicate - takes the op out of the queue (a
// rejection won't become an acceptance later, and doSync()'s own pull
// reconciles whatever the server kept). A non-per-op 4xx drops just the
// head op so it can't wedge the queue forever, and leaves the rest for next
// time. Always resolves: a flush problem must never mask the sync after it.
function flushPendingOps(config) {
  var queued = opQueue.list(localStorage);
  if (!queued.length) {
    return Promise.resolve();
  }
  var clientId = getOrCreateClientId();
  var client = new supersync.SuperSyncClient({ baseUrl: config.baseUrl, token: config.jwt });
  var index = 0;
  var step = function () {
    if (index >= queued.length) {
      return Promise.resolve();
    }
    var op = queued[index];
    // No .catch on this chain: a thrown error stops step()'s recursion
    // with `op` (and everything after it) still queued - handled below.
    return client.uploadOps([op], clientId, loadLastSeq()).then(function (res) {
      var result = (res && res.results && res.results[0]) || {};
      if (!result.accepted && result.existingClock) {
        // Same recovery uploadOps() does on a conflict - start the next op
        // against this entity from a clock that beats the server's.
        saveVectorClock(mergeVectorClocks(loadVectorClock(), result.existingClock));
      }
      opQueue.remove(localStorage, op.id);
      index++;
      return step();
    });
  };
  return step().catch(function (err) {
    var status = err && err.status;
    if (status && status >= 400 && status < 500 && status !== 408 && status !== 429) {
      var head = queued[index];
      if (head && head.id) {
        opQueue.remove(localStorage, head.id);
      }
      console.log('[pkjs] flushPendingOps: dropped a queued op after HTTP ' + status +
        '; ' + Math.max(0, queued.length - index - 1) + ' still queued');
      return;
    }
    console.log('[pkjs] flushPendingOps: paused with ' + Math.max(0, queued.length - index) +
      ' queued (' + (err && err.message) + ')');
  });
}

// Runs after a single-op upload (success or failure) when the setting is
// on. Uploading only pushes that one op - it doesn't pull whatever else has
// changed server-side, nor re-derive/re-send the watch's own task list
// (which matters once todayOnly or backlog membership makes a just-changed
// task's visibility change). Defaults on since "the watch's change reaches
// the desktop" is the behavior actually being asked for; runs best-effort
// even after a failed upload so at least the pull side stays current,
// matching doSync()'s own error handling.
function runAutoSyncAfterOp(config, failureMsg) {
  if (config.autoSyncOnComplete === false) {
    return;
  }
  var syncPromise = doSync();
  if (failureMsg && syncPromise && typeof syncPromise.then === 'function') {
    // doSync() ends by sending its own STATUS_OK/STATUS_ERROR - without
    // this, an op that was actually rejected would show "Failed: ..." for
    // a moment and then get silently overwritten by the follow-up sync's
    // routine STATUS_OK, hiding the exact failure this whole mechanism
    // exists to surface.
    syncPromise.then(function () {
      sendStatus(STATUS_ERROR, failureMsg);
    });
  }
}

var hideDoneSweepTimerId = null;

// Only relevant when hideDoneTasks is on: getActiveTasks' own grace period
// (HIDE_DONE_GRACE_MS in task-store.js) keeps a just-completed task visible
// for a few seconds so marking it done on the watch doesn't make it vanish
// before the user can see it happen - but nothing else proactively re-
// checks that window once it elapses. Without this, a user who doesn't
// trigger another sync in the meantime (manual Resync, another watch
// action, the periodic timer) would keep seeing the done task indefinitely.
// Coalesces bursts of toggles into a single sweep timed off the LAST one
// rather than one timer per toggle - the sweep re-derives the list from
// whatever's actually in state at fire time, so an earlier toggle's sweep
// being superseded here just avoids a redundant push, not a correctness
// issue.
function scheduleHideDoneSweep(config) {
  if (!config.hideDoneTasks) {
    return;
  }
  if (hideDoneSweepTimerId) {
    clearTimeout(hideDoneSweepTimerId);
  }
  // +100ms padding past the grace period itself - setTimeout only
  // guarantees firing at or after the requested delay, so this is cheap
  // insurance against the sweep landing a hair before getActiveTasks'
  // own >= check would actually exclude the task yet.
  hideDoneSweepTimerId = setTimeout(function () {
    hideDoneSweepTimerId = null;
    var config2 = loadConfig();
    if (!config2 || !config2.hideDoneTasks) {
      return; // Setting was turned off while this sweep was pending.
    }
    var tasks = watchTaskList(loadState(), Object.assign({}, config2, { hideDoneTasks: true }));
    sendTaskListToWatch(tasks);
  }, store.HIDE_DONE_GRACE_MS + 100);
}

// Builds one "[Task Shared] updateTask" op - the shape task-store.js's
// applyTaskAction() expects on the way back down (matches the real wire
// shape confirmed by decrypting live ops: decrypted payload is
// { actionPayload: { task: { id, changes } } }, task-shared.actions.ts).
// entityChanges is required (even empty) for the real client's own
// isMultiEntityPayload() guard (operation.types.ts) - without it,
// extractActionPayload() never unwraps actionPayload, so the replayed
// action ends up with a top-level `actionPayload` key instead of `task`,
// and the reducer destructures `task` as undefined. vectorClock is
// REQUIRED by the real server (validation.service.ts's
// sanitizeVectorClock(), called from validateOp()) - a missing/non-object
// one fails validation outright and the whole op is rejected before it's
// ever stored; this was the actual cause of watch-completed tasks never
// reaching any other client, desktop included. Saved immediately (not
// gated on upload success) since the local change already causally
// happened whether or not this particular upload attempt succeeds - see
// loadVectorClock's own comment. Shared by handleTaskToggle's own toggle
// and its parent-auto-complete follow-up below, so both get a causally
// distinct, correctly-incremented clock rather than colliding on the same
// one.
function buildTaskUpdateOp(taskId, changes, clientId) {
  var crypto = getCrypto();
  var payload = { actionPayload: { task: { id: taskId, changes: changes } }, entityChanges: [] };
  var newVectorClock = incrementVectorClock(loadVectorClock(), clientId);
  saveVectorClock(newVectorClock);
  return {
    id: generateOpId(),
    opType: 'UPD',
    actionType: '[Task Shared] updateTask',
    entityType: 'TASK',
    entityId: taskId,
    payload: crypto ? crypto.encrypt(payload) : payload,
    isPayloadEncrypted: !!crypto,
    vectorClock: newVectorClock,
    clientId: clientId,
    timestamp: Date.now(),
    schemaVersion: SCHEMA_VERSION,
  };
}

// "[Project] Move Task from backlog to regular" (project.actions.ts, but a TASK-
// entity op - payload { taskId, afterTaskId, workContextId }, confirmed against
// the shape task-store's replay and its tests use). workContextId is the
// project id; afterTaskId null drops it at the top of the regular list.
function buildBacklogToRegularOp(projectId, taskId, clientId) {
  var crypto = getCrypto();
  var payload = {
    actionPayload: { taskId: taskId, afterTaskId: null, workContextId: projectId },
    entityChanges: [],
  };
  var newVectorClock = incrementVectorClock(loadVectorClock(), clientId);
  saveVectorClock(newVectorClock);
  return {
    id: generateOpId(),
    opType: 'UPD',
    actionType: '[Project] Move Task from backlog to regular',
    entityType: 'TASK',
    entityId: taskId,
    payload: crypto ? crypto.encrypt(payload) : payload,
    isPayloadEncrypted: !!crypto,
    vectorClock: newVectorClock,
    clientId: clientId,
    timestamp: Date.now(),
    schemaVersion: SCHEMA_VERSION,
  };
}

// Builds (and applies the matching local optimistic update for) the
// parent's own updateTask op if taskId's just-applied completion left
// every sibling subtask done too - see handleTaskToggle's own call site
// for the full reasoning (mirrors the real app's
// TaskInternalEffects.onAllSubTasksDone$ effect). Returns null if the
// setting's off, taskId isn't a subtask, the parent's missing or already
// done, or a sibling's still open - i.e. whenever there's nothing to do.
function buildParentAutoCompleteOp(taskId, state, config, clientId) {
  if (!config.autoMarkParentDone) {
    return null;
  }
  var task = state.task[taskId];
  if (!task || !task.parentId) {
    return null;
  }
  var parent = state.task[task.parentId];
  if (!parent || parent.isDone) {
    return null;
  }
  var allSubtasksDone = (parent.subTaskIds || []).every(function (subId) {
    var sub = state.task[subId];
    return sub && sub.isDone;
  });
  if (!allSubtasksDone) {
    return null;
  }
  // Same doneOn-stamping reasoning as handleTaskToggle's own toggle -
  // getActiveTasks' hideDone grace period needs an accurate completion
  // timestamp, not just isDone itself.
  state.task[parent.id] = Object.assign({}, parent, { isDone: true, doneOn: Date.now() });
  saveState(state);
  return buildTaskUpdateOp(parent.id, { isDone: true }, clientId);
}

function handleTaskToggle(taskId, done) {
  var config = loadConfig();
  if (!config || !config.jwt) {
    sendStatus(STATUS_NOT_PAIRED);
    return;
  }

  var state = loadState();
  // doneOn mirrors the real reducer's own side effect (task.reducer.util.ts:
  // marking a task done stamps doneOn with Date.now() when the caller
  // doesn't supply one, and clears it back to undefined on undone -
  // task.service.ts's own toggle call never supplies one either, so this is
  // exactly what a real client's local state would show right after this
  // exact toggle). Needed here specifically so getActiveTasks' hideDone
  // grace period (see HIDE_DONE_GRACE_MS in task-store.js) has an accurate
  // completion timestamp to measure from - not uploaded as part of the op
  // payload, matching the real app's own action shape.
  state.task[taskId] = Object.assign({}, state.task[taskId], { isDone: done, doneOn: done ? Date.now() : undefined });
  saveState(state);
  if (done) {
    scheduleHideDoneSweep(config);
  }

  var clientId = getOrCreateClientId();
  var op = buildTaskUpdateOp(taskId, { isDone: done }, clientId);
  // Mirrors the real app's own TaskInternalEffects.onAllSubTasksDone$
  // effect (task-internal.effects.ts): completing a subtask that leaves
  // every sibling done also completes the parent, as a SECOND updateTask
  // op rather than folded into this one - the exact same shape a real
  // client's own effect would have produced, so a desktop client
  // replaying this watch-originated pair sees nothing different from
  // completing it there instead. Gated on config.autoMarkParentDone
  // (mirrors tasksCfg.isAutoMarkParentAsDone, also off by default there)
  // since this silently changes a second task beyond the one the user
  // actually acted on. Only attempted when done is true (never on undo) -
  // the real effect only reacts to a subtask's own isDone turning true,
  // and never reopens an already-completed parent when one's undone
  // later.
  var parentOp = done ? buildParentAutoCompleteOp(taskId, state, config, clientId) : null;
  if (parentOp) {
    // Without this, the parent's completion was invisible on the watch
    // until the NEXT list push - which, with hideDoneTasks on, could
    // easily be scheduleHideDoneSweep's own sweep (fired after
    // HIDE_DONE_GRACE_MS specifically to exclude anything whose grace
    // period has elapsed). The parent's doneOn is stamped within the same
    // synchronous call as the subtask's own, so by the time that sweep
    // fires its grace period has ALSO already elapsed - the parent would
    // vanish from the list having never once been shown as done, unlike
    // the subtask itself (whose "Done" flash comes from the watch's own
    // local optimistic toggle on that same row, independent of any push -
    // main.c's menu_select_click - which the parent has no equivalent of,
    // since the watch has no idea a second, different row just changed).
    // Pushed while still well within the grace window, so hideDoneTasks
    // doesn't exclude it from this particular push either.
    sendTaskListToWatch(watchTaskList(state, config));
  }

  var uploads = [uploadSingleOp(op, config, clientId)];
  if (parentOp) {
    uploads.push(uploadSingleOp(parentOp, config, clientId));
  }

  var toggleFailureMsg = null;
  Promise.all(uploads)
    .catch(function (err) {
      // Log and leave the local optimistic update(s) in place. uploadOps()
      // has already queued the op(s) if this was a transport failure, so
      // the next sync's flushPendingOps() re-sends them; a hard server
      // rejection isn't queued and the next full pull reconciles it.
      toggleFailureMsg = (err && err.message) || 'upload failed, will retry next sync';
      console.log('[pkjs] failed to upload task toggle: ' + toggleFailureMsg);
      sendStatus(STATUS_ERROR, toggleFailureMsg);
    })
    .then(function () {
      runAutoSyncAfterOp(config, toggleFailureMsg);
    });
}

// Watch long-press Up / Down (or swipe-left) after a 3s cancel window. `when`:
// 'today' / 'tomorrow' set the task's dueDay to that day, anything else
// ('unschedule') clears its scheduling entirely. All go out as a single
// "[Task Shared] updateTask" op - the watch has no planner-list UI, and dueDay
// is the field every "today" membership check keys off (task-store.js).
// dueWithTime/remindAt are cleared alongside either way, mirroring the real
// app's own scheduleTask/unschedule reducers (dueDay and dueWithTime are
// mutually exclusive there).
// projectId is sent only when the gesture came from the Projects browser's
// task view - used to pull the task out of that project's backlog (a scheduled
// task belongs in the regular list) and to re-push that view.
function handleTaskReschedule(taskId, when, projectId) {
  var config = loadConfig();
  if (!config || !config.jwt) {
    sendStatus(STATUS_NOT_PAIRED);
    return;
  }
  var state = loadState();
  var task = state.task[taskId];
  if (!task) {
    // Deleted elsewhere between the list push and the gesture - nothing to do.
    return;
  }

  var newDueDay = null;
  if (when === 'today') {
    newDueDay = store.todayStr();
  } else if (when === 'tomorrow') {
    var d = new Date();
    d.setDate(d.getDate() + 1);
    newDueDay = store.dateToDateStr(d);
  }
  // null (not undefined) so the cleared fields survive JSON.stringify onto the
  // wire and actually clear on other clients, not just locally.
  var changes = { dueDay: newDueDay, dueWithTime: null, remindAt: null };
  state.task[taskId] = Object.assign({}, task, changes);

  var clientId = getOrCreateClientId();
  var ops = [buildTaskUpdateOp(taskId, changes, clientId)];

  // Scheduling a task that's sitting in its project's backlog moves it into the
  // regular list, both here and on other clients ([Project] Move Task from
  // backlog to regular - the same action the desktop fires, which task-store's
  // replay already handles).
  if (projectId && newDueDay && task.__inBacklog) {
    state.task[taskId].__inBacklog = false;
    ops.push(buildBacklogToRegularOp(String(projectId), taskId, clientId));
  }
  saveState(state);

  // Push a fresh list right away rather than waiting for the follow-up sync
  // (same as archive). From the browser it's that project's task list, so the
  // row jumps out of the backlog section there; the today list catches up on
  // runAutoSyncAfterOp's follow-up sync. Two chunked sends back to back would
  // fight over the one AppMessage slot, so it's one or the other.
  if (projectId) {
    sendProjectTasks(String(projectId), state, config);
  } else {
    sendTaskListToWatch(watchTaskList(state, config));
  }

  var failureMsg = null;
  uploadOps(ops, config, clientId)
    .catch(function (err) {
      failureMsg = (err && err.message) || 'upload failed, will retry next sync';
      console.log('[pkjs] failed to upload task reschedule: ' + failureMsg);
      sendStatus(STATUS_ERROR, failureMsg);
    })
    .then(function () {
      runAutoSyncAfterOp(config, failureMsg);
    });
}

// Phase 2: the watch started tracking `taskId` (elapsedMs > 0 for a session
// resumed on relaunch). Broadcast it as this watch's own presence so other
// devices see "Tracking on Pebble". A no-op on the op-log - time still only
// syncs on stop (handleTrackTimeStop). Stored even when the socket isn't up
// yet; applyPresence flushes it on connect.
function handleTrackStart(taskId, elapsedMs) {
  if (!taskId) {
    return;
  }
  presenceBroadcasting = { taskId: String(taskId), sinceTs: Date.now() - (elapsedMs || 0) };
  if (presenceClient) {
    presenceClient.broadcastTracking(presenceBroadcasting.taskId, presenceBroadcasting.sinceTs);
  }
  // Re-push the task list so the now-tracked task is force-included in it
  // (watchTaskList). Without this a task tracked from the Projects browser
  // that isn't on today's list never reaches the watch's s_tasks, so its
  // pinned "TRACKING" section stays empty while the watch still believes it
  // is tracking - which pushes a remote presence session onto the old
  // dark-blue LIVE row instead of the pinned section. The watch can't do this
  // itself: start_tracking already sends MSG_TRACK_TIME_START, and a second
  // back-to-back send (a sync request) collides on its single outbox slot.
  var config = loadConfig();
  if (config && config.jwt) {
    sendTaskListToWatch(watchTaskList(loadState(), config));
  }
}

// Phase 2: the watch stopped tracking - end the "Tracking on Pebble" broadcast.
// Separate from the op upload in handleTrackTimeStop so it still fires for a
// sub-second session that carries no time delta.
function handleTrackStopBroadcast() {
  if (presenceClient) {
    presenceClient.broadcastStopped();
  }
  presenceBroadcasting = null;
}

function handleTrackTimeStop(taskId, trackedMs) {
  var config = loadConfig();
  if (!config || !config.jwt) {
    sendStatus(STATUS_NOT_PAIRED);
    return;
  }
  if (!taskId || !trackedMs || trackedMs <= 0) {
    return;
  }

  var today = store.todayStr();
  // Deliberately NOT an optimistic local bump here, unlike
  // handleTaskToggle's isDone set. That bump is a plain replace (applying
  // it twice is harmless), but time-tracking is additive - bumping
  // timeSpentOnDay here, before upload, guarantees the same delta gets
  // counted a SECOND time moments later when this exact op comes back down
  // from the server and gets replayed by the normal additive-merge path
  // (task-store.js's '[TimeTracking] Sync time spent' case). And if the
  // upload never succeeds, the bump is never reverted either - it just
  // strands there permanently, since nothing here tracks "this local delta
  // is still unconfirmed." Both were real: found via live data showing a
  // task's watch-displayed total far exceeding the sum of everything the
  // server had actually ever accepted from it. The watch's own C-side
  // optimistic bump (stop_tracking_and_report) is safe by comparison - it
  // gets wholesale-replaced (not additively merged) by the next
  // TASK_SYNC_END, which runAutoSyncAfterOp's follow-up doSync() below
  // already triggers within moments, so skipping the equivalent bump here
  // only costs a brief instant of staleness in the phone's own cache, never
  // permanent drift.
  var crypto = getCrypto();
  // Confirmed against time-tracking.actions.ts's syncTimeSpent action
  // creator: the payload is exactly { taskId, date, duration } - duration
  // is the DELTA in ms for that calendar day, not a cumulative total and
  // not the full timeSpentOnDay map (see validation.service.ts's
  // TASK_TIME_DELTA_ACTION_TYPE check, which validates precisely this
  // shape for the unencrypted case).
  // entityChanges required for the same isMultiEntityPayload() unwrap reason
  // as handleTaskToggle's payload above. syncTimeSpent's reducer
  // (task.reducer.ts) reads action.taskId/date/duration directly, never
  // entityChanges - the real client only populates non-empty entityChanges
  // for this action type to help conflict-resolution's disjoint-field merge
  // (conflict-disjoint-merge.util.ts) attribute a concurrent-edit delta to
  // this field; omitting it here only affects that merge-conflict edge case
  // (falls back to whole-op LWW), not basic replay.
  var payload = { actionPayload: { taskId: taskId, date: today, duration: trackedMs }, entityChanges: [] };
  var clientId = getOrCreateClientId();
  var newVectorClock = incrementVectorClock(loadVectorClock(), clientId);
  saveVectorClock(newVectorClock);
  var op = {
    id: generateOpId(),
    opType: 'UPD',
    actionType: '[TimeTracking] Sync time spent',
    entityType: 'TASK',
    entityId: taskId,
    payload: crypto ? crypto.encrypt(payload) : payload,
    isPayloadEncrypted: !!crypto,
    vectorClock: newVectorClock,
    clientId: clientId,
    timestamp: Date.now(),
    schemaVersion: SCHEMA_VERSION,
  };

  var trackFailureMsg = null;
  uploadSingleOp(op, config, clientId)
    .catch(function (err) {
      trackFailureMsg = (err && err.message) || 'upload failed, will retry next sync';
      console.log('[pkjs] failed to upload tracked time: ' + trackFailureMsg);
      sendStatus(STATUS_ERROR, trackFailureMsg);
    })
    .then(function () {
      runAutoSyncAfterOp(config, trackFailureMsg);
    });
}

// delta is +1 (Select) or -1 (long-select). Applied against whatever this
// phone's own cache currently has for today's count, not the watch's own
// (possibly stale) value - the watch sends a direction, not a target.
function handleHabitAdjust(habitId, delta) {
  var config = loadConfig();
  if (!config || !config.jwt) {
    sendStatus(STATUS_NOT_PAIRED);
    return;
  }
  if (config.enableHabits === false) {
    // Defensive backstop against a stale watch UI still showing the Habits
    // row/window for one message round-trip right after it's disabled - the
    // row itself should already be hidden by then (see sendStatus), but
    // don't act on a habit adjustment that shouldn't be reachable.
    return;
  }

  var today = store.todayStr();
  var state = loadState();
  var counter = state.simpleCounter[habitId];
  if (!counter) {
    return;
  }
  var currentVal = (counter.countOnDay && counter.countOnDay[today]) || 0;
  var newVal = Math.max(0, currentVal + delta);

  // Safe to apply optimistically here, unlike handleTrackTimeStop's
  // additive delta - this mirrors handleTaskToggle's isDone set: a plain
  // replace of countOnDay[today] (confirmed against the real reducer's own
  // setSimpleCounterCounterToday case, Math.max(0, newVal) - not additive),
  // so re-applying the same value again when this op comes back down
  // through the normal replay path is harmless, not a source of drift.
  var countOnDay = Object.assign({}, counter.countOnDay);
  countOnDay[today] = newVal;
  state.simpleCounter[habitId] = Object.assign({}, counter, { countOnDay: countOnDay });
  saveState(state);

  var crypto = getCrypto();
  // Matches setSimpleCounterCounterToday's action payload exactly:
  // { id, newVal, today } (simple-counter.actions.ts) - a flat top-level
  // shape, not wrapped in a nested entity object the way updateTask's is.
  // entityChanges required for the same isMultiEntityPayload() unwrap reason
  // as handleTaskToggle's payload above.
  var payload = { actionPayload: { id: habitId, newVal: newVal, today: today }, entityChanges: [] };
  var clientId = getOrCreateClientId();
  var newVectorClock = incrementVectorClock(loadVectorClock(), clientId);
  saveVectorClock(newVectorClock);
  var op = {
    id: generateOpId(),
    opType: 'UPD',
    actionType: '[SimpleCounter] Set SimpleCounter Counter Today',
    entityType: 'SIMPLE_COUNTER',
    entityId: habitId,
    payload: crypto ? crypto.encrypt(payload) : payload,
    isPayloadEncrypted: !!crypto,
    vectorClock: newVectorClock,
    clientId: clientId,
    timestamp: Date.now(),
    schemaVersion: SCHEMA_VERSION,
  };

  var habitFailureMsg = null;
  uploadSingleOp(op, config, clientId)
    .catch(function (err) {
      habitFailureMsg = (err && err.message) || 'upload failed, will retry next sync';
      console.log('[pkjs] failed to upload habit adjustment: ' + habitFailureMsg);
      sendStatus(STATUS_ERROR, habitFailureMsg);
    })
    .then(function () {
      runAutoSyncAfterOp(config, habitFailureMsg);
    });
}

// Long-select time tracking for a StopWatch-type habit - modeled directly on
// handleTrackTimeStop (same additive-delta reasoning: syncSimpleCounterTime
// applies duration on top of whatever's already there server-side, so NOT
// bumping state.simpleCounter here optimistically avoids the exact double-
// count drift handleTrackTimeStop's own comment documents for tasks. The
// watch's own C-side optimistic bump (stop_habit_tracking_and_report) gives
// instant feedback and gets wholesale-replaced by the next
// MSG_HABIT_SYNC_END, which runAutoSyncAfterOp's follow-up doSync() below
// triggers moments later by default.
function handleHabitTrackStop(habitId, trackedMs) {
  var config = loadConfig();
  if (!config || !config.jwt) {
    sendStatus(STATUS_NOT_PAIRED);
    return;
  }
  if (config.enableHabits === false) {
    return;
  }
  if (!habitId || !trackedMs || trackedMs <= 0) {
    return;
  }

  var today = store.todayStr();
  var crypto = getCrypto();
  // Matches syncSimpleCounterTime's action payload exactly: { id, date,
  // duration } (simple-counter.actions.ts) - same shape as syncTimeSpent's
  // { taskId, date, duration }, just id instead of taskId.
  var payload = { actionPayload: { id: habitId, date: today, duration: trackedMs }, entityChanges: [] };
  var clientId = getOrCreateClientId();
  var newVectorClock = incrementVectorClock(loadVectorClock(), clientId);
  saveVectorClock(newVectorClock);
  var op = {
    id: generateOpId(),
    opType: 'UPD',
    actionType: '[SimpleCounter] Sync counter time',
    entityType: 'SIMPLE_COUNTER',
    entityId: habitId,
    payload: crypto ? crypto.encrypt(payload) : payload,
    isPayloadEncrypted: !!crypto,
    vectorClock: newVectorClock,
    clientId: clientId,
    timestamp: Date.now(),
    schemaVersion: SCHEMA_VERSION,
  };

  var trackFailureMsg = null;
  uploadSingleOp(op, config, clientId)
    .catch(function (err) {
      trackFailureMsg = (err && err.message) || 'upload failed, will retry next sync';
      console.log('[pkjs] failed to upload habit tracked time: ' + trackFailureMsg);
      sendStatus(STATUS_ERROR, trackFailureMsg);
    })
    .then(function () {
      runAutoSyncAfterOp(config, trackFailureMsg);
    });
}

// Watch-dictated "Add Task" - the only watch-initiated write that creates a
// brand-new entity rather than toggling/adjusting an existing one.
// config.defaultProjectId is a setting local to this app's own pairing page
// (not a read of the real app's own GlobalConfig.tasks.defaultProjectId) -
// falls back to Super Productivity's built-in Inbox project when unset.
function handleAddTask(title) {
  if (!title || !String(title).trim()) {
    // Defensive - the watch's own dictation confirmation UI shouldn't ever
    // hand back an empty string, but don't trust that blindly.
    return;
  }
  var config = loadConfig();
  if (!config || !config.jwt) {
    sendStatus(STATUS_NOT_PAIRED);
    return;
  }
  if (config.enableAddTask === false) {
    // Defensive backstop against a stale watch UI still showing the Add
    // Task row for one message round-trip right after it's disabled - the
    // row itself should already be hidden by then (see sendStatus).
    return;
  }

  var projectId = config.defaultProjectId || 'INBOX_PROJECT';
  var task = {
    id: generateTaskId(),
    subTaskIds: [],
    timeSpentOnDay: {},
    timeSpent: 0,
    // Default time estimate from this app's pairing page (minutes -> ms); 0
    // when the setting is "None", matching a desktop-added task with no
    // estimate typed.
    timeEstimate: (config.defaultTaskEstimateMin || 0) * 60000,
    isDone: false,
    title: String(title).trim(),
    tagIds: [],
    created: Date.now(),
    attachments: [],
    projectId: projectId,
    // Without a due date, a task the user is dictating onto the watch right
    // now would be silently invisible on the watch itself whenever Today
    // Only is on (the same, correct, filter a desktop task with no due date
    // is subject to on the real Today page) - mirrors what the real app's
    // own createNewTaskWithDefaults does when a task is added while viewing
    // the Today page (workContextId === TODAY_TAG.id: dueDay defaults to
    // today). There's no equivalent "page" concept for a watch-initiated
    // add, but the intent is the same: you're adding this to work on now.
    dueDay: store.todayStr(),
  };

  // Matches addTask's real action payload shape (task-shared.actions.ts):
  // { task, workContextId, workContextType, isAddToBacklog, isAddToBottom }.
  // workContextType is unconditionally 'PROJECT' here - both the configured
  // default project and the Inbox fallback are Project entities, never a
  // Tag, so there's no tag-context case to handle. isIgnoreShortSyntax is
  // set defensively (op-log replay doesn't actually re-dispatch through the
  // real app's short-syntax Effect, so this isn't currently load-bearing,
  // but costs nothing and guards against that changing later).
  // entityChanges required for the same isMultiEntityPayload() unwrap reason
  // as handleTaskToggle's payload above - without it, the real desktop
  // client's extractActionPayload() never unwraps actionPayload, so
  // action.task is undefined and the addTask reducer throws a TypeError
  // reading task.dueDay, silently dropping the op (this was the actual
  // cause of dictated tasks never appearing on desktop, confirmed live via
  // a "Skipped N reducer-failed operation(s)" console log after dictating a
  // task - see git history/commit message for the fuller trace).
  var payload = {
    actionPayload: {
      task: task,
      workContextId: projectId,
      workContextType: 'PROJECT',
      isAddToBacklog: false,
      isAddToBottom: false,
      isIgnoreShortSyntax: true,
    },
    entityChanges: [],
  };

  // Optimistic local update via the SAME replay path a real synced addTask
  // op goes through (task-store.js's applyTaskAction 'addTask' case), not a
  // hand-rolled merge - so this can't drift from what replay actually does.
  var state = loadState();
  store.applyOperations(
    [{ op: { entityType: 'TASK', actionType: '[Task Shared] addTask', opType: 'CRT', payload: payload, isPayloadEncrypted: false } }],
    state
  );
  saveState(state);

  // Unlike toggle/habit-adjust, the watch has no way to render a task it's
  // never seen on its own - push the updated list right away rather than
  // waiting for uploadSingleOp/runAutoSyncAfterOp's follow-up sync to
  // eventually get around to it.
  sendTaskListToWatch(watchTaskList(state, config));

  var crypto = getCrypto();
  var clientId = getOrCreateClientId();
  var newVectorClock = incrementVectorClock(loadVectorClock(), clientId);
  saveVectorClock(newVectorClock);
  var op = {
    id: generateOpId(),
    opType: 'CRT',
    actionType: '[Task Shared] addTask',
    entityType: 'TASK',
    entityId: task.id,
    payload: crypto ? crypto.encrypt(payload) : payload,
    isPayloadEncrypted: !!crypto,
    vectorClock: newVectorClock,
    clientId: clientId,
    timestamp: Date.now(),
    schemaVersion: SCHEMA_VERSION,
  };

  var addTaskFailureMsg = null;
  uploadSingleOp(op, config, clientId)
    .catch(function (err) {
      addTaskFailureMsg = (err && err.message) || 'upload failed, will retry next sync';
      console.log('[pkjs] failed to upload new task: ' + addTaskFailureMsg);
      sendStatus(STATUS_ERROR, addTaskFailureMsg);
    })
    .then(function () {
      runAutoSyncAfterOp(config, addTaskFailureMsg);
    });
}

// A chunk boundary that never splits a UTF-16 surrogate pair (an astral
// character, e.g. most emoji, encoded as two UTF-16 code units) across two
// chunks - a naive fixed-offset slice() could otherwise land exactly
// between the high and low surrogate, corrupting that one character on
// reassembly. Backs the boundary off by one code unit in that case; the
// half-unit of slack this costs is irrelevant against NOTE_CHUNK_LEN.
function safeChunkEnd(str, desiredEnd) {
  if (desiredEnd < str.length) {
    var code = str.charCodeAt(desiredEnd - 1);
    if (code >= 0xD800 && code <= 0xDBFF) {
      return desiredEnd - 1;
    }
  }
  return desiredEnd;
}

// Sends one chunk of a full-notes transfer and recurses to the next once
// the phone confirms delivery (same one-in-flight-at-a-time discipline as
// sendTaskAt/sendHabitAt - see sendWithRetry's own comment for why this
// matters more than a one-off send). offset >= notes.length (including the
// initial call for an empty note) sends MSG_NOTE_SYNC_END and stops.
function sendNoteChunk(taskId, notes, offset) {
  if (offset >= notes.length) {
    sendWithRetry({ MSG_TYPE: MSG_NOTE_SYNC_END, TASK_ID: taskId }, function () {}, function (e) {
      console.log('[pkjs] giving up on NOTE_SYNC_END for ' + taskId + ' after retries: ' + JSON.stringify(e));
    });
    return;
  }
  var end = safeChunkEnd(notes, Math.min(offset + NOTE_CHUNK_LEN, notes.length));
  var dict = { MSG_TYPE: MSG_NOTE_CHUNK, TASK_ID: taskId, NOTE_CHUNK_TEXT: notes.slice(offset, end) };
  sendWithRetry(dict, function () {
    sendNoteChunk(taskId, notes, end);
  }, function (e) {
    console.log('[pkjs] giving up on NOTE_CHUNK at offset ' + offset + ' for ' + taskId + ' after retries: ' + JSON.stringify(e));
  });
}

// Answers MSG_NOTE_REQUEST (see main.c's request_notes_full) and also
// re-triggers itself after a note-append (see handleNoteAppend below) so
// an open overlay picks up the appended text without a full list resync.
// NOTE_TOTAL_LEN lets the watch malloc exactly once for the whole transfer
// instead of growing a buffer chunk by chunk.
function sendFullNotesForTask(taskId) {
  var state = loadState();
  var task = state.task[taskId];
  var notes = (task && task.notes) ? String(task.notes) : '';
  // NOTE_TOTAL_LEN has to be the actual UTF-8 BYTE count AppMessage will
  // put on the wire (what main.c mallocs against), not notes.length - that
  // counts UTF-16 code units, which undercounts any non-ASCII character
  // (accented letters, symbols, emoji all encode to more UTF-8 bytes than
  // UTF-16 units). Confirmed live: a note with a few emoji sent a
  // NOTE_TOTAL_LEN several bytes short of what actually arrived, and the
  // watch's malloc'd buffer being that many bytes too small made its own
  // overflow guard silently drop every chunk once the shortfall was used
  // up - the note just went truncated with no error anywhere.
  var totalBytes = sha256lib.utf8ToBytes(notes).length;
  var startDict = { MSG_TYPE: MSG_NOTE_SYNC_START, TASK_ID: taskId, NOTE_TOTAL_LEN: totalBytes };
  sendWithRetry(startDict, function () {
    sendNoteChunk(taskId, notes, 0);
  }, function (e) {
    console.log('[pkjs] giving up on NOTE_SYNC_START for ' + taskId + ' after retries: ' + JSON.stringify(e));
  });
}

function handleNoteRequest(taskId) {
  if (!taskId) {
    return;
  }
  sendFullNotesForTask(taskId);
}

// Watch-dictated note append - long-select on the notes overlay (see
// MSG_NOTE_APPEND in main.c/start_note_append_dictation). notes is a plain
// string field, so this - like handleTaskToggle's isDone set, unlike
// handleTrackTimeStop's additive delta - is a plain replace: applying the
// same computed newNotes value twice (once optimistically here, once again
// when this same op is later downloaded and replayed) is harmless, so the
// optimistic local update below is safe.
function handleNoteAppend(taskId, noteText) {
  if (!taskId || !noteText || !String(noteText).trim()) {
    return;
  }
  var config = loadConfig();
  if (!config || !config.jwt) {
    sendStatus(STATUS_NOT_PAIRED);
    return;
  }

  var state = loadState();
  var task = state.task[taskId];
  if (!task) {
    // This phone's own state.task cache has no record of the task the watch
    // thinks it's appending to (e.g. deleted elsewhere between opening the
    // notes overlay and finishing dictation) - nothing sensible to append.
    return;
  }

  var existingNotes = task.notes || '';
  var dictated = String(noteText).trim();
  var newNotes = existingNotes ? existingNotes + '\n\n' + NOTE_APPEND_DIVIDER + '\n\n' + dictated : dictated;

  state.task[taskId] = Object.assign({}, task, { notes: newNotes });
  saveState(state);

  // Re-sends this one task's full notes right away (same fetch
  // MSG_NOTE_REQUEST triggers - see sendFullNotesForTask) rather than a
  // whole-list resync: main.c matches the reply against whichever task's
  // overlay is currently open and re-renders in place, so the user sees
  // their dictated text appended without closing and reopening the
  // overlay. A no-op on the watch if the overlay's since moved to a
  // different task (or closed) - see MSG_NOTE_SYNC_START's id check there.
  sendFullNotesForTask(taskId);

  var crypto = getCrypto();
  // Same "[Task Shared] updateTask" / { id, changes } shape as
  // handleTaskToggle's payload above, entityChanges required for the same
  // isMultiEntityPayload() unwrap reason documented there.
  var payload = { actionPayload: { task: { id: taskId, changes: { notes: newNotes } } }, entityChanges: [] };
  var clientId = getOrCreateClientId();
  var newVectorClock = incrementVectorClock(loadVectorClock(), clientId);
  saveVectorClock(newVectorClock);
  var op = {
    id: generateOpId(),
    opType: 'UPD',
    actionType: '[Task Shared] updateTask',
    entityType: 'TASK',
    entityId: taskId,
    payload: crypto ? crypto.encrypt(payload) : payload,
    isPayloadEncrypted: !!crypto,
    vectorClock: newVectorClock,
    clientId: clientId,
    timestamp: Date.now(),
    schemaVersion: SCHEMA_VERSION,
  };

  var noteFailureMsg = null;
  uploadSingleOp(op, config, clientId)
    .catch(function (err) {
      noteFailureMsg = (err && err.message) || 'upload failed, will retry next sync';
      console.log('[pkjs] failed to upload note append: ' + noteFailureMsg);
      sendStatus(STATUS_ERROR, noteFailureMsg);
    })
    .then(function () {
      runAutoSyncAfterOp(config, noteFailureMsg);
    });
}

// The real app has no single "project notes" field - a project has a LIST
// of separate Note entities (project.noteIds, entityType 'NOTE' - see
// task-store.js's applyNoteAction). The watch's project row (shown when
// grouping is on) treats a project's OLDEST Note (by `created`) as the one
// synthetic "project note" it shows/appends to - same one-note shape
// task.notes already has - rather than exposing a whole list the watch has
// no UI for. Returns undefined if the project has no notes yet.
function firstNoteForProject(state, projectId) {
  var notes = state.note || {};
  var best = null;
  Object.keys(notes).forEach(function (id) {
    var n = notes[id];
    if (n && n.projectId === projectId && (!best || (n.created || 0) < (best.created || 0))) {
      best = n;
    }
  });
  return best;
}

// Mirrors sendFullNotesForTask above, for the project's own synthetic note.
function sendFullNotesForProject(projectId) {
  var state = loadState();
  var note = firstNoteForProject(state, projectId);
  var notes = (note && note.content) ? String(note.content) : '';
  var totalBytes = sha256lib.utf8ToBytes(notes).length;
  var startDict = { MSG_TYPE: MSG_PROJECT_NOTE_SYNC_START, PROJECT_ID: projectId, NOTE_TOTAL_LEN: totalBytes };
  sendWithRetry(startDict, function () {
    sendProjectNoteChunk(projectId, notes, 0);
  }, function (e) {
    console.log('[pkjs] giving up on PROJECT_NOTE_SYNC_START for ' + projectId + ' after retries: ' + JSON.stringify(e));
  });
}

// Mirrors sendNoteChunk above, for a project's synthetic note.
function sendProjectNoteChunk(projectId, notes, offset) {
  if (offset >= notes.length) {
    sendWithRetry({ MSG_TYPE: MSG_PROJECT_NOTE_SYNC_END, PROJECT_ID: projectId }, function () {}, function (e) {
      console.log('[pkjs] giving up on PROJECT_NOTE_SYNC_END for ' + projectId + ' after retries: ' + JSON.stringify(e));
    });
    return;
  }
  var end = safeChunkEnd(notes, Math.min(offset + NOTE_CHUNK_LEN, notes.length));
  var dict = { MSG_TYPE: MSG_PROJECT_NOTE_CHUNK, PROJECT_ID: projectId, NOTE_CHUNK_TEXT: notes.slice(offset, end) };
  sendWithRetry(dict, function () {
    sendProjectNoteChunk(projectId, notes, end);
  }, function (e) {
    console.log('[pkjs] giving up on PROJECT_NOTE_CHUNK at offset ' + offset + ' for ' + projectId + ' after retries: ' + JSON.stringify(e));
  });
}

function handleProjectNoteRequest(projectId) {
  if (!projectId) {
    return;
  }
  sendFullNotesForProject(projectId);
}

// Watch-dictated project note append - long-select on the project notes
// overlay (see MSG_PROJECT_NOTE_APPEND in main.c/start_note_append_dictation).
// Unlike handleNoteAppend (always updating an existing task.notes string),
// a project may not have a synthetic note yet at all - the first append
// creates one ('[Note] Add Note'); a later append updates its content
// ('[Note] Update Note'), same plain-replace safety as handleNoteAppend's
// own comment (applying the same computed newContent twice, once
// optimistically here and again on replay, is harmless either way).
function handleProjectNoteAppend(projectId, noteText) {
  if (!projectId || !noteText || !String(noteText).trim()) {
    return;
  }
  var config = loadConfig();
  if (!config || !config.jwt) {
    sendStatus(STATUS_NOT_PAIRED);
    return;
  }

  var state = loadState();
  // A cache saved before this feature existed has no `note` key at all
  // (loadState() is a raw JSON.parse of whatever's cached, not merged
  // against store.emptyState()'s shape) - ensure it exists before writing
  // below, same defensive need firstNoteForProject already has reading it.
  state.note = state.note || {};
  var existingNote = firstNoteForProject(state, projectId);
  var dictated = String(noteText).trim();
  var crypto = getCrypto();
  var clientId = getOrCreateClientId();
  var newVectorClock = incrementVectorClock(loadVectorClock(), clientId);
  saveVectorClock(newVectorClock);
  var op;

  if (existingNote) {
    var existingContent = existingNote.content || '';
    var newContent = existingContent ? existingContent + '\n\n' + NOTE_APPEND_DIVIDER + '\n\n' + dictated : dictated;
    state.note[existingNote.id] = Object.assign({}, existingNote, { content: newContent, modified: Date.now() });
    saveState(state);

    // Same "[Note] Update Note" / { id, changes } shape as note.actions.ts's
    // real updateNote action - entityChanges required for the same
    // isMultiEntityPayload() unwrap reason documented on handleTaskToggle's
    // payload above.
    var updPayload = {
      actionPayload: { note: { id: existingNote.id, changes: { content: newContent, modified: Date.now() } } },
      entityChanges: [],
    };
    op = {
      id: generateOpId(),
      opType: 'UPD',
      actionType: '[Note] Update Note',
      entityType: 'NOTE',
      entityId: existingNote.id,
      payload: crypto ? crypto.encrypt(updPayload) : updPayload,
      isPayloadEncrypted: !!crypto,
      vectorClock: newVectorClock,
      clientId: clientId,
      timestamp: Date.now(),
      schemaVersion: SCHEMA_VERSION,
    };
  } else {
    var newNote = {
      id: generateNoteId(),
      projectId: projectId,
      isPinnedToToday: false,
      content: dictated,
      created: Date.now(),
      modified: Date.now(),
    };
    state.note[newNote.id] = newNote;
    saveState(state);

    // Same "[Note] Add Note" / { note, isPreventFocus } shape as
    // note.actions.ts's real addNote action.
    var addPayload = { actionPayload: { note: newNote, isPreventFocus: true }, entityChanges: [] };
    op = {
      id: generateOpId(),
      opType: 'CRT',
      actionType: '[Note] Add Note',
      entityType: 'NOTE',
      entityId: newNote.id,
      payload: crypto ? crypto.encrypt(addPayload) : addPayload,
      isPayloadEncrypted: !!crypto,
      vectorClock: newVectorClock,
      clientId: clientId,
      timestamp: Date.now(),
      schemaVersion: SCHEMA_VERSION,
    };
  }

  // Re-sends this project's full notes right away, same "let an open
  // overlay pick up the appended text in place" reasoning as
  // handleNoteAppend's own sendFullNotesForTask call above.
  sendFullNotesForProject(projectId);

  var noteFailureMsg = null;
  uploadSingleOp(op, config, clientId)
    .catch(function (err) {
      noteFailureMsg = (err && err.message) || 'upload failed, will retry next sync';
      console.log('[pkjs] failed to upload project note append: ' + noteFailureMsg);
      sendStatus(STATUS_ERROR, noteFailureMsg);
    })
    .then(function () {
      runAutoSyncAfterOp(config, noteFailureMsg);
    });
}

// Watch-triggered "Finish Day" - archives every currently-done task, the
// one core data-mutating effect of the real app's Finish Day flow
// (daily-summary.component.ts's finishDay() -> TaskService.moveToArchive()
// -> TaskSharedActions.moveToArchive({ tasks })). The rest of that flow
// (mood/eval sheet, work-time note, "plan tomorrow" tab, GitLab pre-sync
// hooks) is optional UI enrichment moveToArchive's own reducer never reads.
//
// Unlike every other watch-initiated op, the watch sends NO task data at
// all here - just a bare trigger (see send_finish_day in main.c) - because
// the real moveToArchive payload needs FULL task objects: other real
// clients write whatever's sent here directly into their own permanent
// archive store when this op replays on their side
// (writeTasksToArchiveForRemoteSync in the real app's
// archive-operation-handler.service.ts), so a stub built from the watch's
// own trimmed C-side Task struct (title/done/due/time only) would archive
// permanently lossy data on every other client. This phone's own
// state.task cache already holds the full task objects as synced from the
// op log, so building the payload here (not on the watch) is the only way
// to avoid that.
function handleFinishDay() {
  var config = loadConfig();
  if (!config || !config.jwt) {
    sendStatus(STATUS_NOT_PAIRED);
    return;
  }

  var state = loadState();
  var allTasks = state.task || {};
  // Matches the real workContextService.doneTasks$ -> moveToArchive(doneTasks)
  // shape: only top-level tasks are passed as TaskWithSubTasks[], each with
  // its subtasks nested inline - subtask completion doesn't independently
  // archive, it rides along with its parent (same treatment subtasks get
  // everywhere else in this file - see pushTaskAndSubtasks in task-store.js).
  var doneParents = Object.keys(allTasks)
    .map(function (id) { return allTasks[id]; })
    .filter(function (t) { return t && t.isDone && !t.parentId; });

  if (doneParents.length === 0) {
    // Nothing to archive - a silent no-op, same convention as every other
    // no-effect watch action in this file (e.g. adjust_habit already at 0).
    return;
  }

  var tasksWithSubtasks = doneParents.map(function (t) {
    var subTasks = (t.subTaskIds || [])
      .map(function (subId) { return allTasks[subId]; })
      .filter(function (sub) { return !!sub; });
    return Object.assign({}, t, { subTasks: subTasks });
  });
  var archivedIds = doneParents.map(function (t) { return t.id; });

  // Optimistic local update via the SAME replay path a real synced
  // moveToArchive op goes through (task-store.js's applyTaskAction
  // 'moveToArchive' case, already there for the read side since it's how
  // this watch reacts when a REAL client finishes their day) - so this
  // can't drift from what replay actually does.
  store.applyOperations(
    [{ op: { entityType: 'TASK', actionType: '[Task Shared] moveToArchive', opType: 'UPD', payload: { actionPayload: { tasks: tasksWithSubtasks } }, isPayloadEncrypted: false } }],
    state
  );
  saveState(state);
  // Unlike toggle/habit-adjust, archived tasks vanish from the list rather
  // than just changing in place - push the updated list right away rather
  // than waiting for uploadSingleOp/runAutoSyncAfterOp's follow-up sync.
  sendTaskListToWatch(watchTaskList(state, config));

  var crypto = getCrypto();
  // entityChanges required for the same isMultiEntityPayload() unwrap
  // reason as every other payload in this file. entityIds (plural, plus
  // entityId set to the first id) mirrors the real client's own bulk-op
  // wire shape for moveToArchive (operation-log.effects.ts) - confirmed
  // against the server's own SuperSyncOperationSchema, which accepts both
  // as independent optional top-level fields.
  var payload = { actionPayload: { tasks: tasksWithSubtasks }, entityChanges: [] };
  var clientId = getOrCreateClientId();
  var newVectorClock = incrementVectorClock(loadVectorClock(), clientId);
  saveVectorClock(newVectorClock);
  var op = {
    id: generateOpId(),
    opType: 'UPD',
    actionType: '[Task Shared] moveToArchive',
    entityType: 'TASK',
    entityId: archivedIds[0],
    entityIds: archivedIds,
    payload: crypto ? crypto.encrypt(payload) : payload,
    isPayloadEncrypted: !!crypto,
    vectorClock: newVectorClock,
    clientId: clientId,
    timestamp: Date.now(),
    schemaVersion: SCHEMA_VERSION,
  };

  var finishDayFailureMsg = null;
  uploadSingleOp(op, config, clientId)
    .catch(function (err) {
      finishDayFailureMsg = (err && err.message) || 'upload failed, will retry next sync';
      console.log('[pkjs] failed to upload finish-day archive: ' + finishDayFailureMsg);
      sendStatus(STATUS_ERROR, finishDayFailureMsg);
    })
    .then(function () {
      runAutoSyncAfterOp(config, finishDayFailureMsg);
    });
}

// ---------------- live tracking presence ----------------

// SuperSync live-tracking presence (super-productivity desktop v18.21.1).
// Opt-in via config.liveTracking, SuperSync only. The watch shows a "LIVE"
// row plus a detail window for whatever ANOTHER device is tracking, and can
// stop it. Alive only while the watchapp is open - PebbleKit JS does not run
// in the background, so this is a glance, not a notification. Transport and
// the E2EE envelope codec live in lib/presence-client.js.
var presenceClient = null;
var presenceClientToken = null;
var presenceLastSessionId = null;
var presenceLastReceivedAt = 0;
var presenceStaleTimer = null;
// Phase 2: what this watch is currently broadcasting as its own tracking, or
// null. Kept here (not just in presence-client) so a MSG_TRACK_TIME_START that
// lands before the socket is open still gets announced once it connects.
var presenceBroadcasting = null; // { taskId, sinceTs }

// How often to re-check whether a shown "tracking" session has gone silent
// (producer closed its app without a final "stopped").
var PRESENCE_STALE_CHECK_MS = 20 * 1000;

// While a background "the tracked task isn't in our synced state yet" catch-up
// sync sequence is running: { taskId, tries }. Bounded so a task that never
// arrives (deleted again, permission change) can't loop forever.
var presenceCatchUp = null;
var PRESENCE_CATCH_UP_MAX_TRIES = 3;
var PRESENCE_CATCH_UP_RETRY_MS = 5 * 1000;

function stopPresenceStaleTimer() {
  if (presenceStaleTimer) {
    clearInterval(presenceStaleTimer);
    presenceStaleTimer = null;
  }
}

// A task the desktop just created and immediately started tracking can reach
// the watch as presence (over the WS) before this phone has pulled the op that
// creates it - so pushPresenceToWatch can't resolve a title and the watch
// shows "a recently started task". Pull it in with a background sync, then
// re-push the presence view so the real name lands. Bailed the moment the task
// shows up, the session changes, or the try budget runs out.
function catchUpTrackedTask(view) {
  if (view.opaque || view.state !== 'tracking' || !view.taskId) {
    return;
  }
  if (loadState().task[view.taskId]) {
    return; // already synced - nothing to chase
  }
  if (presenceCatchUp && presenceCatchUp.taskId === view.taskId) {
    return; // already chasing this one
  }
  presenceCatchUp = { taskId: view.taskId, tries: 0 };
  var step = function () {
    if (!presenceCatchUp || presenceCatchUp.taskId !== view.taskId ||
        presenceLastSessionId !== view.sessionId) {
      presenceCatchUp = null;
      return;
    }
    presenceCatchUp.tries++;
    var syncPromise = doSync();
    var afterSync = function () {
      if (!presenceCatchUp || presenceLastSessionId !== view.sessionId) {
        presenceCatchUp = null;
        return;
      }
      if (loadState().task[view.taskId]) {
        presenceCatchUp = null;
        pushPresenceToWatch(view, false);
        return;
      }
      if (presenceCatchUp.tries >= PRESENCE_CATCH_UP_MAX_TRIES) {
        presenceCatchUp = null;
        return;
      }
      setTimeout(step, PRESENCE_CATCH_UP_RETRY_MS);
    };
    if (syncPromise && typeof syncPromise.then === 'function') {
      syncPromise.then(afterSync);
    } else {
      setTimeout(afterSync, PRESENCE_CATCH_UP_RETRY_MS);
    }
  };
  step();
}

function sendPresenceClear() {
  presenceLastSessionId = null;
  presenceCatchUp = null;
  stopPresenceStaleTimer();
  sendWithRetry({ MSG_TYPE: MSG_PRESENCE_UPDATE, PRESENCE_STATE: 0 }, function () {}, function (e) {
    console.log('[pkjs] giving up on PRESENCE_UPDATE(clear): ' + JSON.stringify(e));
  });
}

// Pushes one decoded presence view to the watch. `stale` forces the
// "was tracking" state regardless of what the producer last reported.
function pushPresenceToWatch(view, stale, isNewSession) {
  var state;
  var title;
  var device = '';
  var canStop = 0;
  var elapsedS = 0;
  var t = null;

  if (view.opaque) {
    // Couldn't decode the payload (no matching Argon2 key cached yet, or a
    // fail-closed plaintext envelope) - still surface it, just unnamed. Empty
    // title lets the watch fall back to its generic "Live tracking" label.
    state = 1;
    title = '';
  } else {
    device = view.deviceLabel || '';
    elapsedS = Math.max(0, Math.round((Date.now() - view.sinceTs) / 1000));
    t = loadState().task[view.taskId];
    title = (t && t.title) ? String(t.title) : 'a recently started task';
    if (stale) {
      state = 3;
    } else if (view.state === 'tracking') {
      state = 1;
      canStop = view.producerConnected ? 1 : 0;
    } else if (view.reason === 'idle') {
      state = 2;
    } else {
      // A plain "stopped" - mirror the desktop chip: keep it visible as
      // "Stopped on X" for the linger window, then presence-client's own
      // linger timer fires onCleared and we hide it. A re-start inside the
      // window replaces it in place.
      state = 4;
    }
  }

  var dict = {
    MSG_TYPE: MSG_PRESENCE_UPDATE,
    PRESENCE_STATE: state,
    PRESENCE_TASK_TITLE: String(title).slice(0, 63),
    PRESENCE_DEVICE: String(device).slice(0, 23),
    PRESENCE_ELAPSED_S: Math.min(elapsedS, 2000000000),
    PRESENCE_CAN_STOP: canStop,
  };
  // The tracked task's synced time / estimate, so the detail window can show
  // "spent / estimate" like a task row - only when we resolved the task and it
  // has an estimate.
  if (t && t.timeEstimate) {
    dict.PRESENCE_SPENT_MS = Math.min(t.timeSpent || 0, 2000000000);
    dict.PRESENCE_ESTIMATE_MS = Math.min(t.timeEstimate, 2000000000);
  }
  // Tells the watch to re-arm its over-estimate notification for a freshly
  // started / switched remote session (see maybe_notify_overtime).
  if (isNewSession) {
    dict.PRESENCE_NEW_SESSION = 1;
  }
  sendWithRetry(dict, function () {}, function (e) {
    console.log('[pkjs] giving up on PRESENCE_UPDATE: ' + JSON.stringify(e));
  });
}

function onPresenceState(view) {
  presenceLastReceivedAt = Date.now();
  var newSessionId = view.opaque ? null : view.sessionId;
  // A different sessionId means the remote device started (or switched) a
  // tracking session - the watch re-arms its over-estimate latch on this.
  var isNewSession = !!(newSessionId && newSessionId !== presenceLastSessionId);
  presenceLastSessionId = newSessionId;

  // While THIS watch is broadcasting its own tracking, a remote device also
  // claiming "tracking" is a takeover contest (only one active session
  // account-wide). Later start wins - mirrors _resolveTakeover on the desktop.
  if (presenceBroadcasting && !view.opaque && view.state === 'tracking') {
    if (view.sinceTs > presenceBroadcasting.sinceTs) {
      // Remote is newer - stop the watch's timer and let the remote show.
      sendWithRetry({ MSG_TYPE: MSG_PRESENCE_STOP_LOCAL }, function () {}, function () {});
    } else {
      // We are newer - re-announce so the other device stops itself, and
      // don't show its (losing) state on our own watch.
      if (presenceClient) {
        presenceClient.broadcastTracking(presenceBroadcasting.taskId, presenceBroadcasting.sinceTs);
      }
      stopPresenceStaleTimer();
      return;
    }
  }

  pushPresenceToWatch(view, false, isNewSession);
  catchUpTrackedTask(view);

  stopPresenceStaleTimer();
  if (!view.opaque && view.state === 'tracking') {
    presenceStaleTimer = setInterval(function () {
      if (Date.now() - presenceLastReceivedAt > presence.STALE_AFTER_MS) {
        pushPresenceToWatch(view, true);
        stopPresenceStaleTimer();
      }
    }, PRESENCE_STALE_CHECK_MS);
  }
}

// (Re)applies config.liveTracking: opens the presence WebSocket when the
// feature is on and the account is paired, tears it down (and hides the watch
// UI) otherwise or when the token changed. Called from 'ready' and after every
// settings save, same as scheduleAutoSync().
function applyPresence(config) {
  var wantsPresence = !!(config && config.liveTracking && config.jwt);
  var token = config && config.jwt;
  if (presenceClient && (!wantsPresence || presenceClientToken !== token)) {
    presenceClient.disconnect();
    presenceClient = null;
  }
  if (!wantsPresence) {
    presenceBroadcasting = null;
    sendPresenceClear();
    return;
  }
  if (!presenceClient) {
    presenceClientToken = token;
    presenceClient = new presence.PresenceClient({
      baseUrl: config.baseUrl || supersync.DEFAULT_BASE_URL,
      token: token,
      clientId: getOrCreateClientId(),
      getCrypto: getCrypto,
      log: function (m) { console.log('[presence] ' + m); },
    });
    presenceClient.onState(onPresenceState);
    presenceClient.onCleared(sendPresenceClear);
    // A remote device stopped the timer this watch is broadcasting - relay
    // the stop to the watch; its own MSG_TRACK_TIME_STOP then acks it.
    presenceClient.onStopCommand(function () {
      sendWithRetry({ MSG_TYPE: MSG_PRESENCE_STOP_LOCAL }, function () {}, function () {});
    });
  }
  presenceClient.connect();
  // Resume broadcasting a session already in progress (the watch told us
  // before the socket was up, or the client was just rebuilt).
  if (presenceBroadcasting) {
    presenceClient.broadcastTracking(presenceBroadcasting.taskId, presenceBroadcasting.sinceTs);
  }
}

// ---------------- Pebble event wiring ----------------

Pebble.addEventListener('ready', function () {
  console.log('[pkjs] ready');
  var config = loadConfig();
  var lastSyncedAt = loadLastSyncedAt();
  // See RECENT_SYNC_SKIP_MS's own comment - skips a real network sync only
  // for the passive "app just opened" trigger, and only when paired (an
  // unpaired watch has never actually synced, so lastSyncedAt is always 0
  // there and this condition can't be true).
  if (config && config.jwt && lastSyncedAt && Date.now() - lastSyncedAt < RECENT_SYNC_SKIP_MS &&
      opQueue.count(localStorage) === 0) {
    console.log('[pkjs] skipping sync on open, last synced ' + Math.round((Date.now() - lastSyncedAt) / 1000) + 's ago');
    pushCachedStateToWatch(config);
  } else {
    doSync();
  }
  scheduleAutoSync(config);
  applyPresence(config);
});

Pebble.addEventListener('appmessage', function (e) {
  var payload = e.payload;
  switch (payload.MSG_TYPE) {
    case MSG_REQUEST_SYNC:
      doSync();
      break;
    case MSG_TASK_TOGGLE:
      handleTaskToggle(payload.TASK_ID, payload.TASK_DONE === 1);
      break;
    case MSG_TRACK_TIME_START:
      handleTrackStart(payload.TASK_ID, payload.TRACKED_MS);
      break;
    case MSG_TRACK_TIME_STOP:
      handleTrackTimeStop(payload.TASK_ID, payload.TRACKED_MS);
      handleTrackStopBroadcast();
      break;
    case MSG_HABIT_ADJUST:
      handleHabitAdjust(payload.HABIT_ID, payload.HABIT_DELTA);
      break;
    case MSG_HABIT_TRACK_STOP:
      handleHabitTrackStop(payload.HABIT_ID, payload.TRACKED_MS);
      break;
    case MSG_TASK_ADD:
      handleAddTask(payload.TASK_TITLE);
      break;
    case MSG_TASK_PLAN_TOMORROW:
      handleTaskReschedule(payload.TASK_ID, 'tomorrow', payload.PROJECT_ID);
      break;
    case MSG_TASK_PLAN_TODAY:
      handleTaskReschedule(payload.TASK_ID, 'today', payload.PROJECT_ID);
      break;
    case MSG_TASK_UNSCHEDULE:
      handleTaskReschedule(payload.TASK_ID, 'unschedule', payload.PROJECT_ID);
      break;
    case MSG_PRESENCE_STOP:
      if (presenceClient && presenceLastSessionId) {
        presenceClient.requestStop(presenceLastSessionId);
      }
      break;
    case MSG_FINISH_DAY:
      handleFinishDay();
      break;
    case MSG_NOTE_APPEND:
      handleNoteAppend(payload.TASK_ID, payload.NOTE_TEXT);
      break;
    case MSG_NOTE_REQUEST:
      handleNoteRequest(payload.TASK_ID);
      break;
    case MSG_PROJECT_NOTE_APPEND:
      handleProjectNoteAppend(payload.PROJECT_ID, payload.NOTE_TEXT);
      break;
    case MSG_PROJECT_NOTE_REQUEST:
      handleProjectNoteRequest(payload.PROJECT_ID);
      break;
    case MSG_PROJECT_LIST_REQUEST:
      handleProjectListRequest();
      break;
    case MSG_PROJECT_TASKS_REQUEST:
      handleProjectTasksRequest(payload.PROJECT_ID);
      break;
    case MSG_STATS_REQUEST:
      handleStatsRequest();
      break;
    default:
      break;
  }
});

Pebble.addEventListener('showConfiguration', function () {
  var config = loadConfig() || {};
  var state = loadState();
  var projects = Object.keys(state.project || {}).map(function (id) {
    return { id: id, title: state.project[id].title };
  });
  var url = pairingPage.buildPairingPageUrl(
    config.baseUrl || supersync.DEFAULT_BASE_URL,
    config.email || '',
    {
      groupByProject: !!config.groupByProject,
      todayOnly: !!config.todayOnly,
      hideDoneTasks: !!config.hideDoneTasks,
      autoMarkParentDone: !!config.autoMarkParentDone,
      // Undefined (never configured before) defaults to on - see the
      // matching comment in handleTaskToggle for why.
      autoSyncOnComplete: config.autoSyncOnComplete !== false,
      // Minutes between background syncs; 0 means off - see
      // scheduleAutoSync's own comment for why this defaults to off rather
      // than mirroring autoSyncOnComplete's default-on.
      autoSyncIntervalMin: config.autoSyncIntervalMin || 0,
      // Lets the pairing page leave the password/token fields blank on a
      // settings-only visit instead of demanding they be re-pasted - see
      // webviewclosed below for the other half of this.
      hasPassword: !!localStorage.getItem('sp_password'),
      hasToken: !!config.jwt,
      // "Add Task" dictation's target project - a setting local to this
      // app's own pairing page, separate from the real app's own
      // GlobalConfig.tasks.defaultProjectId. Empty string means "fall back
      // to Inbox", not "unset vs configured" - see handleAddTask.
      defaultProjectId: config.defaultProjectId || '',
      // Minutes of time estimate to stamp on a watch-dictated task; 0 means
      // leave it unset - see handleAddTask.
      defaultTaskEstimateMin: config.defaultTaskEstimateMin || 0,
      projects: projects,
      enableHabits: config.enableHabits !== false,
      enableAddTask: config.enableAddTask !== false,
      enableProjects: config.enableProjects !== false,
      enableStats: config.enableStats !== false,
      enableSchedule: config.enableSchedule !== false,
      backlightMode: config.backlightMode || 0,
      touchNav: !!config.touchNav,
      overtimeNotify: !!config.overtimeNotify,
      overtimeRepeat: !!config.overtimeRepeat,
      overtimeSound: !!config.overtimeSound,
      breakReminderMin: config.breakReminderMin || 0,
      liveTracking: !!config.liveTracking,
    }
  );
  Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function (e) {
  if (!e.response) {
    return;
  }
  var result;
  try {
    result = JSON.parse(decodeURIComponent(e.response));
  } catch (err) {
    console.log('[pkjs] could not parse config page response: ' + err.message);
    return;
  }
  if (result.cancelled) {
    return;
  }

  // The pairing page's own "Clear all data & resync" button, separate from
  // Save & sync - wipes the local replay cache without touching credentials/
  // options, then does a fresh full resync (isFirstSync in doSync() keys off
  // exactly this state: lastSeq 0 and no cached tasks). Also clears
  // sp_last_synced_at - if the immediate doSync() below doesn't get to
  // finish before this JS session ends (e.g. backing out of the config page
  // relaunches the watchapp's JS, firing a new 'ready' event) and that
  // relaunch lands inside RECENT_SYNC_SKIP_MS of the *previous* (pre-clear)
  // sync, the 'ready' handler would otherwise skip the real sync and push
  // the now-empty cache straight to the watch instead of forcing a fresh
  // download.
  if (result.clearData) {
    localStorage.removeItem('sp_entities');
    localStorage.removeItem('sp_last_seq');
    localStorage.removeItem('sp_vector_clock');
    localStorage.removeItem('sp_last_synced_at');
    doSync();
    return;
  }

  var existingConfig = loadConfig() || {};
  var previousPassword = localStorage.getItem('sp_password');
  // Blank jwt/password fields mean "keep what's already saved" (the
  // pairing page only requires jwt on a first-ever pairing - see hasToken
  // there), not "clear it" - so a settings-only visit doesn't force
  // re-pasting either one.
  var newJwt = result.jwt || existingConfig.jwt;
  var jwtChanged = !!result.jwt && result.jwt !== existingConfig.jwt;
  var passwordChanged = !!result.password && result.password !== previousPassword;

  var newConfig = {
    baseUrl: result.baseUrl || supersync.DEFAULT_BASE_URL,
    email: result.email,
    jwt: newJwt,
    groupByProject: !!result.groupByProject,
    todayOnly: !!result.todayOnly,
    hideDoneTasks: !!result.hideDoneTasks,
    autoMarkParentDone: !!result.autoMarkParentDone,
    autoSyncOnComplete: !!result.autoSyncOnComplete,
    // saveConfig() is a full replace, not a merge - every field the app
    // wants persisted has to be listed here explicitly, or it silently
    // vanishes on the next settings-only save (e.g. toggling todayOnly).
    defaultProjectId: result.defaultProjectId || '',
    defaultTaskEstimateMin: parseInt(result.defaultTaskEstimateMin, 10) || 0,
    enableHabits: !!result.enableHabits,
    enableAddTask: !!result.enableAddTask,
    enableProjects: !!result.enableProjects,
    enableStats: !!result.enableStats,
    enableSchedule: !!result.enableSchedule,
    autoSyncIntervalMin: parseInt(result.autoSyncIntervalMin, 10) || 0,
    backlightMode: parseInt(result.backlightMode, 10) || 0,
    touchNav: !!result.touchNav,
    overtimeNotify: !!result.overtimeNotify,
    overtimeRepeat: !!result.overtimeRepeat,
    overtimeSound: !!result.overtimeSound,
    breakReminderMin: parseInt(result.breakReminderMin, 10) || 0,
    liveTracking: !!result.liveTracking,
  };
  saveConfig(newConfig);
  scheduleAutoSync(newConfig);

  if (result.password) {
    localStorage.setItem('sp_password', result.password);
  }
  // The Argon2id derived-key cache in getCrypto() is keyed off this same
  // password, but comparing by value there isn't enough on its own to
  // notice "same password string, different account" - clearing it here
  // whenever pairing completes is cheap insurance either way.
  cachedCrypto = null;
  cachedPassword = null;

  // Only an actual credential change (new account/token or changed
  // password) invalidates what's cached locally - a settings-only save
  // (e.g. toggling todayOnly) used to wipe and fully re-download the task
  // list every time, which is exactly what the new clearData path above is
  // for now; this path should be quiet unless something that changes what
  // the replay log means actually changed.
  if (jwtChanged || passwordChanged) {
    localStorage.removeItem('sp_entities');
    localStorage.removeItem('sp_last_seq');
    localStorage.removeItem('sp_vector_clock');
    // Queued offline ops carry a payload encrypted with the OLD password
    // and, on a jwt change, belong to a different account entirely - both
    // make them undecryptable garbage to the new pairing. Drop them.
    opQueue.clear(localStorage);
    // Persisted Argon2id keys are tied to the old password (and, via
    // jwtChanged, possibly a different account entirely) - drop them so we
    // don't try to decrypt the new account's ops with stale keys. The
    // clearData path above deliberately does NOT clear these: the password
    // is unchanged there, so keeping the cache is exactly what makes the
    // forced full resync fast.
    clearKdfCache();
  }

  // After the crypto caches settle above - a token change rebuilds the client.
  applyPresence(newConfig);

  doSync();
});
