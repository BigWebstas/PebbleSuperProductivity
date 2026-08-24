#include <pebble.h>

// No runtime API exposes the app's own versionLabel (package.json's
// "version") to C code - keep this in sync by hand on every version bump.
#define APP_VERSION "0.6.17"

// Dictionary keys are the MESSAGE_KEY_* externs pebble.h pulls in from
// message_keys.auto.h, generated from the "messageKeys" list in
// package.json - NOT small hand-picked integers. AppMessage assigns each
// key an ID starting at 10000, so a local 0-based enum silently never
// matches what the phone sends/expects.
#define KEY_MSG_TYPE MESSAGE_KEY_MSG_TYPE
#define KEY_TASK_TOTAL MESSAGE_KEY_TASK_TOTAL
#define KEY_TASK_INDEX MESSAGE_KEY_TASK_INDEX
#define KEY_TASK_ID MESSAGE_KEY_TASK_ID
#define KEY_TASK_TITLE MESSAGE_KEY_TASK_TITLE
#define KEY_TASK_DONE MESSAGE_KEY_TASK_DONE
#define KEY_TASK_PROJECT MESSAGE_KEY_TASK_PROJECT
#define KEY_TASK_DUE_MIN MESSAGE_KEY_TASK_DUE_MIN
#define KEY_TASK_TIME_SPENT_MS MESSAGE_KEY_TASK_TIME_SPENT_MS
#define KEY_TASK_TIME_ESTIMATE_MS MESSAGE_KEY_TASK_TIME_ESTIMATE_MS
#define KEY_TRACKED_MS MESSAGE_KEY_TRACKED_MS
#define KEY_STATUS_CODE MESSAGE_KEY_STATUS_CODE
#define KEY_STATUS_MSG MESSAGE_KEY_STATUS_MSG
#define KEY_HABIT_TOTAL MESSAGE_KEY_HABIT_TOTAL
#define KEY_HABIT_INDEX MESSAGE_KEY_HABIT_INDEX
#define KEY_HABIT_ID MESSAGE_KEY_HABIT_ID
#define KEY_HABIT_TITLE MESSAGE_KEY_HABIT_TITLE
#define KEY_HABIT_DONE MESSAGE_KEY_HABIT_DONE
#define KEY_HABIT_VALUE MESSAGE_KEY_HABIT_VALUE
#define KEY_HABIT_GOAL MESSAGE_KEY_HABIT_GOAL
#define KEY_HABIT_DELTA MESSAGE_KEY_HABIT_DELTA
#define KEY_HABIT_TYPE MESSAGE_KEY_HABIT_TYPE
#define KEY_HABIT_COUNTDOWN_MS MESSAGE_KEY_HABIT_COUNTDOWN_MS
#define KEY_HABITS_ENABLED MESSAGE_KEY_HABITS_ENABLED
#define KEY_ADD_TASK_ENABLED MESSAGE_KEY_ADD_TASK_ENABLED
#define KEY_BACKLIGHT_MODE MESSAGE_KEY_BACKLIGHT_MODE
#define KEY_TASK_NOTES MESSAGE_KEY_TASK_NOTES
#define KEY_AUTO_SYNC_INTERVAL_MIN MESSAGE_KEY_AUTO_SYNC_INTERVAL_MIN
#define KEY_NOTE_TEXT MESSAGE_KEY_NOTE_TEXT

// MSG_TYPE values, watch <-> phone.
enum {
  MSG_TASK_SYNC_START = 1,  // phone -> watch: TASK_TOTAL follows
  MSG_TASK_ITEM = 2,        // phone -> watch: one task (TASK_INDEX/ID/TITLE/DONE)
  MSG_TASK_SYNC_END = 3,    // phone -> watch: list is complete, redraw
  MSG_SYNC_STATUS = 4,      // phone -> watch: STATUS_CODE (+ optional STATUS_MSG)
  MSG_REQUEST_SYNC = 5,     // watch -> phone: please refresh
  MSG_TASK_TOGGLE = 6,      // watch -> phone: TASK_ID + TASK_DONE (new state)
  MSG_TRACK_TIME_STOP = 7,  // watch -> phone: TASK_ID + TRACKED_MS (this session's tracked ms)
  MSG_HABIT_SYNC_START = 8, // phone -> watch: HABIT_TOTAL follows
  MSG_HABIT_ITEM = 9,       // phone -> watch: one habit
  MSG_HABIT_SYNC_END = 10,  // phone -> watch: list is complete, redraw
  MSG_HABIT_ADJUST = 11,    // watch -> phone: HABIT_ID + HABIT_DELTA (+1 or -1)
  MSG_TASK_ADD = 12,        // watch -> phone: TASK_TITLE (new task's dictated title)
  MSG_HABIT_TRACK_STOP = 13, // watch -> phone: HABIT_ID + TRACKED_MS (this session's tracked ms, StopWatch-type only)
  MSG_FINISH_DAY = 14,      // watch -> phone: archive every currently-done task (no extra keys)
  MSG_NOTE_APPEND = 15,     // watch -> phone: TASK_ID + NOTE_TEXT (dictated text to append to this task's notes)
};

// STATUS_CODE values sent from the phone.
enum {
  STATUS_OK = 0,
  STATUS_SYNCING = 1,
  STATUS_NOT_PAIRED = 2,
  STATUS_ERROR = 3,
};

// emery (Pebble Time 2) has far more free RAM than the rest of the lineup
// (~106KB vs ~41KB on basalt/chalk/diorite, and aplite well under 1KB after
// MAX_ID_LEN's own bump - see MAX_HABITS' comment below for the same split)
// so it alone gets a higher cap rather than raising the shared default and
// risking the tighter platforms.
#ifdef PBL_PLATFORM_EMERY
#define MAX_TASKS 50
#else
#define MAX_TASKS 30
#endif
#define MAX_TITLE_LEN 64
// Plain generated task ids are ~21 chars, but calendar-integration ids
// (generateCalendarTaskId: `cal_${issueProviderId}_${calendarEventId}`) have
// no fixed cap - issueProviderId alone is a ~21-char id, and calendarEventId
// is an arbitrary iCal UID that can itself carry a recurrence-instance
// timestamp suffix. A real one confirmed live on a real account ran 79
// chars. 40 silently truncated it: the truncated id doesn't match any real
// task server-side, so a toggle/track-time upload against it touches
// nothing, while the task's TITLE (a separate field) still displayed
// correctly - completely invisible until you act on that specific task.
#define MAX_ID_LEN 96
#define MAX_PROJECT_LEN 32
// A task's notes can be many paragraphs of markdown in the real app, but
// this is a glance-sized preview (see show_notes_overlay), not a reader -
// kept short deliberately, both because nothing here scrolls past what one
// screen can show and because this field is carried by EVERY Task in the
// double-buffered s_tasks/s_incoming arrays (MAX_TASKS * 2 copies), unlike
// the one-time-cost overlay text itself. #ifndef PBL_PLATFORM_APLITE-gated
// entirely, same reasoning as the backlight feature above: aplite has no
// RAM budget left for a whole extra per-task field (confirmed via pebble
// build's own memory report - see BACKLIGHT_MODE_ALWAYS_ON's comment for
// the exact numbers), and without it there'd be nothing to show anyway, so
// aplite keeps the plain instant single-click toggle it's always had
// rather than paying a universal double-click commit delay for a feature
// it can't display.
#ifndef PBL_PLATFORM_APLITE
#define MAX_NOTES_LEN 200
#endif

typedef struct {
  char id[MAX_ID_LEN];
  char title[MAX_TITLE_LEN];
  char project[MAX_PROJECT_LEN]; // '' when the phone isn't grouping by project
  bool done;
  int due_min;       // minutes since local midnight, or -1 when the task has no dueWithTime
  int time_spent_ms; // total tracked time (all days, all devices), 0 if none
  int time_estimate_ms; // 0 if none
#ifndef PBL_PLATFORM_APLITE
  char notes[MAX_NOTES_LEN]; // '' when the task has no notes
#endif
} Task;

// One entry per contiguous run of equal Task.project in s_tasks (the phone
// pre-sorts by project when grouping is on, so a run IS a group). When
// grouping is off the phone sends '' for every task, which always
// collapses to exactly one group covering the whole list - indistinguishable
// from the pre-grouping flat-list behavior, by construction.
typedef struct {
  char name[MAX_PROJECT_LEN];
  int start; // index into s_tasks
  int count;
} TaskGroup;

// "Habits" are Super Productivity's SimpleCounter feature (real entityType
// SIMPLE_COUNTER - there is no separate "Habit" entity). StopWatch-type
// counters have a ms-valued value/goal (tracked time, not a "did you do
// this today" count) and a long-select-to-start/stop timer, same as a
// RepeatedCountdownReminder-type counter's own long-select-to-start/stop
// countdown timer (see Habit.is_countdown's own comment) - both distinct
// from the plain Select/long-select increment/decrement a ClickCounter row
// uses. See Habit.is_stopwatch, habits_menu_select_long_click, and
// start_habit_tracking/stop_habit_tracking_and_report below.
// Kept low (unlike MAX_TASKS' 30) because aplite's ~24KB RAM budget is
// already tight after MAX_ID_LEN's own 96-byte bump for calendar tasks.
// aplite specifically, not a shared cap - basalt/chalk/diorite still have
// ~41KB free, so holding every platform back to aplite's number here would
// be needlessly conservative. emery gets its own higher ceiling for the
// same reason MAX_TASKS does - ~95KB free even after every other feature
// in this file, confirmed via pebble build's own per-platform memory
// report (same verification standard as every other budget note here).
// Confirmed via pebble build's own per-platform memory report: 8 overflowed
// aplite's linked binary by 280 bytes even before the row-icon resources
// added below; those pushed the workable aplite number down further to 3.
// The StopWatch habit timer (see s_tracking_habit_id's own comment) is
// excluded from aplite's VISIBLE habit list entirely rather than shown
// read-only there (see resolve_habit_at/habits_menu_get_num_rows) - even
// with the tracking machinery and its display path fully compiled out,
// the Habit struct's own extra is_stopwatch field (needed on every
// platform, to know which entries to skip) still grew aplite's habit
// array past budget, pushing the workable number down once more to 2.
#ifdef PBL_PLATFORM_APLITE
#define MAX_HABITS 2
#elif defined(PBL_PLATFORM_EMERY)
#define MAX_HABITS 16
#else
#define MAX_HABITS 8
#endif
// SimpleCounter ids are always a plain nanoid() (simple-counter.service.ts)
// - unlike TASK, there's no calendar-integration id format to accommodate,
// so this doesn't need MAX_ID_LEN's 96-byte allowance; a smaller buffer
// here matters given MAX_HABITS' own memory comment above.
#define MAX_HABIT_ID_LEN 32
typedef struct {
  char id[MAX_HABIT_ID_LEN];
  char title[MAX_TITLE_LEN];
  // The three bools are adjacent (not scattered around value/goal) so they
  // share a single alignment-padding gap ahead of the ints instead of each
  // opening their own - zero-cost on every platform (3 bools + 1 byte
  // padding is the same 4 bytes 2 bools + 2 bytes padding already cost), but
  // every byte still matters on aplite (see MAX_HABITS' own comment).
  bool done;
  bool is_stopwatch; // StopWatch-type counter - see the comment above
  // RepeatedCountdownReminder-type counter: a plain count (today's completed
  // rounds, same units as a ClickCounter's value/goal - NOT ms), but with
  // its own long-select-to-start/stop countdown timer instead of plain
  // Select/long-select +1/-1 - see countdown_ms below,
  // habits_menu_select_long_click, and complete_habit_countdown. Always
  // false on aplite: resolve_habit_at/habits_menu_get_num_rows filter these
  // out of the visible list there, matching is_stopwatch's own aplite
  // exclusion (same RAM reasoning - a live countdown needs the same tracking
  // machinery a StopWatch timer does).
  bool is_countdown;
  int value; // today's count, or ms tracked today when is_stopwatch
  int goal;  // streakMinValue-derived target used for the "value/goal" subtitle
#ifndef PBL_PLATFORM_APLITE
  // Configured countdown length in ms for an is_countdown counter (0 for
  // every other type) - only meaningful to the countdown-timer machinery
  // below, itself aplite-excluded for the same reason the StopWatch timer
  // is, so this field doesn't exist there either (unlike is_countdown
  // itself, which stays a plain bool on every platform purely so
  // resolve_habit_at can filter it out - see that field's own comment).
  int countdown_ms;
#endif
} Habit;

// Single buffer, not the incoming/committed double-buffer s_tasks/s_incoming
// use - deliberately, to save memory on the tightest platform (aplite).
// Safe because nothing redraws the habits menu (reload_data/mark_dirty)
// until MSG_HABIT_SYNC_END bumps s_habit_count, exactly the same guarantee
// that already makes the task list's own mid-sync writes invisible.
static Habit s_habits[MAX_HABITS];
static int s_habit_count = 0;
static int s_habit_incoming_total = 0;

static Window *s_main_window;
static MenuLayer *s_menu_layer;
static Window *s_habits_window;
static MenuLayer *s_habits_menu_layer;
static TextLayer *s_habits_empty_layer;
static StatusBarLayer *s_habits_status_bar;
static StatusBarLayer *s_status_bar;
static TextLayer *s_empty_layer;
// Small subtitle beneath s_empty_layer's title, used only while the initial
// (no-cached-list-yet) sync is in progress - shows the sync-progress
// percentage (see syncing_timer_callback) or, while it's not yet available,
// the "may take a few minutes" heads-up. Kept as its own layer rather than
// folded into s_empty_layer's own text so the title itself ("Syncing...")
// can use a larger font without that longer, wrapping hint text blowing
// past the available height at the same size. aplite-excluded (#ifndef, not
// a runtime check - same MAX_HABITS/mic-dictation precedent elsewhere in
// this file): a second TextLayer struct overflowed aplite's already-tight
// APP region by 120 bytes even before accounting for anything else. aplite
// keeps the combined single-layer/single-font text it always had (see the
// #ifdef PBL_PLATFORM_APLITE branches in start_syncing_animation/
// syncing_timer_callback/stop_syncing_animation below) - percentage still
// shows there, just inline rather than as a separate bigger-font title.
#ifndef PBL_PLATFORM_APLITE
static TextLayer *s_sync_progress_layer;
#endif
static BitmapLayer *s_logo_layer;
static GBitmap *s_logo_bitmap;
// Icons drawn directly into the Resync/Habits rows (menu_draw_row, not a
// standing BitmapLayer - these rows are custom-painted, same as their
// title/subtitle text). One black/white pair per icon so it can invert on
// selection the same way the row's own text color already does -
// GCompOpSet's alpha-aware compositing doesn't give a free color-invert for
// an 8-bit source bitmap, so this is two actual assets, not one recolored
// at draw time.
#define ROW_ICON_SIZE 25
static GBitmap *s_check_bitmap;
static GBitmap *s_check_white_bitmap;
static GBitmap *s_heart_bitmap;
static GBitmap *s_heart_white_bitmap;
// Mic/dictation state - compiled out entirely on aplite (#ifndef, not just
// a runtime PBL_IF_MICROPHONE_ELSE check), matching MAX_HABITS' own
// #ifdef PBL_PLATFORM_APLITE precedent elsewhere in this file: aplite has no
// mic hardware and can never reach the "Add Task" row (see
// menu_get_num_rows), so paying for this code/data there at all - not just
// leaving it unreached at runtime - would eat into a budget this file has
// otherwise been careful to protect (aplite's RAM headroom dropped from a
// ~314-byte baseline to ~121 bytes when this was still a runtime-only guard,
// confirmed via pebble build's own memory report).
#ifndef PBL_PLATFORM_APLITE
static GBitmap *s_mic_bitmap;
static GBitmap *s_mic_white_bitmap;
// One session for the app's whole lifetime (created in window_load,
// destroyed in window_unload) - the SDK's own dictation_session_create doc
// confirms a session "can be used more than once" and "can be restarted
// multiple times after the UI is exited or stopped", so there's no need to
// recreate it per "Add Task" press.
static DictationSession *s_dictation_session;
// dictation_session_start()'s own return value doesn't cleanly express
// "a session is already in progress" (it returns DictationSessionStatus,
// not a bool, despite the header's prose describing boolean-ish semantics) -
// this flag is a self-contained guard against a rapid double-press starting
// a second session on top of one already running, independent of whatever
// the SDK does internally. Set before starting, cleared unconditionally at
// the top of the status callback (every status, not just success).
static bool s_dictation_pending;
#endif
// A sync error's full message is otherwise only visible as a single-line,
// easily-missed subtitle on the Resync row (or squeezed into the small
// empty-state text when there's no cached list yet) - this takes over the
// whole content area instead, and - unlike everything else here, which
// just reflects whatever the latest status is - deliberately does NOT
// auto-update/dismiss itself on the next status change, so a transient
// retry succeeding underneath can't yank the message away before it's
// been read. Only an explicit Select dismisses it (see menu_select_click).
static TextLayer *s_error_layer;
static bool s_error_overlay_active = false;
#ifndef PBL_PLATFORM_APLITE
// A task's notes are shown in their own pushed Window (not a layer toggled
// on top of s_main_window, the way the error overlay is) specifically so
// Back gets Pebble's own default "pop this window" behavior for free,
// dismissing back to the task list - same reasoning, and same pattern, as
// s_habits_window's own comment ("Back always pops this back off... no
// click config needed here for it"). A layer-toggle overlay on the SAME
// window doesn't get this: Back on s_main_window always falls through to
// ITS default behavior (exit the app, since it's the bottom/only window),
// regardless of which overlay layer happens to be drawn on top at the time
// - this was a real, reported bug (Back on the notes overlay exited to the
// watchface instead of returning to the task list).
static Window *s_notes_window;
static StatusBarLayer *s_notes_status_bar;
static ScrollLayer *s_notes_scroll_layer;
static TextLayer *s_notes_layer;
static bool s_notes_overlay_active = false;
// Which task the notes overlay is currently showing - a plain id copy (not
// a Task*), same reasoning as s_pending_toggle_task_id/s_tracking_task_id
// below: a background sync can rebuild s_tasks out from under the overlay
// while it's open. Needed by long-select's voice note-append (see
// notes_window_select_long_click_handler/start_note_append_dictation) to
// know which task to send the dictated text for.
static char s_notes_overlay_task_id[MAX_ID_LEN] = "";
// Distinguishes a dictation_status_callback firing for note-append (started
// from the notes overlay's long-select) from one firing for Add Task
// (started from the section-0 row) - both share the single s_dictation_session/
// s_dictation_pending pair (see its own comment), since only one dictation
// can ever be in flight at a time regardless of which triggered it.
static bool s_dictation_is_note_append = false;
// Double-click detection on Select: a single click doesn't commit its
// task-done toggle immediately - it starts this timer instead, so a
// second click on the SAME task arriving before it fires can cancel the
// toggle and show notes instead of committing it. Tracked by id (not a
// raw Task*) because a background sync can fully rebuild s_tasks out from
// under a still-pending click (see pending_toggle_timer_callback's own
// find_task_by_id lookup) - a stale pointer into the old array would be
// undefined behavior, a stale id just fails to resolve and silently no-ops.
static AppTimer *s_pending_toggle_timer = NULL;
static char s_pending_toggle_task_id[MAX_ID_LEN] = "";
// Matches the SDK's own multi-click doc ("a value of 0 means to use the
// system default 300ms") - this app can't use window_multi_click_subscribe
// directly (MenuLayerCallbacks has no multi-click hook, and
// menu_layer_set_click_config_onto_window owns the window's whole click
// config already), so this timestamp-based approach reimplements the same
// timing convention by hand.
#define DOUBLE_CLICK_WINDOW_MS 300
#endif

// Time tracking: long-select on a task starts/stops tracking it (only one
// task at a time, mirroring the real app's single global currentTaskId -
// see task.service.ts). s_tracking_task_id is '\0' when nothing is being
// tracked. Persisted across an app close/relaunch (see save_tracking()/
// load_tracking()) rather than tied to this window's lifetime - the real
// app's "current task" isn't a UI-session concept either, and losing an
// in-progress tracked session just because the watchapp was closed for a
// minute would defeat the point of long-running tracking.
static char s_tracking_task_id[MAX_ID_LEN] = "";
static time_t s_tracking_start_epoch = 0;
static AppTimer *s_tracking_tick_timer = NULL;
#define TRACKING_TICK_INTERVAL_MS 1000

// Same idea as s_tracking_task_id above, but for a StopWatch-type habit -
// kept as its own independent slot (not reusing s_tracking_task_id) since
// the real app's own currentTaskId (task) and SimpleCounter.isOn (habit)
// are independent pieces of state too: tracking a task and a habit
// stopwatch at the same time is a real, valid scenario there, not a
// conflict to resolve. Unlike s_tracking_tick_timer (which redraws
// s_menu_layer, a layer that exists for the app's whole lifetime), this
// one's tick timer only runs while the habits window is actually loaded -
// see habits_window_load/unload - since s_habits_menu_layer is torn down
// and rebuilt on every visit to that window.
// Compiled out entirely on aplite (#ifndef, matching s_mic_bitmap's own
// precedent for the same reason): the tracking machinery (persisted
// start/stop state, tick timer, the functions below) pushed aplite's
// already-tight budget 820 bytes over its .bss region even with
// MAX_HABITS already down to 3 there. A StopWatch habit still shows its
// "value / goal" progress read-only on aplite (habits_menu_draw_row) -
// just without the ability to start/stop tracking from the watch itself.
#ifndef PBL_PLATFORM_APLITE
static char s_tracking_habit_id[MAX_HABIT_ID_LEN] = "";
static time_t s_tracking_habit_start_epoch = 0;
static AppTimer *s_habit_tracking_tick_timer = NULL;
// Select pauses/resumes an in-progress RepeatedCountdownReminder round
// (long-select still cancels it outright, same as it already does for a
// StopWatch) - meaningless for a StopWatch's open-ended up-count, so this
// only ever applies when s_tracking_habit_id refers to an is_countdown
// habit. While paused, s_tracking_habit_start_epoch stops mattering (the
// tick timer itself is stopped - see toggle_habit_countdown_pause) and
// s_habit_countdown_frozen_elapsed_ms holds the total elapsed so far;
// while running, it holds everything accumulated BEFORE the current
// running segment, with s_tracking_habit_start_epoch marking where that
// segment began - see countdown_elapsed_ms().
static bool s_habit_countdown_paused = false;
static int s_habit_countdown_frozen_elapsed_ms = 0;
#endif

static Task s_tasks[MAX_TASKS];
static int s_task_count = 0;      // tasks currently shown (committed)
static int s_incoming_total = 0;  // total announced by the current sync batch
static Task s_incoming[MAX_TASKS];
static int s_status_code = STATUS_SYNCING;
#define MAX_STATUS_MSG_LEN 64
static char s_status_msg[MAX_STATUS_MSG_LEN] = "";
// Phone-side settings, mirrored here via optional fields on MSG_SYNC_STATUS
// (see inbox_received_handler) - default true so a freshly-installed/
// not-yet-synced watch behaves exactly as it always has until a real sync
// says otherwise. Plain unconditional statics (not inside the mic-only
// #ifndef PBL_PLATFORM_APLITE block below) - trivial size even on aplite,
// and keeps that block's scope narrowly mic-specific. s_add_task_enabled is
// inert on aplite regardless of its value: PBL_IF_MICROPHONE_ELSE already
// keeps the Add Task row permanently absent there.
static bool s_habits_enabled = true;
static bool s_add_task_enabled = true;
// Backlight override, same phone-settings mirroring convention as the two
// flags above: 0 (system default) until the first sync says otherwise, so
// an unconfigured/never-synced watch never touches the backlight API at
// all - this app shipped for a long time with zero light_enable() calls
// anywhere, and that stays the behavior unless the phone opts into
// something else. A negative value (BACKLIGHT_MODE_ALWAYS_ON) forces the
// backlight on for as long as the app stays open; a positive value is a
// custom relight-and-hold duration in seconds, applied after any button
// press - see backlight_touch()/apply_backlight_mode() below.
//
// #ifndef PBL_PLATFORM_APLITE-gated entirely, same as the StopWatch habit
// timer above - confirmed via pebble build's own memory usage report that
// aplite had all of 10 bytes of free RAM left BEFORE this feature existed
// (every other platform: 37KB+), so anything added here has to cost
// aplite exactly zero, not "a little". backlight_touch() becomes a no-op
// macro on aplite (below) so none of its 8 call sites elsewhere in this
// file need their own #ifdef.
#ifndef PBL_PLATFORM_APLITE
#define BACKLIGHT_MODE_ALWAYS_ON -1
static int32_t s_backlight_mode = 0;
static AppTimer *s_backlight_timer = NULL;
#endif

// Mirrors the phone's "Sync automatically on a timer" pairing setting
// (config.autoSyncIntervalMin), same 0-until-first-sync-says-otherwise
// convention as s_backlight_mode above. This is what schedule_next_wakeup()
// uses to periodically relaunch the app via wakeup_schedule() so a sync can
// happen even while the app is closed - PebbleKit JS (where all the actual
// networking lives) only runs while THIS app is the one currently open, so
// a phone-side setInterval alone (what this setting used to rely on
// exclusively) never fires once the watch has moved on to the watchface or
// another app. See schedule_next_wakeup()'s own comment for the rest of
// the mechanism.
//
// #ifndef PBL_PLATFORM_APLITE-gated entirely, same reasoning (and the same
// confirmed-via-pebble-build's-own-memory-report standard) as every other
// aplite exclusion in this file: aplite was already at its ceiling before
// this feature existed, and this alone overflowed it by 332 bytes. Aplite
// keeps the setting's old, more limited behavior (only actually syncs on
// this schedule while the app happens to be open) rather than gaining
// wakeup-based background sync.
#ifndef PBL_PLATFORM_APLITE
static int32_t s_auto_sync_interval_min = 0;
// Set once at init() from launch_reason() and never changed after - detects
// whether THIS session exists because a wakeup fired (as opposed to the
// user opening the app normally), which is what gates the auto-exit in
// inbox_received_handler's MSG_SYNC_STATUS case below. A wakeup-launched
// session's whole purpose is a quiet sync-and-return; a manually opened one
// should behave exactly as it always has and stay open.
static bool s_is_wakeup_launch = false;
// Guards schedule_next_wakeup() from running on every single status push
// this session (SYNCING, then OK/ERROR, plus one per watch-initiated
// action) - only needed once the interval is first confirmed after launch,
// and again if it later actually changes (e.g. a live settings save).
static bool s_wakeup_rescheduled_this_launch = false;
// Guards window_stack_pop_all() from firing more than once if multiple
// terminal statuses arrive in the same wakeup-launched session (e.g. a
// retried sync after an initial error).
static bool s_wakeup_exit_triggered = false;
#endif

static TaskGroup s_groups[MAX_TASKS]; // worst case: every task its own group
static int s_group_count = 0;

// Marquee-scrolls the title of whichever task row is currently selected,
// if (and only if) it's too wide to fit - MenuLayer has no built-in
// scrolling-text cell, and truncating with an ellipsis was the only
// alternative. Only the selected row ever scrolls (not every long row at
// once): cheaper to redraw, and it's the row the user is actually looking
// at.
#define SCROLL_INTERVAL_MS 300
#define SCROLL_STEP_PX 6
#define SCROLL_GAP_PX 24
#define TITLE_FONT_KEY FONT_KEY_GOTHIC_18_BOLD
// One step up from FONT_KEY_GOTHIC_14 (Pebble's system fonts only come in
// fixed sizes, no arbitrary point sizes) - used for a task row's due/
// tracked-time/"Done" subtitle and a habit row's value/goal subtitle
// specifically, not every small-text use in the app (Resync's status line,
// the Finish Day row's version subtitle, and the empty-list version footer
// stay at FONT_KEY_GOTHIC_14 - those read more as app chrome than content
// the user is actively scanning row to row).
#define SUBTITLE_FONT_KEY FONT_KEY_GOTHIC_18
#define TITLE_BOX_X 6
#define TITLE_BOX_Y 2

static AppTimer *s_scroll_timer = NULL;
static int s_scroll_offset_px = 0;

// Cycles "Syncing" -> "Syncing." -> "Syncing.." -> "Syncing..." on the empty
// screen while the very first sync (no cached task list yet) is in flight -
// otherwise that screen just sits on static text with no sign anything is
// happening, for however long the initial full op-log replay takes.
#define SYNCING_ANIM_INTERVAL_MS 400
static AppTimer *s_syncing_timer = NULL;
static int s_syncing_dots = 0;

static void recompute_groups(void) {
  s_group_count = 0;
  int i = 0;
  while (i < s_task_count && s_group_count < MAX_TASKS) {
    int j = i + 1;
    while (j < s_task_count && strncmp(s_tasks[j].project, s_tasks[i].project, MAX_PROJECT_LEN) == 0) {
      j++;
    }
    strncpy(s_groups[s_group_count].name, s_tasks[i].project, MAX_PROJECT_LEN - 1);
    s_groups[s_group_count].name[MAX_PROJECT_LEN - 1] = '\0';
    s_groups[s_group_count].start = i;
    s_groups[s_group_count].count = j - i;
    s_group_count++;
    i = j;
  }
}

static const uint32_t PERSIST_KEY_TASKS = 100;

// ---------- persistence (so the list survives a watchapp relaunch) ----------

static void save_tasks(void) {
  if (s_task_count > 0) {
    persist_write_data(PERSIST_KEY_TASKS, s_tasks, sizeof(Task) * (size_t)s_task_count);
    persist_write_int(PERSIST_KEY_TASKS + 1, s_task_count);
  } else {
    persist_delete(PERSIST_KEY_TASKS);
    persist_delete(PERSIST_KEY_TASKS + 1);
  }
}

static void load_tasks(void) {
  if (persist_exists(PERSIST_KEY_TASKS + 1)) {
    int count = persist_read_int(PERSIST_KEY_TASKS + 1);
    if (count > 0 && count <= MAX_TASKS) {
      int bytes = persist_read_data(PERSIST_KEY_TASKS, s_tasks, sizeof(Task) * (size_t)count);
      if (bytes == (int)(sizeof(Task) * (size_t)count)) {
        s_task_count = count;
      }
    }
  }
}

static const uint32_t PERSIST_KEY_HABITS = 120;

static void save_habits(void) {
  if (s_habit_count > 0) {
    persist_write_data(PERSIST_KEY_HABITS, s_habits, sizeof(Habit) * (size_t)s_habit_count);
    persist_write_int(PERSIST_KEY_HABITS + 1, s_habit_count);
  } else {
    persist_delete(PERSIST_KEY_HABITS);
    persist_delete(PERSIST_KEY_HABITS + 1);
  }
}

static void load_habits(void) {
  if (persist_exists(PERSIST_KEY_HABITS + 1)) {
    int count = persist_read_int(PERSIST_KEY_HABITS + 1);
    if (count > 0 && count <= MAX_HABITS) {
      int bytes = persist_read_data(PERSIST_KEY_HABITS, s_habits, sizeof(Habit) * (size_t)count);
      if (bytes == (int)(sizeof(Habit) * (size_t)count)) {
        s_habit_count = count;
      }
    }
  }
}

static const uint32_t PERSIST_KEY_TRACKING_ID = 110;
static const uint32_t PERSIST_KEY_TRACKING_START = 111;

// Saved/loaded as its own pair of keys, independent of save_tasks()/
// load_tasks(), so a tracked session survives even across a resync that
// replaces s_tasks wholesale (MSG_TASK_SYNC_END).
static void save_tracking(void) {
  if (s_tracking_task_id[0] != '\0') {
    persist_write_string(PERSIST_KEY_TRACKING_ID, s_tracking_task_id);
    persist_write_int(PERSIST_KEY_TRACKING_START, (int)s_tracking_start_epoch);
  } else {
    persist_delete(PERSIST_KEY_TRACKING_ID);
    persist_delete(PERSIST_KEY_TRACKING_START);
  }
}

static void load_tracking(void) {
  if (persist_exists(PERSIST_KEY_TRACKING_ID)) {
    persist_read_string(PERSIST_KEY_TRACKING_ID, s_tracking_task_id, sizeof(s_tracking_task_id));
    s_tracking_start_epoch = (time_t)persist_read_int(PERSIST_KEY_TRACKING_START);
  }
}

#ifndef PBL_PLATFORM_APLITE
static const uint32_t PERSIST_KEY_HABIT_TRACKING_ID = 130;
static const uint32_t PERSIST_KEY_HABIT_TRACKING_START = 131;
static const uint32_t PERSIST_KEY_HABIT_COUNTDOWN_PAUSED = 132;
static const uint32_t PERSIST_KEY_HABIT_COUNTDOWN_FROZEN_MS = 133;

// Mirrors save_tracking()/load_tracking() above, for a tracked StopWatch/
// countdown habit instead of a tracked task - see s_tracking_habit_id's own
// comment on why this is a separate slot rather than reusing the task one,
// and on why this whole block is compiled out on aplite. The pause fields
// are only ever meaningful for an is_countdown session (see
// s_habit_countdown_paused's own comment) but are harmless to persist
// unconditionally alongside it - StopWatch sessions just always save/load
// paused: false.
static void save_habit_tracking(void) {
  if (s_tracking_habit_id[0] != '\0') {
    persist_write_string(PERSIST_KEY_HABIT_TRACKING_ID, s_tracking_habit_id);
    persist_write_int(PERSIST_KEY_HABIT_TRACKING_START, (int)s_tracking_habit_start_epoch);
    persist_write_int(PERSIST_KEY_HABIT_COUNTDOWN_PAUSED, s_habit_countdown_paused ? 1 : 0);
    persist_write_int(PERSIST_KEY_HABIT_COUNTDOWN_FROZEN_MS, s_habit_countdown_frozen_elapsed_ms);
  } else {
    persist_delete(PERSIST_KEY_HABIT_TRACKING_ID);
    persist_delete(PERSIST_KEY_HABIT_TRACKING_START);
    persist_delete(PERSIST_KEY_HABIT_COUNTDOWN_PAUSED);
    persist_delete(PERSIST_KEY_HABIT_COUNTDOWN_FROZEN_MS);
  }
}

static void load_habit_tracking(void) {
  if (persist_exists(PERSIST_KEY_HABIT_TRACKING_ID)) {
    persist_read_string(PERSIST_KEY_HABIT_TRACKING_ID, s_tracking_habit_id, sizeof(s_tracking_habit_id));
    s_tracking_habit_start_epoch = (time_t)persist_read_int(PERSIST_KEY_HABIT_TRACKING_START);
    s_habit_countdown_paused = persist_exists(PERSIST_KEY_HABIT_COUNTDOWN_PAUSED) &&
                                persist_read_int(PERSIST_KEY_HABIT_COUNTDOWN_PAUSED) != 0;
    s_habit_countdown_frozen_elapsed_ms = persist_exists(PERSIST_KEY_HABIT_COUNTDOWN_FROZEN_MS) ?
                                           persist_read_int(PERSIST_KEY_HABIT_COUNTDOWN_FROZEN_MS) : 0;
  }
}
#endif

static void request_sync(void);
static void hide_error_overlay(void);
static void push_habits_window(void);
static void update_habits_empty_layer(void);
#ifndef PBL_PLATFORM_APLITE
static void backlight_touch(void);
#else
#define backlight_touch() ((void)0)
#endif
#ifndef PBL_PLATFORM_APLITE
static void show_notes_overlay(Task *task);
static void hide_notes_overlay(void);
static void push_notes_window(void);
static void pending_toggle_timer_callback(void *data);
#endif
#ifndef PBL_PLATFORM_APLITE
static void start_add_task_dictation(void);
static void start_note_append_dictation(void);
#endif

// ---------- menu layer callbacks ----------

// Section 0 is always the "Resync" action, plus "Habits" and/or "Add Task"
// when those features are enabled (settings toggled from the phone - see
// s_habits_enabled/s_add_task_enabled) - a DYNAMIC row count (1 to 3), not a
// fixed one. section0_row_count()/section0_row_kind() are the single shared
// source of truth for this layout - menu_get_num_rows/menu_draw_row/
// menu_select_click all defer to them so they can never disagree about
// which row index means what. Add Task also stays gated by
// PBL_IF_MICROPHONE_ELSE regardless of s_add_task_enabled (aplite has no
// mic hardware and never reports/draws/routes clicks to that row at all).
// When there are tasks, sections 1..s_group_count are one per project group
// (see recompute_groups()), followed by one final section (group_idx ==
// s_group_count) holding a single "Finish Day" row - always the very last
// row in the list, centered like the version text it also still shows (as
// a subtitle now, not the row's sole content). Long-select archives every
// currently-done task (see menu_select_long_click, send_finish_day, and
// handleFinishDay in index.js); plain Select is a no-op on this row, same
// as it's always been - Finish Day is a deliberate, not-easily-undone
// action, so it uses this app's existing "long-select is the more
// deliberate gesture" convention (task/habit time tracking) rather than
// the single-tap Select every other primary action uses. When the list is
// empty, there are no further sections (no Finish Day row on that screen
// either - there's nothing to have finished): for STATUS_NOT_PAIRED/ERROR/
// initial-syncing, section 0 doubles as the empty/error screen's single
// phantom retry row (see menu_get_num_rows); for STATUS_OK (e.g. Today Only
// with nothing due) on a platform ACTIONABLE_EMPTY_ACTIVE() allows, section 0
// instead shows its normal Resync/Habits/Add Task rows with "No tasks for
// today." as its header (see update_empty_layer's show_actionable_empty).
typedef enum {
  SECTION0_ROW_RESYNC,
  SECTION0_ROW_HABITS,
  SECTION0_ROW_ADD_TASK,
} Section0RowKind;

// Whether the STATUS_OK/zero-tasks empty state (e.g. Today Only with
// nothing due) shows section 0's normal, still-interactive rows (with a
// header carrying the "No tasks for today." message) instead of the old
// single hidden phantom retry row. Aplite unconditionally keeps the old
// behavior - the extra header/row-count/click-routing logic this needs
// pushed a debug build 176 bytes past aplite's combined flash+RAM "APP"
// region even with no new persistent state, confirmed via pebble build's
// own memory usage report (same tight-budget reasoning as MAX_HABITS/
// MAX_ID_LEN's own comments). A compile-time-constant macro (same idiom as
// the SDK's own PBL_IF_MICROPHONE_ELSE, already used above for Add Task)
// rather than a runtime check, so the dead branches it guards are fully
// eliminated on aplite instead of merely unreachable.
#ifdef PBL_PLATFORM_APLITE
#define ACTIONABLE_EMPTY_ACTIVE() false
#else
#define ACTIONABLE_EMPTY_ACTIVE() (s_status_code == STATUS_OK)
#endif

static int section0_row_count(void) {
  int count = 1; // Resync always present.
  if (s_habits_enabled) {
    count++;
  }
  if (PBL_IF_MICROPHONE_ELSE(s_add_task_enabled, false)) {
    count++;
  }
  return count;
}

// Maps a section-0 row index to which action occupies it. Resync is always
// row 0; Habits (if enabled) then Add Task (if enabled) fill in after it in
// that fixed order - matches section0_row_count()'s own counting order.
static Section0RowKind section0_row_kind(int row) {
  if (row == 0) {
    return SECTION0_ROW_RESYNC;
  }
  int next = 1;
  if (s_habits_enabled) {
    if (row == next) {
      return SECTION0_ROW_HABITS;
    }
    next++;
  }
  if (PBL_IF_MICROPHONE_ELSE(s_add_task_enabled, false)) {
    if (row == next) {
      return SECTION0_ROW_ADD_TASK;
    }
  }
  return SECTION0_ROW_RESYNC; // unreachable for any row menu_get_num_rows reported
}

static uint16_t menu_get_num_sections(MenuLayer *menu_layer, void *context) {
  return s_task_count > 0 ? (uint16_t)(1 + s_group_count + 1) : 1;
}

static uint16_t menu_get_num_rows(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  if (s_task_count == 0) {
    // STATUS_OK with zero tasks (e.g. Today Only filtering out everything
    // due) is an "actionable" empty state - see update_empty_layer's
    // show_actionable_empty - so the menu layer stays visible with its
    // normal section-0 rows (Resync/Habits/Add Task) reachable, same as the
    // populated list. Every other empty reason (not paired/error/initial
    // syncing) keeps the menu hidden behind s_empty_layer, but it still owns
    // the window's click config, so it needs at least one reportable row
    // for SELECT to be dispatched at all - otherwise "Select to retry" on
    // the error screen is dead text. Never actually drawn in that case
    // since the layer is hidden.
    return ACTIONABLE_EMPTY_ACTIVE() ? (uint16_t)section0_row_count() : 1;
  }
  if (section_index == 0) {
    return (uint16_t)section0_row_count();
  }
  int group_idx = (int)section_index - 1;
  if (group_idx == s_group_count) {
    return 1; // Finish Day row
  }
  if (group_idx > s_group_count) {
    return 0;
  }
  return (uint16_t)s_groups[group_idx].count;
}

static int16_t menu_get_header_height(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  if (s_task_count == 0) {
    // Actionable empty state only (see menu_get_num_rows) - the header is
    // where "No tasks for today." itself lives once the menu takes over
    // from s_empty_layer, sized the same as a project group's own header.
    return (section_index == 0 && ACTIONABLE_EMPTY_ACTIVE()) ? 40 : 0;
  }
  if (section_index == 0) {
    return 0;
  }
  int group_idx = (int)section_index - 1;
  // An empty group name means grouping is off (every task collapsed into
  // one '' group) - no header, so this looks exactly like the flat list
  // this had before grouping existed.
  if (group_idx >= s_group_count || s_groups[group_idx].name[0] == '\0') {
    return 0;
  }
  return 40;
}

static void menu_draw_header(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *context) {
  if (s_task_count == 0) {
    if (section_index == 0 && ACTIONABLE_EMPTY_ACTIVE()) {
      // Actionable empty state (see menu_get_num_rows/update_empty_layer) -
      // the "No tasks for today." message that used to live alone on
      // s_empty_layer now sits above the still-reachable Resync/Habits/Add
      // Task rows instead.
      GRect bounds = layer_get_bounds(cell_layer);
      graphics_context_set_fill_color(ctx, GColorWhite);
      graphics_fill_rect(ctx, bounds, 0, GCornerNone);
      graphics_context_set_text_color(ctx, GColorBlack);
      graphics_draw_text(ctx, "No tasks for today.", fonts_get_system_font(FONT_KEY_GOTHIC_18),
                          bounds, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
    }
    return;
  }
  if (section_index == 0) {
    return;
  }
  int group_idx = (int)section_index - 1;
  if (group_idx >= s_group_count || s_groups[group_idx].name[0] == '\0') {
    return;
  }
  const char *name = s_groups[group_idx].name;
  GRect bounds = layer_get_bounds(cell_layer);
  GFont bold_font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  GRect text_rect = GRect(6, 2, bounds.size.w - 12, bounds.size.h - 4);

  // MenuLayer headers, like rows, own their whole cell background - a
  // header has no built-in fill of its own, so this has to happen before
  // the text/lines below or they'd draw onto whatever was already in the
  // framebuffer instead of a solid green cell.
  graphics_context_set_fill_color(ctx, GColorGreen);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, name, bold_font, text_rect,
                      GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  // Underline sized to the actual rendered text, directly beneath it.
  GSize text_size = graphics_text_layout_get_content_size(
      name, bold_font, text_rect, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
  int16_t underline_y = 2 + text_size.h;
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_draw_line(ctx, GPoint(6, underline_y), GPoint(6 + text_size.w, underline_y));

  // A second, full-width divider along the bottom of the header cell -
  // separates this group from its tasks more clearly than the text-width
  // underline alone, especially once several groups are on screen at once.
  int16_t divider_y = bounds.size.h - 2;
  graphics_draw_line(ctx, GPoint(0, divider_y), GPoint(bounds.size.w, divider_y));
}

// Looks up a task by id (e.g. to re-find whatever's being tracked, since
// s_tracking_task_id survives across a resync that reallocates s_tasks
// wholesale, so any Task* captured at start_tracking() time can't be
// trusted to still be valid).
static Task *find_task_by_id(const char *id) {
  for (int i = 0; i < s_task_count; i++) {
    if (strncmp(s_tasks[i].id, id, MAX_ID_LEN) == 0) {
      return &s_tasks[i];
    }
  }
  return NULL;
}

#ifndef PBL_PLATFORM_APLITE
// Same idea as find_task_by_id, for the habits list - only needed by the
// habit-tracking functions below, which are themselves compiled out on
// aplite (see s_tracking_habit_id's own comment).
static Habit *find_habit_by_id(const char *id) {
  for (int i = 0; i < s_habit_count; i++) {
    if (strncmp(s_habits[i].id, id, MAX_HABIT_ID_LEN) == 0) {
      return &s_habits[i];
    }
  }
  return NULL;
}
#endif

// Resolves a MenuIndex to the Task it points at, or NULL if it isn't on a
// task row at all (the Resync row, a group header, or nothing there once
// s_group_count/s_task_count are taken into account - recompute_groups()
// leaves s_group_count at 0 whenever s_task_count is 0, so an empty list is
// handled by the same group_idx bounds check as any other out-of-range row).
static Task *resolve_task_at(MenuIndex index) {
  if (index.section == 0) {
    return NULL;
  }
  int group_idx = (int)index.section - 1;
  if (group_idx >= s_group_count) {
    return NULL;
  }
  int task_idx = s_groups[group_idx].start + (int)index.row;
  if (task_idx < 0 || task_idx >= s_task_count) {
    return NULL;
  }
  return &s_tasks[task_idx];
}

// Resolves the row MenuLayer currently has highlighted to a Task, or NULL
// if the selection isn't on a task row at all.
static Task *resolve_selected_task(void) {
  return resolve_task_at(menu_layer_get_selected_index(s_menu_layer));
}

static int16_t title_natural_width(const char *title) {
  GSize size = graphics_text_layout_get_content_size(
      title, fonts_get_system_font(TITLE_FONT_KEY), GRect(0, 0, 2000, 100),
      GTextOverflowModeFill, GTextAlignmentLeft);
  return size.w;
}

// Formats due_min (minutes since local midnight) as "@ 9:41 AM"/"@ 21:41" -
// respecting the watch's own 12h/24h clock setting, since due_min itself is
// timezone-less (the phone already converted to local time before sending
// it - see sendTaskAt() in index.js).
static void format_due_time(int due_min, char *out, size_t out_len) {
  int h = due_min / 60;
  int m = due_min % 60;
  if (clock_is_24h_style()) {
    snprintf(out, out_len, "@ %d:%02d", h, m);
  } else {
    int h12 = h % 12;
    if (h12 == 0) {
      h12 = 12;
    }
    snprintf(out, out_len, "@ %d:%02d %s", h12, m, h < 12 ? "AM" : "PM");
  }
}

// Formats elapsed tracked time as "1h 23m" (>= 1 hour, seconds dropped to
// keep it compact - this ticks every second while tracking, but a second
// digit isn't useful once the total is measured in hours), "5m 09s"
// (>= 1 minute), or "42s". is_tracking prefixes a plain ASCII "> " marker
// so a live-ticking number reads unambiguously as "currently running"
// versus a static previously-accumulated total - plain ASCII here just
// because there's no need for anything fancier, not because this codebase
// avoids non-ASCII glyphs in general (see the subtask marker's own comment,
// task-store.js's SUBTASK_PREFIX, for glyphs confirmed to render fine).
static void format_duration_ms(int ms, bool is_tracking, char *out, size_t out_len) {
  int total_s = ms / 1000;
  int h = total_s / 3600;
  int m = (total_s % 3600) / 60;
  int s = total_s % 60;
  const char *prefix = is_tracking ? "> " : "";
  if (h > 0) {
    snprintf(out, out_len, "%s%dh %02dm", prefix, h, m);
  } else if (m > 0) {
    snprintf(out, out_len, "%s%dm %02ds", prefix, m, s);
  } else {
    snprintf(out, out_len, "%s%ds", prefix, s);
  }
}

static void stop_scroll_timer(void) {
  if (s_scroll_timer) {
    app_timer_cancel(s_scroll_timer);
    s_scroll_timer = NULL;
  }
}

static void scroll_timer_callback(void *data) {
  s_scroll_offset_px += SCROLL_STEP_PX;
  layer_mark_dirty(menu_layer_get_layer(s_menu_layer));
  s_scroll_timer = app_timer_register(SCROLL_INTERVAL_MS, scroll_timer_callback, NULL);
}

// Starts/stops the marquee timer to match whether the currently-selected
// row actually needs it, and (optionally) resets the scroll position -
// called whenever the selection moves or the task list is reloaded, since
// either can change which title (if any) needs to scroll.
static void refresh_scroll_state(bool reset_offset) {
  if (reset_offset) {
    s_scroll_offset_px = 0;
  }
  Task *selected = resolve_selected_task();
  GRect menu_bounds = layer_get_bounds(menu_layer_get_layer(s_menu_layer));
  int16_t available = menu_bounds.size.w - TITLE_BOX_X * 2;
  bool needs_scroll = selected && title_natural_width(selected->title) > available;
  if (needs_scroll && !s_scroll_timer) {
    s_scroll_timer = app_timer_register(SCROLL_INTERVAL_MS, scroll_timer_callback, NULL);
  } else if (!needs_scroll) {
    stop_scroll_timer();
  }
}

static void menu_selection_changed(MenuLayer *menu_layer, MenuIndex new_index, MenuIndex old_index, void *context) {
  refresh_scroll_state(true);
  backlight_touch();
}

#ifndef PBL_PLATFORM_APLITE
static void backlight_timer_callback(void *data) {
  s_backlight_timer = NULL;
  // Hands control back to the system's own automatic backlight behavior -
  // NOT "force off". Since this fires with no real button press happening
  // at that exact instant, automatic control has nothing to keep it lit
  // for, so in practice this is what makes the "custom timeout" duration
  // real: the light goes dark right as this timer expires instead of
  // lingering under whichever duration the user's own watch Settings
  // happen to specify.
  light_enable(false);
}

// Called on every button interaction relevant to whichever menu is
// visible (select/long-select clicks and up/down scroll - see call sites
// in both windows' click callbacks below) - NOT on a settings change
// itself, see apply_backlight_mode() for that. A mode of 0 (system
// default) is a deliberate no-op: this app goes back to never touching the
// backlight API at all unless the phone has explicitly opted into one of
// the other two modes, same as every other watch-side feature toggle
// defaulting to inert.
static void backlight_touch(void) {
  if (s_backlight_timer) {
    app_timer_cancel(s_backlight_timer);
    s_backlight_timer = NULL;
  }
  if (s_backlight_mode == 0) {
    return;
  }
  light_enable(true);
  if (s_backlight_mode == BACKLIGHT_MODE_ALWAYS_ON) {
    return; // Stays on until the mode itself changes - see apply_backlight_mode().
  }
  s_backlight_timer = app_timer_register(s_backlight_mode * 1000, backlight_timer_callback, NULL);
}

// Reacts to s_backlight_mode itself changing (a settings save that arrives
// while the app is already open, via MSG_SYNC_STATUS below) - not to user
// interaction, see backlight_touch() for that. Leaving always-on needs an
// explicit light_enable(false) here since nothing else ever calls it:
// backlight_touch() only ever turns the light ON, the timeout timer is the
// only thing that turns it back off, and that timer never runs in
// always-on mode.
static void apply_backlight_mode(void) {
  if (s_backlight_mode == 0) {
    if (s_backlight_timer) {
      app_timer_cancel(s_backlight_timer);
      s_backlight_timer = NULL;
    }
    light_enable(false);
    return;
  }
  backlight_touch();
}
#endif // !PBL_PLATFORM_APLITE

#ifndef PBL_PLATFORM_APLITE
// (Re)arms the single wakeup event this app ever uses, relative to NOW -
// called once per launch as soon as s_auto_sync_interval_min is confirmed
// by a sync status, and again whenever it actually changes (see
// inbox_received_handler's MSG_SYNC_STATUS case). wakeup_cancel_all() first
// is deliberate and safe: this is the only feature in this app that uses
// the wakeup API at all, so there's never a second, unrelated wakeup to
// preserve - and re-arming "interval minutes from whatever just happened"
// on every confirmed sync (manual open, watch-initiated action, or a prior
// wakeup's own sync) means the NEXT background wakeup is always exactly
// one full interval past the most recent real activity, never sooner.
// notify_if_missed is false: this is a quiet resync, not a reminder the
// user needs a "you missed N notifications while your watch was off" alert
// for if it doesn't fire exactly on time.
static void schedule_next_wakeup(void) {
  wakeup_cancel_all();
  if (s_auto_sync_interval_min <= 0) {
    return;
  }
  time_t next = time(NULL) + s_auto_sync_interval_min * 60;
  WakeupId id = wakeup_schedule(next, 0, false);
  if (id < 0) {
    APP_LOG(APP_LOG_LEVEL_WARNING, "wakeup_schedule failed: %d", (int)id);
  } else {
    APP_LOG(APP_LOG_LEVEL_INFO, "wakeup scheduled for +%ld min (id %d)", (long)s_auto_sync_interval_min, (int)id);
  }
}
#endif // !PBL_PLATFORM_APLITE

static void menu_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context) {
  if (s_task_count == 0 && !ACTIONABLE_EMPTY_ACTIVE()) {
    // Menu stays hidden for every empty reason except the actionable one
    // (see menu_get_num_rows) - nothing to draw. In the actionable case,
    // section 0's Resync/Habits/Add Task rows below draw exactly as they
    // do for a populated list; menu_get_num_sections never reports more
    // than section 0 while s_task_count is 0, so nothing past this branch
    // is reachable here either way.
    return;
  }
  if (cell_index->section == 0) {
    bool is_selected = menu_layer_get_selected_index(s_menu_layer).section == cell_index->section &&
                        menu_layer_get_selected_index(s_menu_layer).row == cell_index->row;
    GRect bounds = layer_get_bounds(cell_layer);
    Section0RowKind kind = section0_row_kind((int)cell_index->row);

    if (kind == SECTION0_ROW_HABITS) {
      // Navigates to the habits (SimpleCounter) page - see
      // push_habits_window(). Icon matches the real app's own "heart_check"
      // icon for this same feature (magic-nav-config.service.ts).
      graphics_context_set_fill_color(ctx, GColorVividCerulean);
      graphics_fill_rect(ctx, bounds, 0, GCornerNone);
      graphics_context_set_text_color(ctx, is_selected ? GColorWhite : GColorBlack);
      GRect title_box = GRect(TITLE_BOX_X, TITLE_BOX_Y, bounds.size.w - TITLE_BOX_X * 2 - ROW_ICON_SIZE - 8, 30);
      graphics_draw_text(ctx, "Habits", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), title_box,
                          GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
      GRect icon_rect = GRect(bounds.size.w - ROW_ICON_SIZE - 10, (bounds.size.h - ROW_ICON_SIZE) / 2,
                               ROW_ICON_SIZE, ROW_ICON_SIZE);
      // GCompOpSet (not the GCompOpAssign default) is what makes the
      // bitmap's own alpha channel - the transparent background and the
      // checkmark cutout - actually take effect here, same as
      // bitmap_layer_set_compositing_mode(..., GCompOpSet) does for
      // s_logo_layer above; a raw graphics_draw_bitmap_in_rect call doesn't
      // inherit that, it has its own context-level compositing mode.
      graphics_context_set_compositing_mode(ctx, GCompOpSet);
      graphics_draw_bitmap_in_rect(ctx, is_selected ? s_heart_white_bitmap : s_heart_bitmap, icon_rect);
      return;
    }

#ifndef PBL_PLATFORM_APLITE
    if (kind == SECTION0_ROW_ADD_TASK) {
      // Only reachable on mic-equipped platforms with the feature enabled -
      // section0_row_kind() never returns this on aplite, or when
      // s_add_task_enabled is false, or (on aplite specifically) not even
      // compiled in - see s_mic_bitmap's own comment. Starts dictation via
      // menu_select_click; see start_add_task_dictation()/
      // dictation_status_callback below.
      graphics_context_set_fill_color(ctx, GColorJaegerGreen);
      graphics_fill_rect(ctx, bounds, 0, GCornerNone);
      graphics_context_set_text_color(ctx, is_selected ? GColorWhite : GColorBlack);
      GRect title_box = GRect(TITLE_BOX_X, TITLE_BOX_Y, bounds.size.w - TITLE_BOX_X * 2 - ROW_ICON_SIZE - 8, 30);
      graphics_draw_text(ctx, "Add Task", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), title_box,
                          GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
      GRect icon_rect = GRect(bounds.size.w - ROW_ICON_SIZE - 10, (bounds.size.h - ROW_ICON_SIZE) / 2,
                               ROW_ICON_SIZE, ROW_ICON_SIZE);
      graphics_context_set_compositing_mode(ctx, GCompOpSet);
      graphics_draw_bitmap_in_rect(ctx, is_selected ? s_mic_white_bitmap : s_mic_bitmap, icon_rect);
      return;
    }
#endif

    // Reflects live sync status so a resync failure is visible even while
    // the previously-cached list is still showing, instead of being
    // silently swallowed (only the empty/error screen showed status
    // before this row existed).
    static char s_resync_subtitle[MAX_STATUS_MSG_LEN + 16];
    const char *subtitle = "Synced";
    switch (s_status_code) {
      case STATUS_SYNCING:
        subtitle = "Syncing...";
        break;
      case STATUS_ERROR:
        if (s_status_msg[0] != '\0') {
          snprintf(s_resync_subtitle, sizeof(s_resync_subtitle), "Failed: %s", s_status_msg);
          subtitle = s_resync_subtitle;
        } else {
          subtitle = "Sync failed";
        }
        break;
      case STATUS_NOT_PAIRED:
        // Previously only shown on the empty/first-run screen (see
        // update_empty_layer) - invisible once a cached list is already on
        // screen, same silent-failure shape as the outbox case above.
        subtitle = "Not paired - open phone app";
        break;
      default:
        break;
    }
    // Bypasses menu_cell_basic_draw so this row reads as a standing
    // call-to-action rather than just another list item - background stays
    // red regardless of selection state, so it can't be mistaken for a task.
    // Text itself still inverts to white on select, matching every other
    // row's selection feedback instead of looking inert when highlighted.
    graphics_context_set_fill_color(ctx, GColorRed);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    graphics_context_set_text_color(ctx, is_selected ? GColorWhite : GColorBlack);
    GRect title_box = GRect(TITLE_BOX_X, TITLE_BOX_Y, bounds.size.w - TITLE_BOX_X * 2 - ROW_ICON_SIZE - 8, 30);
    graphics_draw_text(ctx, "Resync", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), title_box,
                        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    // Full row width (not narrowed like title_box above) - a status message
    // here ("Failed: ...") is more important not to truncate than leaving
    // room to dodge the icon, which sits up in the title row's vertical
    // band, not down here.
    GRect subtitle_box = GRect(TITLE_BOX_X, bounds.size.h - 18, bounds.size.w - TITLE_BOX_X * 2, 18);
    graphics_draw_text(ctx, subtitle, fonts_get_system_font(FONT_KEY_GOTHIC_14), subtitle_box,
                        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    // Matches the Habits row's own icon (see below) so the two pinned rows
    // read as a consistent pair - "the sp logo" (this app's checkmark
    // mark, already used elsewhere as IMAGE_MENU_ICON/IMAGE_LOGO_LARGE).
    GRect icon_rect = GRect(bounds.size.w - ROW_ICON_SIZE - 10, TITLE_BOX_Y + 2, ROW_ICON_SIZE, ROW_ICON_SIZE);
    graphics_context_set_compositing_mode(ctx, GCompOpSet);
    graphics_draw_bitmap_in_rect(ctx, is_selected ? s_check_white_bitmap : s_check_bitmap, icon_rect);
    return;
  }
  if ((int)cell_index->section - 1 == s_group_count) {
#ifndef PBL_PLATFORM_APLITE
    // Finish Day row, always last - long-select archives every currently-
    // done task (menu_select_long_click); plain Select is a no-op here,
    // same as it's always been (resolve_task_at returns NULL for a
    // group_idx past s_group_count). Inverts on selection like a real row
    // now (it wasn't interactive before this existed), matching the same
    // black-selected/white-text convention plain task rows use below.
    // Compiled out on aplite (see MAX_HABITS/s_tracking_habit_id's own
    // comments for the same reasoning) - the version-only footer this row
    // used to be stays exactly as it was there instead.
    bool is_selected = menu_layer_get_selected_index(s_menu_layer).section == cell_index->section &&
                        menu_layer_get_selected_index(s_menu_layer).row == cell_index->row;
    GRect bounds = layer_get_bounds(cell_layer);
    GColor bg = is_selected ? GColorBlack : GColorWhite;
    GColor fg = is_selected ? GColorWhite : GColorBlack;
    graphics_context_set_fill_color(ctx, bg);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    graphics_context_set_text_color(ctx, fg);
    GRect title_box = GRect(TITLE_BOX_X, TITLE_BOX_Y, bounds.size.w - TITLE_BOX_X * 2, 30);
    graphics_draw_text(ctx, "Finish Day", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), title_box,
                        GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    // Version text moves down to a subtitle instead of being the row's sole
    // content - this is the same footer slot the version display has always
    // lived in (v0.6.5), just no longer alone here.
    GRect subtitle_box = GRect(TITLE_BOX_X, bounds.size.h - 18, bounds.size.w - TITLE_BOX_X * 2, 18);
    graphics_draw_text(ctx, "v" APP_VERSION, fonts_get_system_font(FONT_KEY_GOTHIC_14), subtitle_box,
                        GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
#else
    // Plain version-only footer, unchanged from before Finish Day existed -
    // no tap/long-select action on aplite (see the #ifndef branch above).
    GRect bounds = layer_get_bounds(cell_layer);
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    graphics_context_set_text_color(ctx, GColorBlack);
    graphics_draw_text(ctx, "v" APP_VERSION, fonts_get_system_font(FONT_KEY_GOTHIC_14), bounds,
                        GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
#endif
    return;
  }
  Task *task = resolve_task_at(*cell_index);
  if (!task) {
    return;
  }

  bool is_selected = menu_layer_get_selected_index(s_menu_layer).section == cell_index->section &&
                      menu_layer_get_selected_index(s_menu_layer).row == cell_index->row;
  GRect bounds = layer_get_bounds(cell_layer);
  int16_t available = bounds.size.w - TITLE_BOX_X * 2;
  int16_t natural_width = title_natural_width(task->title);
  bool needs_marquee = is_selected && natural_width > available;

  // Every task row goes through this same custom draw path, not just the
  // marquee/done ones - menu_cell_basic_draw's own title font (used for the
  // fast path this used to take) doesn't match TITLE_FONT_KEY, so titles
  // rendered by it looked a different size than a scrolling/done title
  // drawn here. Using one font for every row keeps that consistent, and
  // means the row's own background has to be redrawn here too, using colors
  // matching the platform's own default invert-on-select cell style
  // (confirmed in the emulator: selected rows are black background/white
  // text, not white/black like an unselected row) - otherwise leftover
  // framebuffer content stays behind the text.
  GColor bg = is_selected ? GColorBlack : GColorWhite;
  GColor fg = is_selected ? GColorWhite : GColorBlack;
  // Dim a done task's title relative to its own row background (light gray
  // on the selected/black row, dark gray on a normal/white one) instead of
  // full-strength text color - this SDK has no italic system font and no
  // text-skew API to fake one, so a muted color is the achievable "done
  // looks different" cue here.
  if (task->done) {
    fg = is_selected ? GColorLightGray : GColorDarkGray;
  }
  graphics_context_set_fill_color(ctx, bg);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  graphics_context_set_text_color(ctx, fg);

  GFont title_font = fonts_get_system_font(TITLE_FONT_KEY);
  // Constrained to exactly one line's height (measured, not the row's full
  // remaining height) so GTextOverflowModeTrailingEllipsis ellipsizes at
  // the end of line 1 instead of wrapping onto a second line - confirmed in
  // the emulator that a taller box lets a long title wrap rather than
  // truncate, which is what this app wants titles to never do.
  GSize one_line_size = graphics_text_layout_get_content_size(
      "Ag", title_font, GRect(0, 0, 200, 100), GTextOverflowModeFill, GTextAlignmentLeft);
  int16_t title_box_h = one_line_size.h > 0 ? one_line_size.h : (bounds.size.h - TITLE_BOX_Y);
  GRect title_box = GRect(TITLE_BOX_X, TITLE_BOX_Y, bounds.size.w - TITLE_BOX_X * 2, title_box_h);

  if (needs_marquee) {
    int16_t period = natural_width + SCROLL_GAP_PX;
    int16_t x = -(s_scroll_offset_px % period);
    // No app-level clip-rect API exists in this SDK; relying on cell_layer's
    // own bounds to constrain rendering the way MenuLayer's own row drawing
    // already does for every other row - confirmed visually in the
    // emulator, not just assumed (see the commit this landed in).
    graphics_draw_text(ctx, task->title, title_font,
                        GRect(title_box.origin.x + x, title_box.origin.y, natural_width, title_box.size.h),
                        GTextOverflowModeFill, GTextAlignmentLeft, NULL);
    graphics_draw_text(ctx, task->title, title_font,
                        GRect(title_box.origin.x + x + period, title_box.origin.y, natural_width, title_box.size.h),
                        GTextOverflowModeFill, GTextAlignmentLeft, NULL);
  } else {
    graphics_draw_text(ctx, task->title, title_font, title_box,
                        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  }

  if (task->done) {
    GRect subtitle_box = GRect(TITLE_BOX_X, bounds.size.h - 22, bounds.size.w - TITLE_BOX_X * 2, 22);
    graphics_draw_text(ctx, "Done", fonts_get_system_font(SUBTITLE_FONT_KEY), subtitle_box,
                        GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  } else {
    // Tracked time is shown BEHIND (after) the due time when the task has
    // one, both sharing the single subtitle line - not its own row, since
    // every other subtitle here already lives in that same 22px strip.
    bool is_tracking_this = s_tracking_task_id[0] != '\0' &&
                             strncmp(s_tracking_task_id, task->id, MAX_ID_LEN) == 0;
    int effective_ms = task->time_spent_ms;
    if (is_tracking_this) {
      time_t elapsed_s = time(NULL) - s_tracking_start_epoch;
      if (elapsed_s > 0) {
        effective_ms += (int)elapsed_s * 1000;
      }
    }

    char subtitle[56] = "";
    if (task->due_min >= 0) {
      format_due_time(task->due_min, subtitle, sizeof(subtitle));
    }
    if (effective_ms > 0 || is_tracking_this) {
      char time_text[20];
      format_duration_ms(effective_ms, is_tracking_this, time_text, sizeof(time_text));
      // The estimate rides on the same time_text - "already spent / target"
      // - rather than its own separate segment, since it's only meaningful
      // alongside the timer, not on its own (see the "if visible" gate
      // below).
      if (task->time_estimate_ms > 0) {
        char estimate_text[20];
        format_duration_ms(task->time_estimate_ms, false, estimate_text, sizeof(estimate_text));
        char combined[48];
        snprintf(combined, sizeof(combined), "%s / %s", time_text, estimate_text);
        strncpy(time_text, combined, sizeof(time_text) - 1);
        time_text[sizeof(time_text) - 1] = '\0';
      }
      size_t existing_len = strlen(subtitle);
      if (existing_len > 0) {
        // A dash reads as "these are two separate, settled facts about this
        // task" (due time - time already spent). While actively tracking,
        // the ticking "> ..." number is visibly still in motion, not a
        // settled fact yet, so it keeps the plainer double-space instead -
        // the dash is reserved for the not-currently-tracking case.
        const char *separator = is_tracking_this ? "  " : " - ";
        snprintf(subtitle + existing_len, sizeof(subtitle) - existing_len, "%s%s", separator, time_text);
      } else {
        strncpy(subtitle, time_text, sizeof(subtitle) - 1);
        subtitle[sizeof(subtitle) - 1] = '\0';
      }
    }

    if (subtitle[0] != '\0') {
      GRect subtitle_box = GRect(TITLE_BOX_X, bounds.size.h - 22, bounds.size.w - TITLE_BOX_X * 2, 22);
      graphics_draw_text(ctx, subtitle, fonts_get_system_font(SUBTITLE_FONT_KEY), subtitle_box,
                          GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    }
  }
}

// ---------- outbound send retry ----------
// A watch->phone send (task toggle, resync request, ...) can fail
// transiently - APP_MSG_SEND_TIMEOUT ("the other end did not confirm
// receiving the sent data with an (n)ack in time", per pebble.h) is the
// single most common real-world cause, reported live as a "Couldn't reach
// phone app" error that clears up on its own a moment later (Bluetooth
// momentarily congested, or the phone app briefly backgrounded/relaunching
// - not an actual pairing problem). There's no public API to extend
// AppMessage's own internal ack-wait duration, so this retries the exact
// same message automatically, with a short backoff, before ever surfacing
// the fullscreen error overlay - see is_retryable_failure()/
// outbox_failed_handler() below for which failures actually get retried.
// Every watch-initiated send funnels through begin_send() (not a direct
// app_message_outbox_begin/send pair of its own) specifically so there's
// one single place that knows how to rebuild and resend the last message -
// AppMessage has no "resend this same dictionary" API, so retrying means
// reconstructing it from scratch from whatever this stashed.
//
// aplite-excluded (#ifndef, same MAX_HABITS/StopWatch-timer precedent
// elsewhere in this file): s_retry_str alone needs to be MAX_ID_LEN (96
// bytes, for a long calendar-task id) to retry every message type this
// sends, which alone blows past aplite's ~10-byte margin several times
// over. aplite keeps its original immediate-error behavior (see the
// #ifdef PBL_PLATFORM_APLITE bodies of send_task_toggle/
// send_track_time_stop/request_sync/send_habit_adjust below, and
// outbox_failed_handler's own aplite branch) - a real regression in
// resilience there specifically, but a working, honest one: an aplite user
// still sees the same accurate error and can just manually Resync/retry
// their action, same as every platform did before this.
#ifndef PBL_PLATFORM_APLITE
#define MAX_SEND_RETRIES 3
#define RETRY_BACKOFF_BASE_MS 1000
static int s_retry_msg_type = 0; // 0 = no message to retry if this send fails
// Sized for the largest of what this ever holds: a task id (MAX_ID_LEN, the
// biggest - calendar-integration ids), a habit id (MAX_HABIT_ID_LEN), or a
// dictated task title (MAX_TITLE_LEN) - MSG_REQUEST_SYNC/MSG_FINISH_DAY
// need neither and leave this empty.
static char s_retry_str[MAX_ID_LEN];
// Second string payload, only ever populated for MSG_NOTE_APPEND (the one
// message type that needs TWO strings at once - TASK_ID in s_retry_str
// above, plus this, the dictated note text to append). Every other message
// type leaves this empty; kept separate from s_retry_str rather than
// repurposing it for a second role.
static char s_retry_str2[MAX_NOTES_LEN];
static int32_t s_retry_int = 0; // TASK_DONE / TRACKED_MS / HABIT_DELTA, whichever s_retry_msg_type needs
static int s_retry_count = 0;
static AppTimer *s_retry_timer = NULL;

static void clear_pending_retry(void) {
  s_retry_msg_type = 0;
  s_retry_count = 0;
  if (s_retry_timer) {
    app_timer_cancel(s_retry_timer);
    s_retry_timer = NULL;
  }
}

// Rebuilds and (re)sends whatever message s_retry_msg_type/str/int describe
// - the single source of truth for every outbound send's actual wire
// format, used both for a message's first attempt (via begin_send) and any
// retry of it, so the two can never drift out of sync with each other.
static void send_pending_retry(void) {
  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) != APP_MSG_OK) {
    return;
  }
  dict_write_int32(iter, KEY_MSG_TYPE, s_retry_msg_type);
  switch (s_retry_msg_type) {
    case MSG_TASK_TOGGLE:
      dict_write_cstring(iter, KEY_TASK_ID, s_retry_str);
      dict_write_int32(iter, KEY_TASK_DONE, s_retry_int);
      break;
    case MSG_TRACK_TIME_STOP:
      dict_write_cstring(iter, KEY_TASK_ID, s_retry_str);
      dict_write_int32(iter, KEY_TRACKED_MS, s_retry_int);
      break;
    case MSG_HABIT_ADJUST:
      dict_write_cstring(iter, KEY_HABIT_ID, s_retry_str);
      dict_write_int32(iter, KEY_HABIT_DELTA, s_retry_int);
      break;
    case MSG_TASK_ADD:
      dict_write_cstring(iter, KEY_TASK_TITLE, s_retry_str);
      break;
    case MSG_HABIT_TRACK_STOP:
      dict_write_cstring(iter, KEY_HABIT_ID, s_retry_str);
      dict_write_int32(iter, KEY_TRACKED_MS, s_retry_int);
      break;
    case MSG_NOTE_APPEND:
      dict_write_cstring(iter, KEY_TASK_ID, s_retry_str);
      dict_write_cstring(iter, KEY_NOTE_TEXT, s_retry_str2);
      break;
    case MSG_FINISH_DAY:
    case MSG_REQUEST_SYNC:
    default:
      break; // no extra keys
  }
  app_message_outbox_send();
}

// Every watch-initiated send starts here: stash what it takes to rebuild
// this exact message (for a possible retry - see send_pending_retry), reset
// the retry count for this new attempt, and cancel any older still-pending
// retry timer so a stale retry for a since-superseded message (e.g. this
// same row toggled again before its first attempt's retry ever fired)
// doesn't also fire and resend outdated data. str_val2 is only ever
// non-NULL for MSG_NOTE_APPEND - every other message type passes NULL and
// leaves s_retry_str2 empty.
static void begin_send(int msg_type, const char *str_val, const char *str_val2, int32_t int_val) {
  if (s_retry_timer) {
    app_timer_cancel(s_retry_timer);
    s_retry_timer = NULL;
  }
  s_retry_msg_type = msg_type;
  if (str_val) {
    strncpy(s_retry_str, str_val, sizeof(s_retry_str) - 1);
    s_retry_str[sizeof(s_retry_str) - 1] = '\0';
  } else {
    s_retry_str[0] = '\0';
  }
  if (str_val2) {
    strncpy(s_retry_str2, str_val2, sizeof(s_retry_str2) - 1);
    s_retry_str2[sizeof(s_retry_str2) - 1] = '\0';
  } else {
    s_retry_str2[0] = '\0';
  }
  s_retry_int = int_val;
  s_retry_count = 0;
  send_pending_retry();
}
#endif

static void send_task_toggle(Task *task) {
#ifdef PBL_PLATFORM_APLITE
  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) != APP_MSG_OK) {
    return;
  }
  dict_write_int32(iter, KEY_MSG_TYPE, MSG_TASK_TOGGLE);
  dict_write_cstring(iter, KEY_TASK_ID, task->id);
  dict_write_int32(iter, KEY_TASK_DONE, task->done ? 1 : 0);
  app_message_outbox_send();
#else
  begin_send(MSG_TASK_TOGGLE, task->id, NULL, task->done ? 1 : 0);
#endif
}

static void send_track_time_stop(const char *task_id, int32_t tracked_ms) {
#ifdef PBL_PLATFORM_APLITE
  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) != APP_MSG_OK) {
    return;
  }
  dict_write_int32(iter, KEY_MSG_TYPE, MSG_TRACK_TIME_STOP);
  dict_write_cstring(iter, KEY_TASK_ID, task_id);
  dict_write_int32(iter, KEY_TRACKED_MS, tracked_ms);
  app_message_outbox_send();
#else
  begin_send(MSG_TRACK_TIME_STOP, task_id, NULL, tracked_ms);
#endif
}

#ifndef PBL_PLATFORM_APLITE
// Set right before send_finish_day() queues its message; outbox_sent_handler
// checks this to know a just-CONFIRMED send is the one that should close the
// app, not some unrelated message (task toggle, resync, ...) that happens to
// succeed around the same time. Closing only on confirmed send (not merely
// "queued") is deliberate: closing eagerly and having the send fail
// afterward would hide that failure behind an app that's no longer open to
// show it, defeating this app's own outbox_failed_handler fix (see
// CLAUDE.md's "AppMessage send failures used to be completely silent") for
// this one action. outbox_failed_handler clears the flag too, so a failed
// send doesn't leave it armed for some later, unrelated successful send to
// wrongly trigger a close.
static bool s_close_after_finish_day_sent = false;

// No extra keys - the watch has no way to build a full-fidelity archive
// payload itself (its own Task struct is a trimmed display projection, not
// the real task's full field set other real clients need to persist into
// their own permanent archive), so this is a bare trigger; the phone's own
// state.task cache already has everything needed - see handleFinishDay in
// index.js. Compiled out on aplite along with the rest of the Finish Day
// row (see menu_draw_row's own comment) - nothing calls this there.
static void send_finish_day(void) {
  begin_send(MSG_FINISH_DAY, NULL, NULL, 0);
}
#endif

#ifndef PBL_PLATFORM_APLITE
static void send_task_add(const char *title) {
  begin_send(MSG_TASK_ADD, title, NULL, 0);
}

static void send_note_append(const char *task_id, const char *note_text) {
  begin_send(MSG_NOTE_APPEND, task_id, note_text, 0);
}

// Fires when dictation finishes (success, cancel, or failure). Only ever
// registered on mic-equipped platforms - compiled out entirely on aplite
// (see s_mic_bitmap's own comment for why). Shared by both Add Task and
// note-append (see s_dictation_is_note_append's own comment) since only one
// dictation session/flag pair exists for the app's whole lifetime.
static void dictation_status_callback(DictationSession *session, DictationSessionStatus status,
                                       char *transcription, void *context) {
  s_dictation_pending = false;
  if (status == DictationSessionStatusSuccess) {
    if (s_dictation_is_note_append) {
      send_note_append(s_notes_overlay_task_id, transcription);
    } else {
      send_task_add(transcription);
    }
    return;
  }
  if (status == DictationSessionStatusFailureTranscriptionRejected) {
    // The user declined the transcription on the confirmation screen (which
    // this app leaves enabled - dictation_session_enable_confirmation) -
    // this is a cancel, not a failure, so it's a silent no-op, same as
    // long-selecting a done task is (see menu_select_long_click).
    return;
  }
  // Every other failure (no speech, connectivity, disabled, internal,
  // recognizer error) already gets a dialog from the OS's own dictation UI
  // by default (dictation_session_enable_error_dialogs, left at its default
  // "on" here) - duplicating that in this app's own error overlay would be
  // redundant, not closing a silent gap the way the outbox/STATUS_NOT_PAIRED
  // fixes did (those had no other signal at all). Logged only.
  APP_LOG(APP_LOG_LEVEL_INFO, "dictation failed, status=%d", (int)status);
}

static void start_add_task_dictation(void) {
  if (s_dictation_pending || !s_dictation_session) {
    // Ignores a rapid double-press (own guard, not relying on
    // dictation_session_start()'s own return value - see s_dictation_pending's
    // comment) and is a no-op if this ever gets called before window_load
    // has created the session (shouldn't happen: the row that reaches this
    // is itself gated by PBL_IF_MICROPHONE_ELSE).
    return;
  }
  s_dictation_is_note_append = false;
  s_dictation_pending = true;
  dictation_session_start(s_dictation_session);
}

// Long-select on the notes overlay (see
// notes_window_select_long_click_handler) - dictates text to append to the
// currently-shown task's notes, same session/guard as start_add_task_dictation
// above, just tagged for the callback to route differently.
// s_notes_overlay_task_id is only valid while s_notes_window is the pushed/
// visible window, which is the only way this ever gets called.
static void start_note_append_dictation(void) {
  if (s_dictation_pending || !s_dictation_session) {
    return;
  }
  s_dictation_is_note_append = true;
  s_dictation_pending = true;
  dictation_session_start(s_dictation_session);
}
#endif

static void tracking_tick_callback(void *data) {
  // Only the elapsed-time text changes each tick, not the row data itself -
  // mark_dirty (a repaint) rather than reload_data (which also re-asks for
  // section/row counts etc.) is the same lighter-weight choice
  // scroll_timer_callback already makes for the marquee tick.
  layer_mark_dirty(menu_layer_get_layer(s_menu_layer));
  s_tracking_tick_timer = app_timer_register(TRACKING_TICK_INTERVAL_MS, tracking_tick_callback, NULL);
}

static void start_tracking_tick(void) {
  if (!s_tracking_tick_timer) {
    s_tracking_tick_timer = app_timer_register(TRACKING_TICK_INTERVAL_MS, tracking_tick_callback, NULL);
  }
}

static void stop_tracking_tick(void) {
  if (s_tracking_tick_timer) {
    app_timer_cancel(s_tracking_tick_timer);
    s_tracking_tick_timer = NULL;
  }
}

static void start_tracking(Task *task) {
  strncpy(s_tracking_task_id, task->id, MAX_ID_LEN - 1);
  s_tracking_task_id[MAX_ID_LEN - 1] = '\0';
  s_tracking_start_epoch = time(NULL);
  save_tracking();
  start_tracking_tick();
}

// Stops whatever's currently being tracked (a no-op if nothing is). When
// send_to_phone is true, reports the elapsed session for upload (see
// handleTrackTimeStop in index.js) - false is used when switching tracking
// to a different task, where the elapsed session on the PREVIOUS task
// still needs reporting first, which the caller does itself before calling
// start_tracking() on the new one (see menu_select_long_click).
static void stop_tracking_and_report(void) {
  if (s_tracking_task_id[0] == '\0') {
    return;
  }
  time_t elapsed_s = time(NULL) - s_tracking_start_epoch;
  if (elapsed_s > 0) {
    int32_t elapsed_ms = (int32_t)elapsed_s * 1000;
    send_track_time_stop(s_tracking_task_id, elapsed_ms);
    // Optimistic local bump, same idea as menu_select_click's optimistic
    // task->done flip: the phone will report back the real synced total
    // (merged with whatever other clients tracked in the meantime) on the
    // next full sync, but this avoids the subtitle reverting to the stale
    // pre-session total in the meantime.
    Task *tracked_task = find_task_by_id(s_tracking_task_id);
    if (tracked_task) {
      tracked_task->time_spent_ms += elapsed_ms;
      save_tasks();
    }
  }
  s_tracking_task_id[0] = '\0';
  s_tracking_start_epoch = 0;
  save_tracking();
  stop_tracking_tick();
}

static void menu_select_click(MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  backlight_touch();
  // No s_notes_overlay_active check here (unlike the error overlay below) -
  // the notes overlay is a separate pushed Window (see s_notes_window's own
  // comment), so this callback simply never fires while it's on top; the
  // window stack itself keeps the two from ever overlapping.
  if (s_error_overlay_active) {
    // Click routing goes through MenuLayer's own config regardless of
    // whether its layer is currently hidden (same mechanism the empty-
    // screen's phantom row 0 below already relies on), so this fires even
    // though s_menu_layer is hidden behind the overlay right now.
    hide_error_overlay();
    return;
  }
  if (s_task_count == 0 && !ACTIONABLE_EMPTY_ACTIVE()) {
    // The empty/error screen's phantom row 0 (see menu_get_num_rows) lands
    // here - this is what makes "Select to retry" actually retry. The
    // actionable empty state (STATUS_OK, zero tasks) falls through to the
    // normal section-0 routing below instead, since it exposes the real
    // Resync/Habits/Add Task rows rather than one phantom retry row.
    request_sync();
    return;
  }
  if (cell_index->section == 0) {
    Section0RowKind kind = section0_row_kind((int)cell_index->row);
    if (kind == SECTION0_ROW_HABITS) {
      push_habits_window();
#ifndef PBL_PLATFORM_APLITE
    } else if (kind == SECTION0_ROW_ADD_TASK) {
      // Only reachable on mic-equipped platforms with the feature enabled
      // (see menu_get_num_rows/section0_row_kind).
      start_add_task_dictation();
#endif
    } else {
      // The "Resync" row atop the populated list.
      request_sync();
    }
    return;
  }
  Task *task = resolve_task_at(*cell_index);
  if (!task) {
    return;
  }
#ifndef PBL_PLATFORM_APLITE
  // Double-click (a second Select click on this SAME task before the
  // pending single-click toggle below has committed) shows notes instead
  // of toggling - see pending_toggle_timer_callback and MAX_NOTES_LEN's
  // own comment for why this is compiled out on aplite entirely.
  if (s_pending_toggle_timer && strncmp(s_pending_toggle_task_id, task->id, MAX_ID_LEN) == 0) {
    app_timer_cancel(s_pending_toggle_timer);
    s_pending_toggle_timer = NULL;
    s_pending_toggle_task_id[0] = '\0';
    show_notes_overlay(task);
    return;
  }
  // A different task's toggle was still pending (the user scrolled and
  // clicked again before it committed) - it's clearly not getting
  // double-clicked anymore, so let it through right away instead of
  // silently dropping it, then start a fresh pending window for this click.
  if (s_pending_toggle_timer) {
    app_timer_cancel(s_pending_toggle_timer);
    pending_toggle_timer_callback(NULL);
  }
  strncpy(s_pending_toggle_task_id, task->id, MAX_ID_LEN - 1);
  s_pending_toggle_task_id[MAX_ID_LEN - 1] = '\0';
  s_pending_toggle_timer = app_timer_register(DOUBLE_CLICK_WINDOW_MS, pending_toggle_timer_callback, NULL);
#else
  task->done = !task->done;
  save_tasks();
  menu_layer_reload_data(s_menu_layer);
  send_task_toggle(task);
#endif
}

// Long-select toggles time tracking on the highlighted task: starts it if
// nothing (or a different task) is being tracked, stops it if this task is
// the one already being tracked. Only one task tracks at a time, so
// starting a new one first stops-and-reports whatever was running before -
// mirrors the real app's single global "current task", not a per-task
// independent timer each.
static void menu_select_long_click(MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  backlight_touch();
  // No s_notes_overlay_active check here - see menu_select_click's own
  // comment; voice note-append is wired directly on s_notes_window's own
  // click config now (notes_window_select_long_click_handler).
  if (s_error_overlay_active || s_task_count == 0 || cell_index->section == 0) {
    return;
  }
#ifndef PBL_PLATFORM_APLITE
  if ((int)cell_index->section - 1 == s_group_count) {
    // Finish Day row - see its own comment in menu_draw_row. No optimistic
    // local change here (unlike task-toggle/habit-adjust/time-tracking) -
    // archiving needs the phone's own full-fidelity state.task cache, which
    // the watch doesn't have; this is a pure fire-and-forget trigger, and
    // the phone pushes an updated task list back once it's actually done.
    // Closes the app once the send is confirmed (outbox_sent_handler), not
    // eagerly here - see s_close_after_finish_day_sent's own comment.
    s_close_after_finish_day_sent = true;
    send_finish_day();
    return;
  }
#endif
  Task *task = resolve_task_at(*cell_index);
  if (!task || task->done) {
    // Tracking a completed task isn't a real scenario - not worth a
    // separate error/feedback path, just ignore the long-press.
    return;
  }
  bool already_tracking_this = s_tracking_task_id[0] != '\0' &&
                                strncmp(s_tracking_task_id, task->id, MAX_ID_LEN) == 0;
  stop_tracking_and_report();
  if (!already_tracking_this) {
    start_tracking(task);
  }
  menu_layer_reload_data(s_menu_layer);
}

// ---------- empty / status placeholder ----------

static void stop_syncing_animation(void) {
  if (s_syncing_timer) {
    app_timer_cancel(s_syncing_timer);
    s_syncing_timer = NULL;
  }
#ifndef PBL_PLATFORM_APLITE
  // Restores s_empty_layer to the plain font every other empty-state
  // message (not-paired/error/no-tasks) actually uses, and hides the
  // percent/hint subtitle - both are only ever populated while syncing.
  text_layer_set_font(s_empty_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_set_hidden(text_layer_get_layer(s_sync_progress_layer), true);
#endif
}

#ifndef PBL_PLATFORM_APLITE
// Refreshes s_sync_progress_layer's text from the latest s_status_msg -
// "NN%" while a multi-page sync is in progress (index.js's pullPage()
// computes this from res.latestSeq, the account's high-water mark, vs how
// far the current page has gotten), or the "may take a few minutes"
// heads-up before any percentage is available (e.g. still on the first
// page, or a single-page sync that finishes before this would ever show).
static void update_sync_progress_text(void) {
  if (s_status_msg[0] != '\0') {
    text_layer_set_text(s_sync_progress_layer, s_status_msg);
  } else {
    text_layer_set_text(s_sync_progress_layer, "This may take a few minutes");
  }
}
#endif

static void syncing_timer_callback(void *data) {
  s_syncing_dots = (s_syncing_dots + 1) % 4;
#ifdef PBL_PLATFORM_APLITE
  // No spare TextLayer here (see s_sync_progress_layer's own comment) - the
  // percentage, when available, rides on the same single line/font as
  // everything else instead of a separate subtitle.
  static char s_syncing_text[MAX_STATUS_MSG_LEN + 16];
  if (s_status_msg[0] != '\0') {
    snprintf(s_syncing_text, sizeof(s_syncing_text), "Syncing %s%.*s", s_status_msg, s_syncing_dots, "...");
  } else {
    snprintf(s_syncing_text, sizeof(s_syncing_text), "Syncing%.*s\n\nThis may take a few minutes", s_syncing_dots, "...");
  }
  text_layer_set_text(s_empty_layer, s_syncing_text);
#else
  static char s_syncing_text[16];
  snprintf(s_syncing_text, sizeof(s_syncing_text), "Syncing%.*s", s_syncing_dots, "...");
  text_layer_set_text(s_empty_layer, s_syncing_text);
  update_sync_progress_text();
#endif
  s_syncing_timer = app_timer_register(SYNCING_ANIM_INTERVAL_MS, syncing_timer_callback, NULL);
}

static void start_syncing_animation(void) {
#ifndef PBL_PLATFORM_APLITE
  // Bigger than every other empty-state message's font (see
  // stop_syncing_animation's restore) - safe to go large here specifically
  // because the title text itself is always short ("Syncing..."), unlike
  // the not-paired/error messages that share this same layer and can run
  // to a full wrapped sentence.
  text_layer_set_font(s_empty_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  layer_set_hidden(text_layer_get_layer(s_sync_progress_layer), false);
  if (s_syncing_timer) {
    update_sync_progress_text(); // a status push (new percent) mid-sync - timer's already running
    return;
  }
#else
  if (s_syncing_timer) {
    return;
  }
#endif
  s_syncing_dots = 0;
#ifdef PBL_PLATFORM_APLITE
  text_layer_set_text(s_empty_layer, "Syncing\n\nThis may take a few minutes");
#else
  text_layer_set_text(s_empty_layer, "Syncing");
  update_sync_progress_text();
#endif
  s_syncing_timer = app_timer_register(SYNCING_ANIM_INTERVAL_MS, syncing_timer_callback, NULL);
}

// Shows (or, called again while already showing, re-affirms/updates the
// message on) the fullscreen error overlay, hiding whatever the menu/
// empty-state layers were showing underneath. Safe to call regardless of
// s_task_count - stop_syncing_animation() covers the case where this
// interrupts the very first sync's "Syncing..." animation.
static void show_error_overlay(void) {
  s_error_overlay_active = true;
  static char s_error_overlay_text[MAX_STATUS_MSG_LEN + 48];
  if (s_status_msg[0] != '\0') {
    snprintf(s_error_overlay_text, sizeof(s_error_overlay_text),
              "Sync Error\n\n%s\n\nSelect to dismiss", s_status_msg);
  } else {
    snprintf(s_error_overlay_text, sizeof(s_error_overlay_text), "Sync Error\n\nSelect to dismiss");
  }
  text_layer_set_text(s_error_layer, s_error_overlay_text);
  layer_set_hidden(text_layer_get_layer(s_error_layer), false);
  layer_set_hidden(menu_layer_get_layer(s_menu_layer), true);
  layer_set_hidden(text_layer_get_layer(s_empty_layer), true);
  layer_set_hidden(bitmap_layer_get_layer(s_logo_layer), true);
  stop_syncing_animation();
}

static void update_empty_layer(void) {
  // The error overlay owns menu/empty-layer visibility while it's up (it
  // hides both itself, in show_error_overlay()) - without this guard, any
  // background status update arriving before Select dismisses it (e.g. a
  // retry's own TASK_SYNC_END) would un-hide the menu or empty layer right
  // out from under the overlay via the calls below, even though the
  // overlay's own layer would still be showing on top of it too.
  // No equivalent guard for the notes overlay - it's a separate pushed
  // Window (s_notes_window) now, not a layer sharing s_main_window with the
  // menu/empty layers this function touches, so it's unaffected either way
  // (and invisible underneath while the notes window is on top), same as
  // s_habits_window needs no such guard here either.
  if (s_error_overlay_active) {
    return;
  }
  bool show_empty = (s_task_count == 0);
  // STATUS_OK with zero tasks (e.g. Today Only filtering out everything due
  // today) is an "actionable" empty state: rather than the standalone
  // s_empty_layer/s_logo_layer pair every other empty reason uses, the menu
  // layer itself stays visible, showing "No tasks for today." as section 0's
  // header with the real Resync/Habits/Add Task rows still reachable below
  // it - see menu_get_num_rows/menu_get_header_height/menu_draw_header/
  // menu_draw_row/menu_select_click, which all key off this same
  // ACTIONABLE_EMPTY_ACTIVE() check (always false on aplite - see its own
  // comment).
  bool show_actionable_empty = show_empty && ACTIONABLE_EMPTY_ACTIVE();
  layer_set_hidden(text_layer_get_layer(s_empty_layer), !show_empty || show_actionable_empty);
  layer_set_hidden(bitmap_layer_get_layer(s_logo_layer), !show_empty || show_actionable_empty);
  layer_set_hidden(menu_layer_get_layer(s_menu_layer), show_empty && !show_actionable_empty);

  // Only the empty screen (no cached list yet at all - i.e. the very first
  // sync) gets the animation; a resync with a populated list already has
  // its own live status via the Resync row's subtitle.
  bool is_initial_syncing = show_empty && s_status_code == STATUS_SYNCING;
  if (is_initial_syncing) {
    start_syncing_animation();
  } else {
    stop_syncing_animation();
  }

  if (!show_empty || is_initial_syncing || show_actionable_empty) {
    return;
  }

  static char s_empty_text[MAX_STATUS_MSG_LEN + 32];

  // Only STATUS_NOT_PAIRED and STATUS_ERROR ever reach here now -
  // show_actionable_empty above already intercepted the STATUS_OK case,
  // and is_initial_syncing already intercepted STATUS_SYNCING.
  if (s_status_code == STATUS_NOT_PAIRED) {
    text_layer_set_text(s_empty_layer, "Open the app on\nyour phone to pair\nwith SuperSync.");
  } else {
    if (s_status_msg[0] != '\0') {
      snprintf(s_empty_text, sizeof(s_empty_text), "Sync error:\n%s\nSelect to retry.", s_status_msg);
      text_layer_set_text(s_empty_layer, s_empty_text);
    } else {
      text_layer_set_text(s_empty_layer, "Sync error.\nSelect to retry.");
    }
  }
}

// Dismisses the error overlay (Select, see menu_select_click) and hands
// visibility back to update_empty_layer() - now unguarded, since the flag
// flips before calling it - to restore whichever of menu/empty-state is
// correct for however s_task_count/s_status_code look right now (not
// necessarily how they looked when the overlay first appeared).
static void hide_error_overlay(void) {
  s_error_overlay_active = false;
  layer_set_hidden(text_layer_get_layer(s_error_layer), true);
  update_empty_layer();
}

#ifndef PBL_PLATFORM_APLITE
// Backing buffer for s_notes_layer's text - file-scope (not function-local
// like show_notes_overlay's own version used to be) since notes_window_load
// needs to read it too, at a later point in time (window_stack_push doesn't
// invoke it synchronously - see push_notes_window). Only the "Notes:\n\n"
// prefix needs building here, not a copy of task->notes itself, but this
// still can't just point text_layer_set_text at task->notes directly since
// that string lives in s_tasks, which a background sync can reallocate out
// from under an already-open notes window (see s_notes_overlay_task_id's
// own comment).
static char s_notes_overlay_text[MAX_NOTES_LEN + 48];

static void build_notes_overlay_text(Task *task) {
  if (task->notes[0] != '\0') {
    snprintf(s_notes_overlay_text, sizeof(s_notes_overlay_text), "Notes:\n\n%s", task->notes);
  } else {
    snprintf(s_notes_overlay_text, sizeof(s_notes_overlay_text), "(No notes for this task)");
  }
}

// How tall s_notes_overlay_text lays out (word-wrapped) at the given width -
// shared by notes_window_load (to size the TextLayer/ScrollLayer on first
// open) and show_notes_overlay's live-refresh path (to resize them when a
// note-append changes the text under an already-open overlay). Measured
// against a generously tall test box: MAX_NOTES_LEN's worst case is still
// only a few screens of wrapped text, nowhere near this ceiling.
// Confirmed live on-device that this measurement runs short of what
// TextLayer actually needs to avoid clipping its own last lines (a real note
// measured at 198px rendered visibly cut off in a 208px-tall viewport, and
// still stopped a line or two short of the true end after a flat +24px
// margin) - the shortfall scales with content length rather than being a
// fixed few px, so the margin below is proportional (10%) plus a flat
// two-line floor, generous enough that even a long, heavily-appended note's
// true last line is always reachable. A short note that already fit just
// gets a bit of harmless extra scroll room past its actual end.
#define NOTES_TEXT_HEIGHT_MARGIN_FLOOR 48

static int16_t measure_notes_text_height(int16_t width) {
  GSize size = graphics_text_layout_get_content_size(
      s_notes_overlay_text, fonts_get_system_font(FONT_KEY_GOTHIC_18), GRect(0, 0, width, 2000),
      GTextOverflowModeWordWrap, GTextAlignmentLeft);
  int16_t margin = size.h / 10;
  if (margin < NOTES_TEXT_HEIGHT_MARGIN_FLOOR) {
    margin = NOTES_TEXT_HEIGHT_MARGIN_FLOOR;
  }
  return size.h + margin;
}

// Shows a task's notes (double-click Select on it - see
// pending_toggle_timer_callback/menu_select_click) in their own pushed
// Window (see s_notes_window's own comment for why - Back's default pop
// behavior). Also doubles as a live refresh while that window is already
// open (see the MSG_TASK_SYNC_END handler's own call site, after a
// note-append round-trip lands): s_notes_layer is only non-NULL between
// notes_window_load/unload, so that case just updates the already-visible
// text in place instead of pushing a second copy of the window.
static void show_notes_overlay(Task *task) {
  s_notes_overlay_active = true;
  strncpy(s_notes_overlay_task_id, task->id, MAX_ID_LEN - 1);
  s_notes_overlay_task_id[MAX_ID_LEN - 1] = '\0';
  build_notes_overlay_text(task);
  if (s_notes_layer) {
    text_layer_set_text(s_notes_layer, s_notes_overlay_text);
    // The new text can be a different length than what was there before
    // (that's the whole point of a live refresh - a note-append just
    // landed) - resize the scrollable content to match and snap back to
    // the top rather than leaving the scroll position mid-way through text
    // that may no longer be there.
    GRect scroll_bounds = layer_get_bounds(scroll_layer_get_layer(s_notes_scroll_layer));
    int16_t text_height = measure_notes_text_height(scroll_bounds.size.w);
    if (text_height < scroll_bounds.size.h) {
      text_height = scroll_bounds.size.h;
    }
    GRect text_frame = layer_get_frame(text_layer_get_layer(s_notes_layer));
    text_frame.size.h = text_height;
    layer_set_frame(text_layer_get_layer(s_notes_layer), text_frame);
    scroll_layer_set_content_size(s_notes_scroll_layer, GSize(scroll_bounds.size.w, text_height));
    scroll_layer_set_content_offset(s_notes_scroll_layer, GPointZero, false);
    return;
  }
  push_notes_window();
}

// Dismisses the notes overlay (Select, see
// notes_window_select_click_handler) - notes_window_unload clears
// s_notes_overlay_active once the pop actually completes, same as it would
// for a Back-triggered dismissal (Pebble's own default pop behavior, no
// click config needed for that - see s_notes_window's own comment), so
// both dismissal paths converge on the same cleanup.
static void hide_notes_overlay(void) {
  window_stack_pop(true);
}

// Commits a single-click task-done toggle once the double-click window has
// passed with no second click - see menu_select_click. Looks the task back
// up by id rather than trusting a stashed pointer: a background sync can
// fully rebuild s_tasks (double-buffered commit on TASK_SYNC_END) while
// this timer is still pending, which would leave a raw Task* dangling.
static void pending_toggle_timer_callback(void *data) {
  s_pending_toggle_timer = NULL;
  Task *task = find_task_by_id(s_pending_toggle_task_id);
  s_pending_toggle_task_id[0] = '\0';
  if (!task) {
    return; // The list changed underneath the pending click - nothing to commit.
  }
  task->done = !task->done;
  save_tasks();
  menu_layer_reload_data(s_menu_layer);
  send_task_toggle(task);
}
#endif

// ---------- AppMessage ----------

static void request_sync(void) {
#ifdef PBL_PLATFORM_APLITE
  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) != APP_MSG_OK) {
    return;
  }
  dict_write_int32(iter, KEY_MSG_TYPE, MSG_REQUEST_SYNC);
  app_message_outbox_send();
#else
  begin_send(MSG_REQUEST_SYNC, NULL, NULL, 0);
#endif
}

static void inbox_received_handler(DictionaryIterator *iterator, void *context) {
  Tuple *type_tuple = dict_find(iterator, KEY_MSG_TYPE);
  if (!type_tuple) {
    return;
  }

  switch (type_tuple->value->int32) {
    case MSG_TASK_SYNC_START: {
      Tuple *total_tuple = dict_find(iterator, KEY_TASK_TOTAL);
      s_incoming_total = total_tuple ? total_tuple->value->int32 : 0;
      if (s_incoming_total > MAX_TASKS) {
        s_incoming_total = MAX_TASKS;
      }
      s_status_code = STATUS_SYNCING;
      break;
    }
    case MSG_TASK_ITEM: {
      Tuple *idx_tuple = dict_find(iterator, KEY_TASK_INDEX);
      Tuple *id_tuple = dict_find(iterator, KEY_TASK_ID);
      Tuple *title_tuple = dict_find(iterator, KEY_TASK_TITLE);
      Tuple *done_tuple = dict_find(iterator, KEY_TASK_DONE);
      Tuple *project_tuple = dict_find(iterator, KEY_TASK_PROJECT);
      Tuple *due_min_tuple = dict_find(iterator, KEY_TASK_DUE_MIN);
      Tuple *time_spent_tuple = dict_find(iterator, KEY_TASK_TIME_SPENT_MS);
      Tuple *time_estimate_tuple = dict_find(iterator, KEY_TASK_TIME_ESTIMATE_MS);
#ifndef PBL_PLATFORM_APLITE
      Tuple *notes_tuple = dict_find(iterator, KEY_TASK_NOTES);
#endif
      if (!idx_tuple || !id_tuple || !title_tuple) {
        break;
      }
      int idx = idx_tuple->value->int32;
      if (idx < 0 || idx >= MAX_TASKS) {
        break;
      }
      strncpy(s_incoming[idx].id, id_tuple->value->cstring, MAX_ID_LEN - 1);
      s_incoming[idx].id[MAX_ID_LEN - 1] = '\0';
      strncpy(s_incoming[idx].title, title_tuple->value->cstring, MAX_TITLE_LEN - 1);
      s_incoming[idx].title[MAX_TITLE_LEN - 1] = '\0';
      strncpy(s_incoming[idx].project, project_tuple ? project_tuple->value->cstring : "", MAX_PROJECT_LEN - 1);
      s_incoming[idx].project[MAX_PROJECT_LEN - 1] = '\0';
      s_incoming[idx].done = done_tuple && done_tuple->value->int32 != 0;
      // Absent (not just 0, which is a legitimate 12:00am) means "no
      // dueWithTime" - the phone only includes this key at all when the
      // task actually has one, see sendTaskAt() in index.js.
      s_incoming[idx].due_min = due_min_tuple ? due_min_tuple->value->int32 : -1;
      // Absent means "phone has no tracked time for this task" - see
      // sendTaskAt() in index.js, same convention as due_min above.
      s_incoming[idx].time_spent_ms = time_spent_tuple ? time_spent_tuple->value->int32 : 0;
      // Absent means "no timeEstimate" - same convention as time_spent_ms.
      s_incoming[idx].time_estimate_ms = time_estimate_tuple ? time_estimate_tuple->value->int32 : 0;
#ifndef PBL_PLATFORM_APLITE
      // Absent means "no notes" - same convention as due_min/time_spent_ms
      // above. See sendTaskAt() in index.js for the phone-side truncation.
      strncpy(s_incoming[idx].notes, notes_tuple ? notes_tuple->value->cstring : "", MAX_NOTES_LEN - 1);
      s_incoming[idx].notes[MAX_NOTES_LEN - 1] = '\0';
#endif
      break;
    }
    case MSG_TASK_SYNC_END: {
      int count = s_incoming_total < MAX_TASKS ? s_incoming_total : MAX_TASKS;
      memcpy(s_tasks, s_incoming, sizeof(Task) * (size_t)count);
      s_task_count = count;
      s_status_code = STATUS_OK;
      recompute_groups();
      save_tasks();
#ifndef PBL_PLATFORM_APLITE
      if (s_notes_overlay_active) {
        // A note-append (or any other resync) can change this task's notes
        // while its overlay is still open - refresh the displayed text
        // rather than leaving it showing whatever was true before this
        // sync landed. No-op if the task's gone missing from this refresh
        // (stays showing the last text it had).
        Task *notes_task = find_task_by_id(s_notes_overlay_task_id);
        if (notes_task) {
          show_notes_overlay(notes_task);
        }
      }
#endif
      menu_layer_reload_data(s_menu_layer);
      update_empty_layer();
      // A refreshed list can change whether the (possibly now-different)
      // selected row needs to scroll at all.
      refresh_scroll_state(true);
      break;
    }
    case MSG_HABIT_SYNC_START: {
      Tuple *total_tuple = dict_find(iterator, KEY_HABIT_TOTAL);
      s_habit_incoming_total = total_tuple ? total_tuple->value->int32 : 0;
      if (s_habit_incoming_total > MAX_HABITS) {
        s_habit_incoming_total = MAX_HABITS;
      }
      break;
    }
    case MSG_HABIT_ITEM: {
      Tuple *idx_tuple = dict_find(iterator, KEY_HABIT_INDEX);
      Tuple *id_tuple = dict_find(iterator, KEY_HABIT_ID);
      Tuple *title_tuple = dict_find(iterator, KEY_HABIT_TITLE);
      Tuple *done_tuple = dict_find(iterator, KEY_HABIT_DONE);
      Tuple *value_tuple = dict_find(iterator, KEY_HABIT_VALUE);
      Tuple *goal_tuple = dict_find(iterator, KEY_HABIT_GOAL);
      Tuple *type_tuple = dict_find(iterator, KEY_HABIT_TYPE);
      if (!idx_tuple || !id_tuple || !title_tuple) {
        break;
      }
      int idx = idx_tuple->value->int32;
      if (idx < 0 || idx >= MAX_HABITS) {
        break;
      }
      // Written directly into s_habits (not a separate incoming buffer -
      // see the Habit struct's own comment on why that's safe here).
      strncpy(s_habits[idx].id, id_tuple->value->cstring, MAX_HABIT_ID_LEN - 1);
      s_habits[idx].id[MAX_HABIT_ID_LEN - 1] = '\0';
      strncpy(s_habits[idx].title, title_tuple->value->cstring, MAX_TITLE_LEN - 1);
      s_habits[idx].title[MAX_TITLE_LEN - 1] = '\0';
      s_habits[idx].done = done_tuple && done_tuple->value->int32 != 0;
      s_habits[idx].value = value_tuple ? value_tuple->value->int32 : 0;
      s_habits[idx].goal = goal_tuple ? goal_tuple->value->int32 : 0;
      // 0 = ClickCounter, 1 = StopWatch, 2 = RepeatedCountdownReminder - see
      // sendHabitAt() in index.js.
#ifdef PBL_PLATFORM_APLITE
      // Every extra byte of code costs here (aplite's linker script maps
      // .text/.data/.bss into one shared 24KB region, unlike every other
      // platform's separate flash/RAM regions - see MAX_HABITS' own
      // comment), so this stays the cheap single-check form: is_countdown
      // is never read anywhere in aplite's compiled code (resolve_habit_at/
      // habits_menu_get_num_rows only check is_stopwatch there - see their
      // own comments), and a RepeatedCountdownReminder is excluded from
      // aplite's visible list exactly the same as a real StopWatch is, so
      // collapsing both into is_stopwatch here still filters correctly.
      s_habits[idx].is_stopwatch = type_tuple && type_tuple->value->int32 != 0;
#else
      int habit_type = type_tuple ? type_tuple->value->int32 : 0;
      s_habits[idx].is_stopwatch = habit_type == 1;
      s_habits[idx].is_countdown = habit_type == 2;
      Tuple *countdown_tuple = dict_find(iterator, KEY_HABIT_COUNTDOWN_MS);
      s_habits[idx].countdown_ms = countdown_tuple ? countdown_tuple->value->int32 : 0;
#endif
      break;
    }
    case MSG_HABIT_SYNC_END: {
      s_habit_count = s_habit_incoming_total;
      save_habits();
      if (s_habits_menu_layer) {
        menu_layer_reload_data(s_habits_menu_layer);
      }
      if (s_habits_empty_layer) {
        update_habits_empty_layer();
      }
      break;
    }
    case MSG_SYNC_STATUS: {
      Tuple *status_tuple = dict_find(iterator, KEY_STATUS_CODE);
      if (status_tuple) {
        s_status_code = status_tuple->value->int32;
      }
      Tuple *msg_tuple = dict_find(iterator, KEY_STATUS_MSG);
      if (msg_tuple) {
        strncpy(s_status_msg, msg_tuple->value->cstring, MAX_STATUS_MSG_LEN - 1);
        s_status_msg[MAX_STATUS_MSG_LEN - 1] = '\0';
      } else {
        s_status_msg[0] = '\0';
      }
      // Feature toggles set from the phone's own pairing settings - optional
      // fields (same dict_find + null-check pattern as STATUS_CODE above),
      // absent-means-unchanged so an old phone build talking to this watch
      // (or vice versa) can't accidentally reset either flag. Read before
      // reload_data below so a settings change is reflected in the very
      // same redraw, not a subsequent one.
      Tuple *habits_enabled_tuple = dict_find(iterator, KEY_HABITS_ENABLED);
      if (habits_enabled_tuple) {
        s_habits_enabled = habits_enabled_tuple->value->int32 != 0;
      }
      Tuple *add_task_enabled_tuple = dict_find(iterator, KEY_ADD_TASK_ENABLED);
      if (add_task_enabled_tuple) {
        s_add_task_enabled = add_task_enabled_tuple->value->int32 != 0;
      }
      // Same optional/absent-means-unchanged pattern as the two flags above,
      // but only re-applied (apply_backlight_mode()) when the value actually
      // changed - this field is sent on EVERY status push, including the
      // routine ones a background auto-sync fires every few minutes, and
      // re-triggering the backlight on each of those (rather than just on an
      // actual settings save) would defeat a custom timeout's whole point.
#ifndef PBL_PLATFORM_APLITE
      Tuple *backlight_mode_tuple = dict_find(iterator, KEY_BACKLIGHT_MODE);
      if (backlight_mode_tuple && backlight_mode_tuple->value->int32 != s_backlight_mode) {
        s_backlight_mode = backlight_mode_tuple->value->int32;
        apply_backlight_mode();
      }
#endif
#ifndef PBL_PLATFORM_APLITE
      // Re-arms the background-wakeup schedule (see schedule_next_wakeup())
      // the first time this launch confirms the interval, and again if it
      // actually changes mid-session - not on every status push, which
      // would otherwise fire on every SYNCING/OK tick of a single sync and
      // on every watch-initiated action's own follow-up status.
      Tuple *auto_sync_interval_tuple = dict_find(iterator, KEY_AUTO_SYNC_INTERVAL_MIN);
      if (auto_sync_interval_tuple &&
          (auto_sync_interval_tuple->value->int32 != s_auto_sync_interval_min || !s_wakeup_rescheduled_this_launch)) {
        s_auto_sync_interval_min = auto_sync_interval_tuple->value->int32;
        schedule_next_wakeup();
        s_wakeup_rescheduled_this_launch = true;
      }
#endif
      // update_empty_layer() only redraws the empty-state text layer, which
      // stays hidden while s_task_count > 0 - reload_data is what actually
      // refreshes the Resync row's status subtitle in that case. Both are
      // no-ops while the error overlay is up (see its guard in
      // update_empty_layer()) - reload_data itself is harmless against a
      // hidden MenuLayer either way.
      menu_layer_reload_data(s_menu_layer);
      update_empty_layer();
      if (s_status_code == STATUS_ERROR) {
        show_error_overlay();
      }
#ifndef PBL_PLATFORM_APLITE
      // A wakeup-launched session exists purely to sync quietly and get
      // back out of the way - once the sync has actually concluded (success,
      // failure, or "not paired", as opposed to the SYNCING status that
      // fires first), pop back to whatever was on screen before this app
      // was auto-launched. A manually opened session (s_is_wakeup_launch
      // false) never takes this branch and behaves exactly as it always
      // has. Guarded so a retried sync's own second terminal status in the
      // same session can't pop an already-empty window stack.
      if (s_is_wakeup_launch && !s_wakeup_exit_triggered &&
          (s_status_code == STATUS_OK || s_status_code == STATUS_ERROR || s_status_code == STATUS_NOT_PAIRED)) {
        s_wakeup_exit_triggered = true;
        APP_LOG(APP_LOG_LEVEL_INFO, "wakeup sync done (status %d), exiting", (int)s_status_code);
        // Tells the system this exit was a deliberate, completed action
        // (not the user backing out) - without this, exiting can land the
        // user in the app launcher menu instead of back on their watchface,
        // which would make a quiet background sync anything but quiet.
        exit_reason_set(APP_EXIT_ACTION_PERFORMED_SUCCESSFULLY);
        window_stack_pop_all(true);
      }
#endif
      break;
    }
    default:
      break;
  }
}

static void inbox_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_WARNING, "AppMessage dropped: %d", (int)reason);
}

#ifndef PBL_PLATFORM_APLITE
// Fires once the phone-side OS layer actually confirms delivery (not just
// "queued") of ANY outbound message - only closes the app when the
// Finish Day send is what just succeeded, tracked via
// s_close_after_finish_day_sent (see its own comment). Also the success
// half of the outbound-send-retry mechanism (see its own top comment) -
// clears whatever begin_send() stashed so a later, unrelated failure can't
// mistakenly retry a message that already went through.
static void outbox_sent_handler(DictionaryIterator *iterator, void *context) {
  clear_pending_retry();
  if (s_close_after_finish_day_sent) {
    s_close_after_finish_day_sent = false;
    window_stack_pop_all(true);
  }
}

// Only these AppMessageResult reasons describe something transient - a
// momentarily busy/backgrounded phone side or Bluetooth link that a short
// retry has a real chance of working around, per each one's own doc
// comment in pebble.h. Deliberately excludes reasons that describe a
// structural problem with the call/message itself (APP_MSG_INVALID_ARGS,
// APP_MSG_OUT_OF_MEMORY, APP_MSG_CLOSED, APP_MSG_INTERNAL_ERROR,
// APP_MSG_INVALID_STATE, APP_MSG_ALREADY_RELEASED, the CALLBACK_* pair) -
// retrying those would just fail identically every time and delay the
// (still accurate) error for no benefit.
static bool is_retryable_failure(AppMessageResult reason) {
  switch (reason) {
    case APP_MSG_SEND_TIMEOUT:
    case APP_MSG_SEND_REJECTED:
    case APP_MSG_NOT_CONNECTED:
    case APP_MSG_APP_NOT_RUNNING:
    case APP_MSG_BUSY:
    case APP_MSG_BUFFER_OVERFLOW:
      return true;
    default:
      return false;
  }
}

static void retry_timer_callback(void *data) {
  s_retry_timer = NULL;
  send_pending_retry();
}
#endif

static void outbox_failed_handler(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_WARNING, "AppMessage send failed: %d", (int)reason);
#ifndef PBL_PLATFORM_APLITE
  // Don't leave the flag armed for some later, unrelated successful send to
  // wrongly close the app - see s_close_after_finish_day_sent's own comment.
  s_close_after_finish_day_sent = false;
  // Retry the exact same message (short backoff) before ever surfacing the
  // error overlay - see this file's "outbound send retry" section for why.
  // Only for failures that look transient, and only up to MAX_SEND_RETRIES
  // times - a genuinely unreachable phone still ends up at the same error
  // it always did, just a few seconds later instead of on the first hiccup.
  if (s_retry_msg_type != 0 && is_retryable_failure(reason) && s_retry_count < MAX_SEND_RETRIES) {
    s_retry_count++;
    uint32_t delay_ms = (uint32_t)RETRY_BACKOFF_BASE_MS << (s_retry_count - 1); // 1s, 2s, 4s
    s_retry_timer = app_timer_register(delay_ms, retry_timer_callback, NULL);
    return;
  }
  clear_pending_retry();
#endif
  // Previously silent (log-only): a watch->phone send (task toggle, track-
  // time-stop, resync request) that fails here - e.g. the phone app is
  // backgrounded or Bluetooth is momentarily busy - used to vanish with zero
  // on-screen feedback, indistinguishable from "worked fine". Routing it
  // through the same fullscreen overlay as a phone->watch sync error makes
  // every send failure visible instead of just this one class of them.
  // Reached immediately on aplite (no retry there - see this file's
  // "outbound send retry" section) or once every retry above is exhausted.
  s_status_code = STATUS_ERROR;
  strncpy(s_status_msg, "Couldn't reach phone app", MAX_STATUS_MSG_LEN - 1);
  s_status_msg[MAX_STATUS_MSG_LEN - 1] = '\0';
  show_error_overlay();
}

// ---------- habits window ----------

static Habit *resolve_habit_at(MenuIndex index) {
  if (index.section != 0) {
    return NULL;
  }
#ifdef PBL_PLATFORM_APLITE
  // StopWatch/RepeatedCountdownReminder-type habits are skipped entirely on
  // aplite (no tracking machinery or read-only display path is compiled in
  // there - see MAX_HABITS' own comment), so row indices here walk only the
  // plain-count subset of s_habits, same list habits_menu_get_num_rows
  // counts. is_stopwatch alone (not "|| is_countdown") is enough to check -
  // aplite's own MSG_HABIT_ITEM parsing collapses both timer-based types
  // into is_stopwatch to save code size (see its own comment); is_countdown
  // is never true there.
  int visible_row = 0;
  for (int i = 0; i < s_habit_count; i++) {
    if (s_habits[i].is_stopwatch) {
      continue;
    }
    if (visible_row == (int)index.row) {
      return &s_habits[i];
    }
    visible_row++;
  }
  return NULL;
#else
  if ((int)index.row >= s_habit_count) {
    return NULL;
  }
  return &s_habits[index.row];
#endif
}

static void send_habit_adjust(Habit *habit, int32_t delta) {
#ifdef PBL_PLATFORM_APLITE
  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) != APP_MSG_OK) {
    return;
  }
  dict_write_int32(iter, KEY_MSG_TYPE, MSG_HABIT_ADJUST);
  dict_write_cstring(iter, KEY_HABIT_ID, habit->id);
  dict_write_int32(iter, KEY_HABIT_DELTA, delta);
  app_message_outbox_send();
#else
  begin_send(MSG_HABIT_ADJUST, habit->id, NULL, delta);
#endif
}

// Everything from here through stop_habit_tracking_and_report is the
// StopWatch/RepeatedCountdownReminder habit timer itself - compiled out
// entirely on aplite (see s_tracking_habit_id's own comment on why).
// habits_menu_draw_row/select_long_click below have their own narrower
// #ifndef guards around just the pieces that touch this state, so a
// StopWatch habit still displays its progress read-only on aplite.
#ifndef PBL_PLATFORM_APLITE
static void send_habit_track_stop(const char *habit_id, int32_t tracked_ms) {
  begin_send(MSG_HABIT_TRACK_STOP, habit_id, NULL, tracked_ms);
}

static void stop_habit_tracking_tick(void) {
  if (s_habit_tracking_tick_timer) {
    app_timer_cancel(s_habit_tracking_tick_timer);
    s_habit_tracking_tick_timer = NULL;
  }
}

// Total elapsed ms for the CURRENT countdown session, whether paused or
// running - see s_habit_countdown_paused's own comment on how the two
// fields it's built from divide up the work. Only meaningful while
// s_tracking_habit_id refers to an is_countdown habit; callers already know
// that from context (only ever called from is_countdown-guarded code).
static int countdown_elapsed_ms(void) {
  if (s_habit_countdown_paused) {
    return s_habit_countdown_frozen_elapsed_ms;
  }
  time_t elapsed_s = time(NULL) - s_tracking_habit_start_epoch;
  int running_ms = elapsed_s > 0 ? (int)elapsed_s * 1000 : 0;
  return s_habit_countdown_frozen_elapsed_ms + running_ms;
}

// A RepeatedCountdownReminder's timer reaching zero - the on-watch
// equivalent of the real app's own explicit "Count up and restart!" banner
// tap (countUpAndNextRepeatCountdownSession in simple-counter-button.
// component.ts): +1 to today's count, uploaded via the same HABIT_ADJUST
// path a plain ClickCounter's Select already uses (handleHabitAdjust in
// index.js applies the delta against the phone's own cached value and
// uploads the result as a plain replace, so this is exactly as safe/
// idempotent as that). Unlike the real app, there's no banner/dialog to
// wait for a tap on here, so this fires automatically the moment the timer
// hits zero rather than waiting for one - but it deliberately does NOT
// auto-restart the countdown for the next round: the real app also
// requires an explicit tap to begin the next round, and auto-restarting
// unattended here would silently rack up completions (and uploads) for a
// countdown nobody's actually watching. Ends the tracking session outright;
// long-select starts the next round same as starting the first one did.
static void complete_habit_countdown(Habit *habit) {
  s_tracking_habit_id[0] = '\0';
  s_tracking_habit_start_epoch = 0;
  s_habit_countdown_paused = false;
  s_habit_countdown_frozen_elapsed_ms = 0;
  save_habit_tracking();
  stop_habit_tracking_tick();
  habit->value += 1;
  habit->done = habit->value >= habit->goal;
  save_habits();
  send_habit_adjust(habit, 1);
  if (s_habits_menu_layer) {
    menu_layer_reload_data(s_habits_menu_layer);
  }
}

// Redraw-only ticker for a tracked StopWatch/countdown habit, mirroring
// tracking_tick_callback for tasks - only the elapsed/remaining-time text
// normally changes each tick, so mark_dirty rather than reload_data (a
// countdown reaching zero is the one exception - see below). Guarded on
// s_habits_menu_layer being non-NULL because, unlike s_menu_layer (alive
// for the app's whole lifetime), this layer is torn down whenever the
// habits window unloads - see stop_habit_tracking_tick() being called
// there before the destroy.
static void habit_tracking_tick_callback(void *data) {
  if (s_tracking_habit_id[0] != '\0') {
    Habit *tracked_habit = find_habit_by_id(s_tracking_habit_id);
    if (tracked_habit && tracked_habit->is_countdown) {
      int remaining_ms = tracked_habit->countdown_ms - countdown_elapsed_ms();
      if (remaining_ms <= 0) {
        complete_habit_countdown(tracked_habit); // re-registers nothing - this round is over
        return;
      }
    }
  }
  if (s_habits_menu_layer) {
    layer_mark_dirty(menu_layer_get_layer(s_habits_menu_layer));
  }
  s_habit_tracking_tick_timer = app_timer_register(TRACKING_TICK_INTERVAL_MS, habit_tracking_tick_callback, NULL);
}

static void start_habit_tracking_tick(void) {
  if (!s_habit_tracking_tick_timer) {
    s_habit_tracking_tick_timer = app_timer_register(TRACKING_TICK_INTERVAL_MS, habit_tracking_tick_callback, NULL);
  }
}

static void start_habit_tracking(Habit *habit) {
  strncpy(s_tracking_habit_id, habit->id, MAX_HABIT_ID_LEN - 1);
  s_tracking_habit_id[MAX_HABIT_ID_LEN - 1] = '\0';
  s_tracking_habit_start_epoch = time(NULL);
  // Reset any pause state left over from a previous, since-finished session
  // - a fresh round always starts running, never paused.
  s_habit_countdown_paused = false;
  s_habit_countdown_frozen_elapsed_ms = 0;
  save_habit_tracking();
  start_habit_tracking_tick();
}

// Select on an is_countdown row that's currently tracking (running or
// paused) toggles between the two - long-select still ends the round
// outright (stop_habit_tracking_and_report). Not offered for a StopWatch
// habit (habits_menu_select_click never calls this for one) - "pause"
// isn't a meaningful state for an open-ended up-count timer the way it is
// for a fixed-length countdown.
static void toggle_habit_countdown_pause(void) {
  if (s_habit_countdown_paused) {
    s_habit_countdown_paused = false;
    s_tracking_habit_start_epoch = time(NULL); // fresh running segment; frozen_elapsed_ms already holds everything before it
    start_habit_tracking_tick();
  } else {
    s_habit_countdown_frozen_elapsed_ms = countdown_elapsed_ms(); // fold the just-finished running segment in before flipping the flag
    s_habit_countdown_paused = true;
    stop_habit_tracking_tick(); // nothing left to tick while frozen
  }
  save_habit_tracking();
  if (s_habits_menu_layer) {
    menu_layer_reload_data(s_habits_menu_layer);
  }
}

// Stops whatever StopWatch/countdown habit is currently being tracked (a
// no-op if nothing is) - mirrors stop_tracking_and_report's task
// equivalent, including the optimistic local bump (corrected by the next
// full sync, same as everywhere else this app does that). For a
// RepeatedCountdownReminder specifically, this only ever means "cancelled
// before completion" (reaching zero is handled entirely by
// complete_habit_countdown, which clears the session itself before this
// would ever run) - unlike a StopWatch habit, there's no meaningful partial
// progress to upload for a round that didn't finish, so this is a silent,
// no-upload cancel for that type.
static void stop_habit_tracking_and_report(void) {
  if (s_tracking_habit_id[0] == '\0') {
    return;
  }
  Habit *tracked_habit = find_habit_by_id(s_tracking_habit_id);
  if (!(tracked_habit && tracked_habit->is_countdown)) {
    time_t elapsed_s = time(NULL) - s_tracking_habit_start_epoch;
    if (elapsed_s > 0) {
      int32_t elapsed_ms = (int32_t)elapsed_s * 1000;
      send_habit_track_stop(s_tracking_habit_id, elapsed_ms);
      if (tracked_habit) {
        tracked_habit->value += elapsed_ms;
        tracked_habit->done = tracked_habit->value >= tracked_habit->goal;
        save_habits();
      }
    }
  }
  s_tracking_habit_id[0] = '\0';
  s_tracking_habit_start_epoch = 0;
  s_habit_countdown_paused = false;
  s_habit_countdown_frozen_elapsed_ms = 0;
  save_habit_tracking();
  stop_habit_tracking_tick();
}
#endif

static uint16_t habits_menu_get_num_sections(MenuLayer *menu_layer, void *context) {
  return 1;
}

static uint16_t habits_menu_get_num_rows(MenuLayer *menu_layer, uint16_t section_index, void *context) {
#ifdef PBL_PLATFORM_APLITE
  // Matches resolve_habit_at's own aplite-only filtering - see its comment.
  uint16_t visible = 0;
  for (int i = 0; i < s_habit_count; i++) {
    if (!s_habits[i].is_stopwatch) {
      visible++;
    }
  }
  return visible;
#else
  return (uint16_t)s_habit_count;
#endif
}

static void habits_menu_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context) {
  Habit *habit = resolve_habit_at(*cell_index);
  if (!habit) {
    return;
  }
  bool is_selected = menu_layer_get_selected_index(s_habits_menu_layer).row == cell_index->row;
  GRect bounds = layer_get_bounds(cell_layer);
  // Matches the Habits nav row's own background (menu_draw_row's section-0
  // branch) rather than the plain black every other selected row in this
  // app uses, so a highlighted habit visually ties back to the row that
  // brought you here.
  GColor bg = is_selected ? GColorVividCerulean : GColorWhite;
  // Unlike a done TASK's title (which dims - see menu_draw_row), a done
  // HABIT's title stays full-strength black/white: the "- Done" subtitle
  // already carries that signal, and a habit routinely gets incremented
  // past goal (still "done") or decremented back below it on the same day,
  // so dimming here would flicker on every single increment/decrement
  // rather than mark a settled, no-longer-relevant task.
  // Text stays black even when selected (unlike every other selectable row
  // in this app, which inverts to white) - the cerulean selected background
  // here is light enough that black reads better on it than white does.
  GColor fg = GColorBlack;
  graphics_context_set_fill_color(ctx, bg);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  graphics_context_set_text_color(ctx, fg);

  GRect title_box = GRect(TITLE_BOX_X, TITLE_BOX_Y, bounds.size.w - TITLE_BOX_X * 2, 30);
  graphics_draw_text(ctx, habit->title, fonts_get_system_font(TITLE_FONT_KEY), title_box,
                      GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  // "value/goal" always visible, with " - Done" appended once today's count
  // reaches goal - same dash-separated "two settled facts" convention the
  // task list's own subtitle uses for due time + tracked time. Keeping the
  // count alongside "Done" (rather than replacing it, as this used to)
  // matters once incrementing past goal is possible: "Done" alone can't
  // tell you 3/3 from 7/3.
  // Sized for the worst case of the StopWatch branch below: two 20-byte
  // format_duration_ms outputs joined by " / " plus " - Done" (matches
  // menu_draw_row's own combined[48] sizing for the analogous task subtitle,
  // with extra headroom for " - Done").
  char subtitle[56];
#ifndef PBL_PLATFORM_APLITE
  if (habit->is_stopwatch) {
    // Same "spent / estimate" formatting as a task's own timer subtitle
    // (menu_draw_row), reusing format_duration_ms directly - value/goal are
    // ms here, not a plain count. effective_ms adds this session's
    // still-running elapsed time on top of the last-synced value, same as
    // menu_draw_row's effective_ms does for a tracked task.
    bool is_tracking_this = s_tracking_habit_id[0] != '\0' &&
                             strncmp(s_tracking_habit_id, habit->id, MAX_HABIT_ID_LEN) == 0;
    int effective_ms = habit->value;
    if (is_tracking_this) {
      time_t elapsed_s = time(NULL) - s_tracking_habit_start_epoch;
      if (elapsed_s > 0) {
        effective_ms += (int)elapsed_s * 1000;
      }
    }
    bool effective_done = effective_ms >= habit->goal;
    char time_text[20];
    format_duration_ms(effective_ms, is_tracking_this, time_text, sizeof(time_text));
    char goal_text[20];
    format_duration_ms(habit->goal, false, goal_text, sizeof(goal_text));
    if (effective_done) {
      snprintf(subtitle, sizeof(subtitle), "%s / %s - Done", time_text, goal_text);
    } else {
      snprintf(subtitle, sizeof(subtitle), "%s / %s", time_text, goal_text);
    }
  } else if (habit->is_countdown) {
    // While its timer is running: remaining time counting down to zero,
    // format_duration_ms's "> " running-prefix doubling as "this is live"
    // same as it does for a StopWatch habit (it doesn't distinguish
    // counting up from counting down, just static-vs-live) - dropped while
    // paused, since a frozen number isn't "live" anymore. Otherwise: a
    // plain completed-rounds count, identical in shape to a ClickCounter's
    // own value/goal - a RepeatedCountdownReminder's countOnDay is rounds
    // completed today, not ms, unlike a StopWatch's.
    bool is_tracking_this = s_tracking_habit_id[0] != '\0' &&
                             strncmp(s_tracking_habit_id, habit->id, MAX_HABIT_ID_LEN) == 0;
    if (is_tracking_this) {
      int remaining_ms = habit->countdown_ms - countdown_elapsed_ms();
      if (remaining_ms < 0) {
        remaining_ms = 0;
      }
      char time_text[20];
      format_duration_ms(remaining_ms, !s_habit_countdown_paused, time_text, sizeof(time_text));
      snprintf(subtitle, sizeof(subtitle), s_habit_countdown_paused ? "%s left - Paused" : "%s left", time_text);
    } else if (habit->done) {
      snprintf(subtitle, sizeof(subtitle), "%d/%d - Done", habit->value, habit->goal);
    } else {
      snprintf(subtitle, sizeof(subtitle), "%d/%d", habit->value, habit->goal);
    }
  } else
#endif
  // On aplite, habit->is_stopwatch/is_countdown are never true here -
  // resolve_habit_at's own aplite-only filtering never returns either (see
  // its comment) - so both branches above are unreachable there and
  // compiled out entirely, leaving just this plain-count path.
  if (habit->done) {
    snprintf(subtitle, sizeof(subtitle), "%d/%d - Done", habit->value, habit->goal);
  } else {
    snprintf(subtitle, sizeof(subtitle), "%d/%d", habit->value, habit->goal);
  }
  GRect subtitle_box = GRect(TITLE_BOX_X, bounds.size.h - 22, bounds.size.w - TITLE_BOX_X * 2, 22);
  graphics_draw_text(ctx, subtitle, fonts_get_system_font(SUBTITLE_FONT_KEY), subtitle_box,
                      GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
}

// Select bumps today's count up by one, long-select down by one (never
// below 0) - the phone applies the delta against its own cached value and
// uploads the resulting total as a plain replace (see handleHabitAdjust in
// index.js), so this optimistic local bump is just a guess at what that'll
// resolve to, corrected by the next full sync same as everywhere else
// optimistic updates happen in this app.
static void adjust_habit(MenuIndex index, int32_t delta) {
  Habit *habit = resolve_habit_at(index);
  if (!habit || habit->value + delta < 0) {
    // Already at 0 and trying to go lower - a silent no-op, same as a
    // long-press track-time attempt on an already-done task elsewhere in
    // this app, rather than a separate error/feedback path.
    return;
  }
  habit->value += delta;
  habit->done = habit->value >= habit->goal;
  save_habits();
  menu_layer_reload_data(s_habits_menu_layer);
  send_habit_adjust(habit, delta);
}

static void habits_menu_select_click(MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  backlight_touch();
  Habit *habit = resolve_habit_at(*cell_index);
#ifndef PBL_PLATFORM_APLITE
  // Select pauses/resumes an is_countdown habit's timer while IT's the one
  // currently tracking (long-select still ends the round outright, same as
  // it always has) - not offered for a StopWatch, since "pause" isn't a
  // meaningful state for its open-ended up-count. If nothing (or a
  // different habit) is tracking this row, falls through to the same
  // no-op every other StopWatch/countdown Select already was - long-select
  // is still what starts a fresh round.
  if (habit && habit->is_countdown) {
    bool is_tracking_this = s_tracking_habit_id[0] != '\0' &&
                             strncmp(s_tracking_habit_id, habit->id, MAX_HABIT_ID_LEN) == 0;
    if (is_tracking_this) {
      toggle_habit_countdown_pause();
    }
    return;
  }
#endif
  // is_countdown is never true on aplite (see MSG_HABIT_ITEM's own comment)
  // - checking is_stopwatch alone there saves the few bytes an always-false
  // "|| is_countdown" would otherwise cost against aplite's especially
  // tight combined code+data budget (see MAX_HABITS' own comment).
#ifdef PBL_PLATFORM_APLITE
  if (habit && habit->is_stopwatch) {
#else
  if (habit && (habit->is_stopwatch || habit->is_countdown)) {
#endif
    // No plain-count action for a StopWatch/countdown habit - see
    // long-select, which starts/stops its timer instead (same button this
    // app's task list already uses for time tracking).
    return;
  }
  adjust_habit(*cell_index, 1);
}

// Long-select toggles the timer on a StopWatch or RepeatedCountdownReminder
// habit (starts it if nothing, or a different habit, is being tracked;
// stops it if this habit is the one already being tracked - mirrors
// menu_select_long_click's task timer exactly, including only one habit
// tracking at a time regardless of type), or decrements a plain ClickCounter
// habit by one otherwise. stop_habit_tracking_and_report() itself knows the
// difference between the two timer types (upload elapsed ms vs. a silent
// cancel - see its own comment), so this code doesn't need to.
static void habits_menu_select_long_click(MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  backlight_touch();
  Habit *habit = resolve_habit_at(*cell_index);
  // Same aplite-only is_stopwatch-alone shortcut as habits_menu_select_click
  // above - see its comment.
#ifdef PBL_PLATFORM_APLITE
  if (habit && habit->is_stopwatch) {
#else
  if (habit && (habit->is_stopwatch || habit->is_countdown)) {
#endif
#ifndef PBL_PLATFORM_APLITE
    bool already_tracking_this = s_tracking_habit_id[0] != '\0' &&
                                  strncmp(s_tracking_habit_id, habit->id, MAX_HABIT_ID_LEN) == 0;
    stop_habit_tracking_and_report();
    if (!already_tracking_this) {
      start_habit_tracking(habit);
    }
    menu_layer_reload_data(s_habits_menu_layer);
#endif
    // No tracking capability on aplite (see s_tracking_habit_id's own
    // comment) - a long-press on a StopWatch/countdown row is a silent
    // no-op there, same as everywhere else in this app an unavailable
    // action is ignored rather than given a separate error/feedback path.
    return;
  }
  adjust_habit(*cell_index, -1);
}

static void update_habits_empty_layer(void) {
  // habits_menu_get_num_rows(NULL, 0, NULL) rather than a plain s_habit_count
  // check - on aplite those can disagree (a habit list containing only
  // StopWatch-type entries has s_habit_count > 0 but zero VISIBLE rows there
  // - see habits_menu_get_num_rows/resolve_habit_at's own aplite filtering),
  // which would otherwise leave a blank menu instead of the empty-state text.
  bool show_empty = (habits_menu_get_num_rows(NULL, 0, NULL) == 0);
  layer_set_hidden(text_layer_get_layer(s_habits_empty_layer), !show_empty);
  layer_set_hidden(menu_layer_get_layer(s_habits_menu_layer), show_empty);
}

#ifndef PBL_PLATFORM_APLITE
static void habits_menu_selection_changed(MenuLayer *menu_layer, MenuIndex new_index, MenuIndex old_index, void *context) {
  backlight_touch();
}
#endif

static void habits_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  backlight_touch();

  const int16_t status_bar_height = STATUS_BAR_LAYER_HEIGHT;
  s_habits_status_bar = status_bar_layer_create();
  layer_add_child(window_layer, status_bar_layer_get_layer(s_habits_status_bar));

  GRect content_bounds = GRect(bounds.origin.x, bounds.origin.y + status_bar_height,
                                bounds.size.w, bounds.size.h - status_bar_height);

  s_habits_menu_layer = menu_layer_create(content_bounds);
  menu_layer_set_callbacks(s_habits_menu_layer, NULL, (MenuLayerCallbacks) {
    .get_num_sections = habits_menu_get_num_sections,
    .get_num_rows = habits_menu_get_num_rows,
    .draw_row = habits_menu_draw_row,
    .select_click = habits_menu_select_click,
    .select_long_click = habits_menu_select_long_click,
#ifndef PBL_PLATFORM_APLITE
    .selection_changed = habits_menu_selection_changed,
#endif
  });
  menu_layer_set_click_config_onto_window(s_habits_menu_layer, window);
  layer_add_child(window_layer, menu_layer_get_layer(s_habits_menu_layer));

  s_habits_empty_layer = text_layer_create(content_bounds);
  text_layer_set_text_alignment(s_habits_empty_layer, GTextAlignmentCenter);
  text_layer_set_font(s_habits_empty_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text(s_habits_empty_layer, "No habits synced.");
  layer_add_child(window_layer, text_layer_get_layer(s_habits_empty_layer));

  update_habits_empty_layer();

#ifndef PBL_PLATFORM_APLITE
  // Resumes a StopWatch/countdown habit's live-ticking redraw if one was
  // already being tracked (elapsed time itself is derived from the
  // persisted start timestamp regardless - see load_habit_tracking() in
  // init() - this is just about getting the redraw going again), same
  // reasoning as window_load's own equivalent for task tracking. Not while
  // paused, though - a paused countdown has nothing left to tick (see
  // toggle_habit_countdown_pause), so this would just be a wasted timer
  // ticking a frozen display.
  if (s_tracking_habit_id[0] != '\0' && !s_habit_countdown_paused) {
    start_habit_tracking_tick();
  }
#endif
}

static void habits_window_unload(Window *window) {
#ifndef PBL_PLATFORM_APLITE
  // Cancel before destroying s_habits_menu_layer, not after - the tick
  // timer's own callback checks the pointer but a still-running timer
  // touching a just-destroyed layer is exactly what window_unload's own
  // stop_tracking_tick()-before-menu_layer_destroy ordering avoids too.
  stop_habit_tracking_tick();
#endif
  menu_layer_destroy(s_habits_menu_layer);
  text_layer_destroy(s_habits_empty_layer);
  status_bar_layer_destroy(s_habits_status_bar);
}

// Created once and reused (pushed again on every visit) rather than
// destroyed/recreated at the Window* level - only its layers are torn down
// and rebuilt each time (habits_window_load/unload), which is what actually
// matters for memory: at most one window's worth of layers exists at a
// time, since Back always pops this back off before returning to the main
// window (Pebble's default unhandled-BACK behavior - no click config needed
// here for it).
static void push_habits_window(void) {
  if (!s_habits_window) {
    s_habits_window = window_create();
    window_set_window_handlers(s_habits_window, (WindowHandlers) {
      .load = habits_window_load,
      .unload = habits_window_unload,
    });
  }
  window_stack_push(s_habits_window, true);
}

#ifndef PBL_PLATFORM_APLITE
// Select dismisses the notes window, same convention as every other
// overlay's Select-to-dismiss in this app - Back also dismisses it, for
// free, via Pebble's own default pop behavior (see s_notes_window's own
// comment) rather than anything subscribed here.
static void notes_window_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  hide_notes_overlay();
}

// Long-select dictates text to append to the currently-shown task's notes
// - see start_note_append_dictation.
static void notes_window_select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  start_note_append_dictation();
}

// Not menu_layer_set_click_config_onto_window - this window has no MenuLayer
// to compose with. Installed onto the ScrollLayer (not the window directly)
// via scroll_layer_set_click_config_onto_window/set_callbacks below, which
// wires UP/DOWN to scrolling first and then calls this for SELECT - a note
// long enough to need scrolling was exactly the un-openable case before the
// ScrollLayer was added (word-wrapped text past screen height had no way to
// bring the rest on screen; this app has no touchscreen on any supported
// hardware, so UP/DOWN are the only way to move it).
static void notes_window_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, notes_window_select_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 0, notes_window_select_long_click_handler, NULL);
}

static void notes_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  backlight_touch();

  const int16_t status_bar_height = STATUS_BAR_LAYER_HEIGHT;
  s_notes_status_bar = status_bar_layer_create();
  layer_add_child(window_layer, status_bar_layer_get_layer(s_notes_status_bar));

  GRect content_bounds = GRect(bounds.origin.x, bounds.origin.y + status_bar_height,
                                bounds.size.w, bounds.size.h - status_bar_height);

  // The TextLayer's own frame has to be sized to the FULL wrapped text
  // height up front, not just the visible viewport - it clips its own
  // content to its frame regardless of the ScrollLayer's (separate) content
  // size, so a frame sized only to the viewport would still cut the text
  // off even once the ScrollLayer itself knows there's more to scroll to.
  int16_t text_height = measure_notes_text_height(content_bounds.size.w);
  if (text_height < content_bounds.size.h) {
    text_height = content_bounds.size.h;
  }

  s_notes_scroll_layer = scroll_layer_create(content_bounds);
  scroll_layer_set_content_size(s_notes_scroll_layer, GSize(content_bounds.size.w, text_height));
  scroll_layer_set_click_config_onto_window(s_notes_scroll_layer, window);
  scroll_layer_set_callbacks(s_notes_scroll_layer, (ScrollLayerCallbacks) {
    .click_config_provider = notes_window_click_config_provider,
  });

  // Left-aligned and top-anchored (unlike the centered, short status text
  // s_error_layer uses) since this is body text, not a status message -
  // word-wrap fills top-down from there. A smaller font than the error
  // layer's bold 18pt: notes can run to MAX_NOTES_LEN characters, several
  // times longer than any error message this app ever shows.
  s_notes_layer = text_layer_create(GRect(0, 0, content_bounds.size.w, text_height));
  text_layer_set_text_alignment(s_notes_layer, GTextAlignmentLeft);
  text_layer_set_font(s_notes_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_background_color(s_notes_layer, GColorWhite);
  text_layer_set_text_color(s_notes_layer, GColorBlack);
  text_layer_set_overflow_mode(s_notes_layer, GTextOverflowModeWordWrap);
  // s_notes_overlay_text was already populated by show_notes_overlay() just
  // before this window was pushed - window_stack_push doesn't invoke this
  // load handler synchronously, so the text has to already be sitting
  // somewhere this can read it back out from.
  text_layer_set_text(s_notes_layer, s_notes_overlay_text);
  scroll_layer_add_child(s_notes_scroll_layer, text_layer_get_layer(s_notes_layer));

  layer_add_child(window_layer, scroll_layer_get_layer(s_notes_scroll_layer));
}

static void notes_window_unload(Window *window) {
  text_layer_destroy(s_notes_layer);
  s_notes_layer = NULL;
  scroll_layer_destroy(s_notes_scroll_layer);
  s_notes_scroll_layer = NULL;
  status_bar_layer_destroy(s_notes_status_bar);
  // The single source of truth for "is the notes window currently up" -
  // cleared here rather than in hide_notes_overlay() itself so a
  // Back-triggered dismissal (which never calls hide_notes_overlay - see
  // its own comment) still clears this exactly the same way a
  // Select-triggered one does.
  s_notes_overlay_active = false;
}

// Created once and reused (pushed again on every visit), same reasoning as
// push_habits_window above - only its layers are torn down/rebuilt each
// time.
static void push_notes_window(void) {
  if (!s_notes_window) {
    s_notes_window = window_create();
    window_set_window_handlers(s_notes_window, (WindowHandlers) {
      .load = notes_window_load,
      .unload = notes_window_unload,
    });
  }
  window_stack_push(s_notes_window, true);
}
#endif

// ---------- window lifecycle ----------

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  backlight_touch();

  const int16_t status_bar_height = STATUS_BAR_LAYER_HEIGHT;
  s_status_bar = status_bar_layer_create();
  layer_add_child(window_layer, status_bar_layer_get_layer(s_status_bar));

  GRect content_bounds = GRect(bounds.origin.x, bounds.origin.y + status_bar_height,
                                bounds.size.w, bounds.size.h - status_bar_height);

  s_menu_layer = menu_layer_create(content_bounds);
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
    .get_num_sections = menu_get_num_sections,
    .get_num_rows = menu_get_num_rows,
    .get_header_height = menu_get_header_height,
    .draw_header = menu_draw_header,
    .draw_row = menu_draw_row,
    .select_click = menu_select_click,
    .select_long_click = menu_select_long_click,
    .selection_changed = menu_selection_changed,
  });
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
  // Covers a persisted (previously cached) task list whose initial
  // selection already needs to scroll, before any sync response arrives.
  refresh_scroll_state(true);

  // Logo sits in a fixed-height strip at the bottom of the empty-state area;
  // the text layer gets whatever's left above it, rather than the full
  // content_bounds it used to own alone.
  #define LOGO_SIZE 50
  #define LOGO_STRIP_HEIGHT 58
  GRect empty_text_bounds = GRect(content_bounds.origin.x, content_bounds.origin.y,
                                   content_bounds.size.w, content_bounds.size.h - LOGO_STRIP_HEIGHT);
  s_empty_layer = text_layer_create(empty_text_bounds);
  text_layer_set_text_alignment(s_empty_layer, GTextAlignmentCenter);
  text_layer_set_font(s_empty_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_layer, text_layer_get_layer(s_empty_layer));

#ifndef PBL_PLATFORM_APLITE
  // Bottom slice of the same text area, reserved for s_sync_progress_layer
  // (see its own comment) - sized off empty_text_bounds so it scales with
  // whatever room each platform's screen actually has, same as
  // empty_text_bounds itself. Hidden by default; only shown while the
  // initial sync's title is up above it in the larger font.
  #define SYNC_PROGRESS_HEIGHT 36
  GRect sync_progress_bounds = GRect(empty_text_bounds.origin.x,
                                      empty_text_bounds.origin.y + empty_text_bounds.size.h - SYNC_PROGRESS_HEIGHT,
                                      empty_text_bounds.size.w, SYNC_PROGRESS_HEIGHT);
  s_sync_progress_layer = text_layer_create(sync_progress_bounds);
  text_layer_set_text_alignment(s_sync_progress_layer, GTextAlignmentCenter);
  text_layer_set_font(s_sync_progress_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  layer_set_hidden(text_layer_get_layer(s_sync_progress_layer), true);
  layer_add_child(window_layer, text_layer_get_layer(s_sync_progress_layer));
#endif

  GRect logo_bounds = GRect(content_bounds.origin.x + (content_bounds.size.w - LOGO_SIZE) / 2,
                             content_bounds.origin.y + content_bounds.size.h - LOGO_STRIP_HEIGHT,
                             LOGO_SIZE, LOGO_SIZE);
  s_logo_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LOGO_LARGE);
  s_logo_layer = bitmap_layer_create(logo_bounds);
  bitmap_layer_set_bitmap(s_logo_layer, s_logo_bitmap);
  bitmap_layer_set_compositing_mode(s_logo_layer, GCompOpSet);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_logo_layer));

  // Row icons for the Resync/Habits rows (menu_draw_row) - loaded once here
  // rather than per-draw, same reasoning as s_logo_bitmap above.
  s_check_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MENU_ICON);
  s_check_white_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CHECK_WHITE);
  s_heart_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HEART_CHECK);
  s_heart_white_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HEART_CHECK_WHITE);

  // Add Task row + dictation session - mic-equipped platforms only, compiled
  // out entirely on aplite (see s_mic_bitmap's own comment for why this is
  // a real #ifndef and not just a runtime check).
#ifndef PBL_PLATFORM_APLITE
  s_mic_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MICROPHONE);
  s_mic_white_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MICROPHONE_WHITE);
  s_dictation_session = dictation_session_create(MAX_TITLE_LEN, dictation_status_callback, NULL);
  // Lets the user review/retry the transcription before it's sent, rather
  // than blind-sending whatever the recognizer heard.
  dictation_session_enable_confirmation(s_dictation_session, true);
#endif

  // Full content_bounds (not the logo-strip-shrunk box the empty-state text
  // gets), and added last so it draws on top of every other layer here
  // when shown - covers the whole content area, not just another line of
  // subtitle text, so a sync error is actually readable instead of easy to
  // miss. Hidden by default; only show_error_overlay() reveals it.
  s_error_layer = text_layer_create(content_bounds);
  text_layer_set_text_alignment(s_error_layer, GTextAlignmentCenter);
  text_layer_set_font(s_error_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_background_color(s_error_layer, GColorRed);
  text_layer_set_text_color(s_error_layer, GColorBlack);
  text_layer_set_overflow_mode(s_error_layer, GTextOverflowModeWordWrap);
  layer_set_hidden(text_layer_get_layer(s_error_layer), true);
  layer_add_child(window_layer, text_layer_get_layer(s_error_layer));

  update_empty_layer();
  request_sync();

  // Resumes a tracking session that was already running when the watchapp
  // was last closed (see load_tracking() in init()) - the elapsed time is
  // derived from the persisted start timestamp either way, so this is just
  // about getting the live-ticking redraw going again, not about the
  // elapsed total itself.
  if (s_tracking_task_id[0] != '\0') {
    start_tracking_tick();
  }
}

static void window_unload(Window *window) {
  stop_scroll_timer();
  // Deliberately NOT stopping tracking here (see s_tracking_task_id's own
  // comment) - only cancels this window's own redraw timer, since
  // s_menu_layer (what it redraws) is about to be destroyed too.
  stop_tracking_tick();
  stop_syncing_animation();
  menu_layer_destroy(s_menu_layer);
  text_layer_destroy(s_empty_layer);
#ifndef PBL_PLATFORM_APLITE
  text_layer_destroy(s_sync_progress_layer);
#endif
  text_layer_destroy(s_error_layer);
#ifndef PBL_PLATFORM_APLITE
  if (s_pending_toggle_timer) {
    app_timer_cancel(s_pending_toggle_timer);
    s_pending_toggle_timer = NULL;
  }
#endif
  bitmap_layer_destroy(s_logo_layer);
  gbitmap_destroy(s_logo_bitmap);
  gbitmap_destroy(s_check_bitmap);
  gbitmap_destroy(s_check_white_bitmap);
  gbitmap_destroy(s_heart_bitmap);
  gbitmap_destroy(s_heart_white_bitmap);
#ifndef PBL_PLATFORM_APLITE
  dictation_session_destroy(s_dictation_session);
  gbitmap_destroy(s_mic_bitmap);
  gbitmap_destroy(s_mic_white_bitmap);
#endif
  status_bar_layer_destroy(s_status_bar);
}

static void init(void) {
#ifndef PBL_PLATFORM_APLITE
  // Detects a session that exists purely because schedule_next_wakeup()'s
  // wakeup fired, as opposed to the user opening the app - see
  // s_is_wakeup_launch's own comment for what this gates.
  s_is_wakeup_launch = launch_reason() == APP_LAUNCH_WAKEUP;
  if (s_is_wakeup_launch) {
    APP_LOG(APP_LOG_LEVEL_INFO, "launched by wakeup event");
  }
#endif
  load_tasks();
  load_habits();
  load_tracking();
#ifndef PBL_PLATFORM_APLITE
  load_habit_tracking();
#endif
  recompute_groups();

  app_message_register_inbox_received(inbox_received_handler);
  app_message_register_inbox_dropped(inbox_dropped_handler);
  app_message_register_outbox_failed(outbox_failed_handler);
#ifndef PBL_PLATFORM_APLITE
  app_message_register_outbox_sent(outbox_sent_handler);
#endif
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_main_window, true);
}

static void deinit(void) {
#ifndef PBL_PLATFORM_APLITE
  // Relinquish the backlight back to automatic control before exiting -
  // otherwise an always-on or mid-timeout override from this session would
  // otherwise persist past the app's own lifetime, per light_enable()'s own
  // docs warning about battery drain from leaving it forced on.
  if (s_backlight_timer) {
    app_timer_cancel(s_backlight_timer);
  }
  if (s_backlight_mode != 0) {
    light_enable(false);
  }
#endif
  window_destroy(s_main_window);
  if (s_habits_window) {
    window_destroy(s_habits_window);
  }
#ifndef PBL_PLATFORM_APLITE
  if (s_notes_window) {
    window_destroy(s_notes_window);
  }
#endif
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
