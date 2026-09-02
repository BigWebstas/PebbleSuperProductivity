#include <pebble.h>

// Keep in sync by hand with package.json "version" on every bump - no runtime
// API exposes it to C.
#define APP_VERSION "0.6.29"

// MESSAGE_KEY_* come from message_keys.auto.h (generated from package.json's
// "messageKeys"); AppMessage assigns IDs from 10000, so a 0-based enum wouldn't
// match the phone.
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
#define KEY_AUTO_SYNC_INTERVAL_MIN MESSAGE_KEY_AUTO_SYNC_INTERVAL_MIN
#define KEY_NOTE_TEXT MESSAGE_KEY_NOTE_TEXT
#define KEY_NOTE_TOTAL_LEN MESSAGE_KEY_NOTE_TOTAL_LEN
#define KEY_NOTE_CHUNK_TEXT MESSAGE_KEY_NOTE_CHUNK_TEXT
#define KEY_TASK_PROJECT_ID MESSAGE_KEY_TASK_PROJECT_ID
#define KEY_TASK_PROJECT_COLOR MESSAGE_KEY_TASK_PROJECT_COLOR
#define KEY_PROJECT_ID MESSAGE_KEY_PROJECT_ID
#define KEY_PROJECT_INDEX MESSAGE_KEY_PROJECT_INDEX
#define KEY_PROJECT_TITLE MESSAGE_KEY_PROJECT_TITLE
#define KEY_PROJECT_COLOR MESSAGE_KEY_PROJECT_COLOR
#define KEY_PROJECT_TOTAL MESSAGE_KEY_PROJECT_TOTAL
#define KEY_PROJECT_TASK_BACKLOG MESSAGE_KEY_PROJECT_TASK_BACKLOG
#define KEY_PROJECTS_ENABLED MESSAGE_KEY_PROJECTS_ENABLED
#define KEY_TASK_TAGS MESSAGE_KEY_TASK_TAGS
#define KEY_TOUCH_NAV_ENABLED MESSAGE_KEY_TOUCH_NAV_ENABLED
#define KEY_OVERTIME_NOTIFY_ENABLED MESSAGE_KEY_OVERTIME_NOTIFY_ENABLED
#define KEY_OVERTIME_REPEAT_ENABLED MESSAGE_KEY_OVERTIME_REPEAT_ENABLED
#define KEY_PRESENCE_STATE MESSAGE_KEY_PRESENCE_STATE
#define KEY_PRESENCE_TASK_TITLE MESSAGE_KEY_PRESENCE_TASK_TITLE
#define KEY_PRESENCE_DEVICE MESSAGE_KEY_PRESENCE_DEVICE
#define KEY_PRESENCE_ELAPSED_S MESSAGE_KEY_PRESENCE_ELAPSED_S
#define KEY_PRESENCE_CAN_STOP MESSAGE_KEY_PRESENCE_CAN_STOP

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
  MSG_NOTE_REQUEST = 16,    // watch -> phone: TASK_ID (ask for this task's full notes, chunked reply)
  MSG_NOTE_SYNC_START = 17, // phone -> watch: TASK_ID + NOTE_TOTAL_LEN (bytes about to follow, 0 = no notes)
  MSG_NOTE_CHUNK = 18,      // phone -> watch: TASK_ID + NOTE_CHUNK_TEXT (append this chunk)
  MSG_NOTE_SYNC_END = 19,   // phone -> watch: TASK_ID (all chunks sent, render now)
  // Project notes reuse the task-notes fetch/append machinery, keyed by
  // PROJECT_ID. The real app has no single "project notes" field (a project has
  // a list of Note entities); the phone treats the oldest one as the synthetic
  // "project note", creating it on first append.
  MSG_PROJECT_NOTE_APPEND = 20,     // watch -> phone: PROJECT_ID + NOTE_TEXT
  MSG_PROJECT_NOTE_REQUEST = 21,    // watch -> phone: PROJECT_ID
  MSG_PROJECT_NOTE_SYNC_START = 22, // phone -> watch: PROJECT_ID + NOTE_TOTAL_LEN
  MSG_PROJECT_NOTE_CHUNK = 23,      // phone -> watch: PROJECT_ID + NOTE_CHUNK_TEXT
  MSG_PROJECT_NOTE_SYNC_END = 24,   // phone -> watch: PROJECT_ID
  MSG_TASK_PLAN_TOMORROW = 25,      // watch -> phone: TASK_ID (set the task's dueDay to tomorrow)
  MSG_TASK_UNSCHEDULE = 26,         // watch -> phone: TASK_ID (clear the task's scheduling)
  // Live tracking presence (SuperSync only, opt-in, non-aplite). PRESENCE_STATE:
  // 0 none, 1 tracking, 2 paused, 3 was-tracking, 4 stopped (brief linger);
  // when != 0 the message also carries PRESENCE_TASK_TITLE / PRESENCE_DEVICE /
  // PRESENCE_ELAPSED_S / PRESENCE_CAN_STOP. The phone owns the session id and
  // the remote stop.
  MSG_PRESENCE_UPDATE = 27,         // phone -> watch: PRESENCE_* (STATE 0 hides the LIVE UI)
  MSG_PRESENCE_STOP = 28,           // watch -> phone: stop the session shown in the LIVE UI
  // Phase 2 - the watch broadcasts its own time-tracking as presence ("Pebble").
  MSG_TRACK_TIME_START = 29,        // watch -> phone: TASK_ID + TRACKED_MS (elapsed so far, 0 on a fresh start)
  MSG_PRESENCE_STOP_LOCAL = 30,     // phone -> watch: a remote device stopped this watch's timer - stop it here
  // Projects browser (config.enableProjects, non-aplite). The watch asks for
  // the project list, then for one project's tasks; each reply is a
  // START / ITEM* / END sequence like the task/habit list sends. A tasks ITEM
  // reuses the TASK_* keys plus PROJECT_TASK_BACKLOG (0 = the project's
  // regular list, 1 = its backlog). Every tasks message carries PROJECT_ID so
  // a reply for a project the watch has navigated away from is ignored.
  MSG_PROJECT_LIST_REQUEST = 31,    // watch -> phone: (no keys)
  MSG_PROJECT_LIST_START = 32,      // phone -> watch: PROJECT_TOTAL
  MSG_PROJECT_LIST_ITEM = 33,       // phone -> watch: PROJECT_INDEX + PROJECT_ID + PROJECT_TITLE
  MSG_PROJECT_LIST_END = 34,        // phone -> watch: (no keys)
  MSG_PROJECT_TASKS_REQUEST = 35,   // watch -> phone: PROJECT_ID
  MSG_PROJECT_TASKS_START = 36,     // phone -> watch: PROJECT_ID + TASK_TOTAL
  MSG_PROJECT_TASKS_ITEM = 37,      // phone -> watch: PROJECT_ID + TASK_INDEX + TASK_* + PROJECT_TASK_BACKLOG
  MSG_PROJECT_TASKS_END = 38,       // phone -> watch: PROJECT_ID
};

// STATUS_CODE values sent from the phone.
enum {
  STATUS_OK = 0,
  STATUS_SYNCING = 1,
  STATUS_NOT_PAIRED = 2,
  STATUS_ERROR = 3,
};

// emery (Pebble Time 2) has far more free heap than basalt/chalk/diorite, but
// its PebbleProcessInfo virtual size (.text+.data+.bss <= 64KB) is the real
// ceiling, and the Task double-buffer is a big chunk of .bss - so emery's cap
// is only modestly higher, kept where the Projects browser's extras still fit.
#ifdef PBL_PLATFORM_EMERY
#define MAX_TASKS 40
#else
#define MAX_TASKS 30
#endif
#define MAX_TITLE_LEN 64
// Generated task ids are ~21 chars, but calendar-integration ids
// (`cal_${issueProviderId}_${calendarEventId}`) have no fixed cap - a real one
// ran 79 chars. A too-short buffer truncates the id so uploads against it
// silently touch nothing server-side while the title still displays fine.
#define MAX_ID_LEN 96
#define MAX_PROJECT_LEN 32
// Project ids are a plain nanoid() - no calendar-id format, so 32 is enough.
#define MAX_PROJECT_ID_LEN 32
// Notes can be many paragraphs, too big to carry on every Task in the
// double-buffered s_tasks/s_incoming arrays. Fetched on demand instead
// (MSG_NOTE_*, see s_notes_full_text), malloc'd to the size the phone reports -
// no per-task cost, no length ceiling. aplite-gated (#ifndef): no RAM budget
// for the notes feature, so it keeps the plain instant single-click toggle.
// Tags, unlike notes, are short and pre-resolved phone-side, so they stay a
// fixed per-task field (phone truncates TASK_TAGS to 63 chars in sendTaskAt).
#define MAX_TASK_TAGS_LEN 64

// The grouped today view's project-row colour swatch. aplite has no grouping
// UI, so it's the only platform without it.
#ifndef PBL_PLATFORM_APLITE
#define TODAY_PROJECT_SWATCH 1
#else
#define TODAY_PROJECT_SWATCH 0
#endif

typedef struct {
  char id[MAX_ID_LEN];
  char title[MAX_TITLE_LEN];
  char project[MAX_PROJECT_LEN]; // '' when the phone isn't grouping by project
#ifndef PBL_PLATFORM_APLITE
  // Project id (not just the display name above) so the project row can fetch
  // that project's notes. aplite-gated with the rest of the notes feature.
  char project_id[MAX_PROJECT_ID_LEN];
  // Comma-joined tag names, '' if untagged - shown above the notes text in the
  // notes overlay. aplite-gated (no margin for another MAX_TASKS*2 field).
  char tags[MAX_TASK_TAGS_LEN];
#endif
  bool done;
#if TODAY_PROJECT_SWATCH
  // Packed GColor8 byte for this task's project's theme-colour swatch (0 =
  // none). Sits in the padding after `done`, costing the double-buffered
  // s_tasks/s_incoming arrays nothing. The grouped today view reads it off
  // whichever task starts each project group.
  uint8_t project_color;
#endif
  int due_min;       // minutes since local midnight, or -1 when the task has no dueWithTime
  int time_spent_ms; // total tracked time (all days, all devices), 0 if none
  int time_estimate_ms; // 0 if none
} Task;

// One entry per contiguous run of equal Task.project in s_tasks (the phone
// pre-sorts by project when grouping is on). Grouping off = '' for every task =
// one group covering the whole list, same as the old flat list.
typedef struct {
  char name[MAX_PROJECT_LEN];
  int start; // index into s_tasks
  int count;
#ifndef PBL_PLATFORM_APLITE
  // Copied from the group's first task so the selectable project row can fetch
  // this project's notes. aplite keeps a plain non-selectable header.
  char project_id[MAX_PROJECT_ID_LEN];
#endif
} TaskGroup;

// "Habits" are Super Productivity's SimpleCounter feature (entityType
// SIMPLE_COUNTER). Three types: ClickCounter (plain Select/long-Select +1/-1),
// StopWatch (ms-valued value/goal, long-Select start/stop timer), and
// RepeatedCountdownReminder (plain count + long-Select countdown timer).
// Caps kept low: aplite's ~24KB RAM is tight and 8 overflowed its binary by
// 280 bytes; the is_stopwatch field alone (needed everywhere to skip those
// rows) pushed the workable aplite number to 2. emery has ~95KB free.
#ifdef PBL_PLATFORM_APLITE
#define MAX_HABITS 2
#elif defined(PBL_PLATFORM_EMERY)
#define MAX_HABITS 16
#else
#define MAX_HABITS 8
#endif
// SimpleCounter ids are plain nanoid() - no calendar-id format, so 32 is enough.
#define MAX_HABIT_ID_LEN 32
typedef struct {
  char id[MAX_HABIT_ID_LEN];
  char title[MAX_TITLE_LEN];
  // Bools kept adjacent so they share one padding gap ahead of the ints.
  bool done;
  bool is_stopwatch; // StopWatch-type counter
  // RepeatedCountdownReminder: a plain count (today's completed rounds, not ms)
  // with a long-Select countdown timer. Always false on aplite (filtered out of
  // the visible list there, same as is_stopwatch).
  bool is_countdown;
  int value; // today's count, or ms tracked today when is_stopwatch
  int goal;  // streakMinValue-derived target for the "value/goal" subtitle
#ifndef PBL_PLATFORM_APLITE
  // Countdown length in ms for an is_countdown counter (0 otherwise). Only used
  // by the countdown-timer machinery, itself aplite-excluded.
  int countdown_ms;
#endif
} Habit;

// Single buffer, not the s_tasks/s_incoming double-buffer, to save RAM on
// aplite. Safe because nothing redraws the habits menu until MSG_HABIT_SYNC_END
// bumps s_habit_count.
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
// Subtitle beneath s_empty_layer's title, shown only during the initial
// (no-cached-list) sync - the progress percentage, or a "may take a few
// minutes" heads-up. Its own layer so the "Syncing..." title can use a bigger
// font without the hint text overflowing. aplite-excluded (a second TextLayer
// overflowed its APP region by 120 bytes) - there the percentage shows inline.
#ifndef PBL_PLATFORM_APLITE
static TextLayer *s_sync_progress_layer;
#endif
static BitmapLayer *s_logo_layer;
static GBitmap *s_logo_bitmap;
// Icons drawn directly into the Resync/Habits rows (menu_draw_row). One
// black/white pair per icon to invert on selection - GCompOpSet gives no free
// color-invert for an 8-bit source, so this is two assets, not one recolored.
#define ROW_ICON_SIZE 25
static GBitmap *s_check_bitmap;
static GBitmap *s_check_white_bitmap;
static GBitmap *s_heart_bitmap;
static GBitmap *s_heart_white_bitmap;
// Mic/dictation state - compiled out on aplite (#ifndef, not a runtime check):
// no mic hardware, can never reach the "Add Task" row, and a runtime-only
// guard cost aplite ~190 bytes of RAM headroom.
#ifndef PBL_PLATFORM_APLITE
static GBitmap *s_mic_bitmap;
static GBitmap *s_mic_white_bitmap;
// One session for the app's whole lifetime - the SDK doc confirms a session
// can be reused and restarted, so no need to recreate it per "Add Task" press.
static DictationSession *s_dictation_session;
// dictation_session_start() returns DictationSessionStatus, not a clean
// "already in progress" signal, so this flag guards against a rapid
// double-press. Set before starting, cleared at the top of every status
// callback.
static bool s_dictation_pending;
#endif
// A sync error's full message is otherwise only an easily-missed one-line
// subtitle on the Resync row - this takes over the whole content area, and
// deliberately does NOT auto-dismiss on the next status change so a transient
// retry succeeding can't yank it away before it's read. Only Select dismisses.
static TextLayer *s_error_layer;
static bool s_error_overlay_active = false;
#ifndef PBL_PLATFORM_APLITE
// Notes are shown in their own pushed Window, not a layer on s_main_window, so
// Back gets Pebble's default "pop this window" for free (dismissing to the task
// list). A layer-toggle overlay doesn't: Back on s_main_window always exits the
// app - a real reported bug (notes-overlay Back exited to the watchface).
static Window *s_notes_window;
static StatusBarLayer *s_notes_status_bar;
static ScrollLayer *s_notes_scroll_layer;
// A fixed header above s_notes_scroll_layer (not inside it), showing
// "Tags: urgent, home" on a colored background - stays put while the notes body
// scrolls under it. Shown for any task, tagged or not; zero-height only for a
// project subject (no tags-on-project concept). A plain custom-drawn Layer, not
// a TextLayer, so it can pad the banner text. Kept local rather than baked into
// s_notes_full_text since it must show even while the fetch is loading/empty.
static Layer *s_notes_tags_layer;
// Comma-joined tag names, empty when the task has none (NOTES_TAGS_EMPTY_TEXT
// substituted at draw time). Not prefixed with "Tags: " - that label is drawn
// separately in bold, and graphics_draw_text can't mix two weights in one call.
static char s_notes_tags_line[MAX_TASK_TAGS_LEN + 16] = "";
#define NOTES_TAGS_LABEL "Tags:"
#define NOTES_TAGS_EMPTY_TEXT "No tags for this task"
#define NOTES_TAGS_BG_COLOR GColorYellow
// Notes body + "Tags:" label fonts and tags-header padding, bumped a step on
// emery for its Large content size. MUST be used everywhere the notes text is
// measured and drawn or the measured and rendered heights drift and the body
// clips.
#ifdef PBL_PLATFORM_EMERY
#define NOTES_BODY_FONT_KEY FONT_KEY_GOTHIC_24
#define NOTES_LABEL_FONT_KEY FONT_KEY_GOTHIC_24_BOLD
#define NOTES_TAGS_PADDING_X 8
#define NOTES_TAGS_PADDING_Y 6
#else
#define NOTES_BODY_FONT_KEY FONT_KEY_GOTHIC_18
#define NOTES_LABEL_FONT_KEY FONT_KEY_GOTHIC_18_BOLD
#define NOTES_TAGS_PADDING_X 6
#define NOTES_TAGS_PADDING_Y 4
#endif
// The notes window's content area (below the status bar), set in
// notes_window_load, read by render_notes_overlay_content to re-slice between
// the tags header and the scrollable body as s_notes_tags_line's height changes.
static GRect s_notes_content_bounds;
static TextLayer *s_notes_layer;
static bool s_notes_overlay_active = false;
// Which task OR project the notes overlay is showing - a plain id copy (a
// background sync can rebuild s_tasks/s_groups while it's open). The one
// overlay/window serves both task and project notes; s_notes_overlay_is_project
// disambiguates which fetch/append message type a pending request/reply is for.
static char s_notes_overlay_subject_id[MAX_ID_LEN] = "";
static bool s_notes_overlay_is_project = false;
// Full notes text for s_notes_overlay_subject_id, fetched on demand
// (MSG_NOTE_*). malloc'd to NOTES_HEADER + the byte count the phone reports in
// MSG_NOTE_SYNC_START, holding "Notes:\n\n" then the chunks; NULL when nothing
// is owned. Freed and re-armed by every show_notes_overlay() and
// notes_window_unload - never left dangling.
static char *s_notes_full_text = NULL;
static int s_notes_full_len = 0;      // bytes written into s_notes_full_text so far
static int s_notes_full_capacity = 0; // malloc'd size of s_notes_full_text, including its null terminator
// What MSG_NOTE_SYNC_START reported, so MSG_NOTE_SYNC_END can render even when
// s_notes_full_text is NULL - true both for "no notes" and "malloc failed" /
// "START never arrived", which need different text.
typedef enum {
  NOTES_FETCH_IDLE,    // no START seen yet for the current request
  NOTES_FETCH_EMPTY,   // START reported zero-length notes
  NOTES_FETCH_STARTED, // START malloc'd s_notes_full_text; chunks may still be arriving
  NOTES_FETCH_FAILED,  // START reported real notes but malloc failed
} NotesFetchState;
static NotesFetchState s_notes_fetch_state = NOTES_FETCH_IDLE;
// What's shown right now - s_notes_full_text once a NOTES_FETCH_STARTED transfer
// ends, or one of the literals below. Never owned/freed here.
static const char *s_notes_display_text = "";
// Whether s_notes_display_text is the loading placeholder - tracked separately
// rather than comparing against the NOTES_LOADING_TEXT pointer (string-literal
// pointer comparison is UB and -Werror=address rejects it).
static bool s_notes_is_loading = false;
#define NOTES_LOADING_TEXT "Loading notes..."
#define NOTES_EMPTY_TEXT "(No notes for this task)"
#define PROJECT_NOTES_EMPTY_TEXT "(No notes for this project)"
#define NOTES_TIMEOUT_TEXT "Couldn't load notes - back out and try again."
// Guards a fetch that never completes (not paired, dropped AppMessage) from
// leaving the overlay stuck on "Loading..." with no signal. Canceled when a
// matching SYNC_START/END arrives; restarted on every new request.
static AppTimer *s_notes_load_timeout_timer = NULL;
// Generous: the NOTE_REQUEST send's own retry backoff can burn ~7s mid-sync
// before the phone even sees it, then the chunked reply has per-chunk retries.
// 8s spuriously fired for notes opened right after launch. A truly unreachable
// phone surfaces its own error from the send-retry path well before this.
#define NOTES_LOAD_TIMEOUT_MS 20000
// Distinguishes a dictation_status_callback for note-append from one for Add
// Task - both share the single s_dictation_session/s_dictation_pending pair.
static bool s_dictation_is_note_append = false;
// Double-click detection on Select: a single click starts this timer instead of
// committing the task-done toggle, so a second click on the same task can
// cancel it and show notes. Tracked by id, not Task* (a background sync can
// rebuild s_tasks under a pending click; a stale id just fails to resolve).
static AppTimer *s_pending_toggle_timer = NULL;
static char s_pending_toggle_task_id[MAX_ID_LEN] = "";
// Matches the SDK's default multi-click window (300ms) - reimplemented by hand
// since MenuLayerCallbacks has no multi-click hook and MenuLayer already owns
// the window's click config.
#define DOUBLE_CLICK_WINDOW_MS 300

// Long-press Up (unschedule) and long-press Down / swipe-left (move to tomorrow)
// each open a 3s cancel window on the selected task, reusing the pending-toggle
// pattern above: the row's subtitle shows "Moving to tomorrow..." / "Un-
// Scheduling..." and a single Select cancels before the timer commits. Tracked
// by id for the same background-sync reason. aplite-excluded for RAM (like the
// send-retry buffer) - and the swipe half is PBL_TOUCH-only regardless.
#define RESCHEDULE_WINDOW_MS 3000
// Below the SDK's 500ms default so a deliberate hold commits before UP/DOWN's
// repeat-scroll walks the selection too far off the intended row.
#define RESCHEDULE_LONGPRESS_MS 400
typedef enum { RESCHEDULE_NONE, RESCHEDULE_TOMORROW, RESCHEDULE_UNSCHEDULE } RescheduleKind;
static AppTimer *s_pending_reschedule_timer = NULL;
static char s_pending_reschedule_task_id[MAX_ID_LEN] = "";
static RescheduleKind s_pending_reschedule_kind = RESCHEDULE_NONE;
#endif

#if defined(PBL_TOUCH)
// A touch tap on the task / habits list only moves the selection - it must not
// also toggle the row done / bump the habit the way the physical Select button
// does. The touch bridge synthesises a SELECT click for every tap; when a tap
// arms this guard (see arm_tap_select_guard by the touch handler) the next
// menu_select_click / habits_menu_select_click swallows that one click.
// Physical Select never routes through the guard. Time-boxed so a stray arm
// (bridge fired no click) can't swallow a later button press.
static bool s_ignore_next_menu_select = false;
static AppTimer *s_tap_guard_timer = NULL;
static void clear_tap_select_guard(void);
static bool consume_tap_select_guard(void);
#endif

// Time tracking: long-select starts/stops tracking a task (one at a time,
// mirroring the real app's single global currentTaskId). '\0' when idle.
// Persisted across app close/relaunch so a long-running session survives the
// watchapp being closed for a minute.
static char s_tracking_task_id[MAX_ID_LEN] = "";
static time_t s_tracking_start_epoch = 0;
static AppTimer *s_tracking_tick_timer = NULL;
#define TRACKING_TICK_INTERVAL_MS 1000

#ifndef PBL_PLATFORM_APLITE
// "Task ran over its estimate" banner - a red strip across the top of the list,
// shown with a double vibe the moment the tracked task's effective time (synced
// time_spent_ms + this session's elapsed) reaches time_estimate_ms, if
// s_overtime_notify_enabled. Auto-dismisses after OVERTIME_BANNER_MS or on the
// next Select. aplite-excluded (an extra TextLayer overflows its RAM) - tracking
// still works there, just without the banner.
static TextLayer *s_overtime_banner_layer = NULL;
static AppTimer *s_overtime_banner_timer = NULL;
// Latched once the banner fires for the current session so the per-second tick
// doesn't re-fire it. Reset by start/stop_tracking; self-re-arms if effective
// time drops back under the estimate. Primed true in init() when a resumed
// session is already over, so reopening mid-overrun doesn't nag.
static bool s_overtime_notified = false;
// Epoch the banner last fired for the current crossing. Only used when the
// "Repeat every 5 minutes" sub-option is on: maybe_notify_overtime re-fires
// once OVERTIME_REPEAT_INTERVAL_S passes and the task is still over.
static time_t s_overtime_last_notify_epoch = 0;
static char s_overtime_banner_text[MAX_TITLE_LEN + 24] = "";
#define OVERTIME_BANNER_MS 6000
#ifdef PBL_PLATFORM_EMERY
#define OVERTIME_BANNER_HEIGHT 60
#else
#define OVERTIME_BANNER_HEIGHT 52
#endif
#define OVERTIME_REPEAT_INTERVAL_S (5 * 60)

// Pinned "TRACKING" section - the tracked task (local, or a remote device's
// via remote_in_pinned_section) shows in its own section below the
// Resync/Habits/Add Task rows and is hidden from its project group. Always on
// (non-aplite). Separate from s_tracking_task_id: it lingers through a 10s
// grace period after tracking stops (s_unpin_timer) so the row slides back
// smoothly. Not persisted - only an active session re-pins on relaunch.
static char s_pinned_task_id[MAX_ID_LEN] = "";
static AppTimer *s_unpin_timer = NULL;
#define UNPIN_GRACE_MS 10000
#ifdef PBL_PLATFORM_EMERY
#define PINNED_HEADER_HEIGHT 28
#else
#define PINNED_HEADER_HEIGHT 22
#endif
#endif

// Like s_tracking_task_id but for a StopWatch/countdown habit - its own slot,
// since tracking a task and a habit stopwatch at once is valid. Its tick timer
// only runs while the habits window is loaded (s_habits_menu_layer is rebuilt
// per visit). aplite-excluded (#ifndef): the tracking machinery pushed aplite
// 820 bytes over .bss; a StopWatch habit still shows value/goal read-only there.
#ifndef PBL_PLATFORM_APLITE
static char s_tracking_habit_id[MAX_HABIT_ID_LEN] = "";
static time_t s_tracking_habit_start_epoch = 0;
static AppTimer *s_habit_tracking_tick_timer = NULL;
// Select pauses/resumes an is_countdown round (long-select still cancels it);
// meaningless for a StopWatch. While paused the tick timer is stopped and
// s_habit_countdown_frozen_elapsed_ms holds total elapsed; while running it
// holds elapsed before the current segment - see countdown_elapsed_ms().
static bool s_habit_countdown_paused = false;
static int s_habit_countdown_frozen_elapsed_ms = 0;
#endif

static Task s_tasks[MAX_TASKS];
static int s_task_count = 0;      // tasks currently shown (committed)
static int s_incoming_total = 0;  // total announced by the current sync batch
static Task s_incoming[MAX_TASKS];
// -1 is a "no real status yet" sentinel, never sent or matched over the wire.
// init() then makes the real STATUS_SYNCING transition explicit through
// set_status_code() (the chokepoint every status change goes through) rather
// than hardcoding it here, which would skip set_status_code's transition logic.
static int s_status_code = -1;
#define MAX_STATUS_MSG_LEN 64
static char s_status_msg[MAX_STATUS_MSG_LEN] = "";
// Phone-side settings mirrored via optional MSG_SYNC_STATUS fields - default
// true so a not-yet-synced watch behaves as before until a real sync says
// otherwise. Unconditional statics (trivial size); s_add_task_enabled is inert
// on aplite anyway (PBL_IF_MICROPHONE_ELSE keeps the row absent).
static bool s_habits_enabled = true;
static bool s_add_task_enabled = true;
// "Projects" browser row (config.enableProjects, default on). Compiled out on
// aplite (RAM) and emery (PROJECTS_BROWSER, below); the flag stays a plain
// unconditional static (trivial) and is simply never read there.
static bool s_projects_enabled = true;

// The Projects browser is built on every platform except aplite (too little
// RAM, like several other features here). PROJECTS_ROW_ACTIVE() is a
// compile-time false there, so the row, its window and its buffers all drop.
#ifdef PBL_PLATFORM_APLITE
#define PROJECTS_BROWSER 0
#else
#define PROJECTS_BROWSER 1
#endif

// The project-list persist cache (save/load_browse_projects). Every platform
// that has the browser gets it - emery included, now that its Task cap (40)
// leaves room under the virtual-size ceiling.
#if PROJECTS_BROWSER
#define PROJECTS_CACHE 1
#else
#define PROJECTS_CACHE 0
#endif

#if PROJECTS_BROWSER
// ---- Projects browser ----
// A pinned section-0 row -> a green list of every project (styled like the
// today view's project group headers) -> that project's tasks (its regular
// list, then its backlog under a divider). Select toggles a task done,
// long-Select starts/stops tracking it - both round-trip through the same
// sends the today list uses, so a task tracked here also lands in the today
// page's pinned "TRACKING" section (index.js force-includes the tracked task
// in every list send).
//
// One window with a level flag (0 = project list, 1 = one project's tasks);
// Back at level 1 returns to the list. The two row buffers are malloc'd while
// the window is open and freed on unload, like the notes text - not static:
// emery's virtual-size budget has no room for a static Task[] here.
//
// The project LIST is persisted (PROJECTS_CACHE) so it renders instantly on
// open and stays viewable while the phone is unreachable; the on-open fetch
// refreshes it. Per-project task lists are always fetched.
#define MAX_BROWSE_PROJECTS 60
#define MAX_BROWSE_TASKS MAX_TASKS
#if PROJECTS_CACHE
static const uint32_t PERSIST_KEY_BROWSE_PROJECTS = 140; // + 1 for the count
#endif
typedef struct {
  char id[MAX_PROJECT_ID_LEN];
  char title[MAX_TITLE_LEN];
  int color; // packed GColor8 byte for the theme-colour swatch, 0 = none
} BrowseProject;
static BrowseProject *s_browse_projects = NULL;
static int s_browse_project_count = 0;      // committed (drawn) count
static int s_browse_project_incoming = 0;   // announced by the current LIST_START
static bool s_browse_projects_loading = false;
// One project's tasks. Full Task structs (not a slim variant) so draw_task_row
// and the tracking/toggle helpers take them as-is. Regular-list rows fill
// indices [0, s_browse_backlog_start); backlog rows fill the rest.
static Task *s_browse_tasks = NULL;
static int s_browse_task_count = 0;
static int s_browse_task_incoming = 0;
static int s_browse_backlog_start = 0;
static bool s_browse_tasks_loading = false;
// Which project's tasks are shown / awaited - a TASKS_* message for any other
// id is stale and ignored.
static char s_browse_project_id[MAX_PROJECT_ID_LEN] = "";
static int s_browse_level = 0;              // 0 = project list, 1 = one project's tasks
static Window *s_browse_window = NULL;
static MenuLayer *s_browse_menu = NULL;
static StatusBarLayer *s_browse_status_bar = NULL;
static TextLayer *s_browse_empty = NULL;
#endif

#ifndef PBL_PLATFORM_APLITE
// "Notify when a task runs over its estimate" (config.overtimeNotify). Opt-in,
// false until the first sync. aplite-excluded with the banner it drives.
static bool s_overtime_notify_enabled = false;
// "Repeat every 5 minutes" (config.overtimeRepeat), a modifier on the above:
// re-fire the banner every OVERTIME_REPEAT_INTERVAL_S while the task stays over.
static bool s_overtime_repeat_enabled = false;
// Live tracking presence (config.liveTracking, MSG_PRESENCE_*). Opt-in,
// aplite-excluded. Shows what ANOTHER device is tracking as a "LIVE" row in
// section 0 plus a detail window; Select in that window asks the phone to stop
// it. State: 0 none, 1 tracking, 2 paused (producer idle), 3 was-tracking
// (producer went silent), 4 stopped (brief linger before it clears, mirrors
// the desktop chip). The phone holds the session id and does the stop.
static int s_presence_state = 0;
static char s_presence_task[MAX_TITLE_LEN] = "";
static char s_presence_device[24] = "";
static bool s_presence_can_stop = false;
static bool s_presence_stopping = false;   // Stop sent, awaiting the phone's clear
static time_t s_presence_elapsed_base = 0; // time(NULL) - elapsed_s, stamped at receipt
static Window *s_live_window = NULL;
static TextLayer *s_live_state_layer = NULL;
static TextLayer *s_live_task_layer = NULL;
static TextLayer *s_live_elapsed_layer = NULL;
static TextLayer *s_live_hint_layer = NULL;
static StatusBarLayer *s_live_status_bar = NULL;
static AppTimer *s_live_tick_timer = NULL;

// A remote presence session shows in the pinned "TRACKING" section (the same
// slot local tracking uses) whenever nothing is tracked locally - keeps
// "something is being tracked" looking the same everywhere. The dark-blue
// section-0 LIVE row only remains for the rare overlap: a remote paused/stopped
// state arriving while this watch is itself tracking (see LIVE_ROW_ACTIVE).
static bool remote_in_pinned_section(void) {
  return s_presence_state != 0 && s_tracking_task_id[0] == '\0';
}
#endif
#if defined(PBL_TOUCH)
// "Touch navigation" (config.touchNav), off by default (see the touch block's
// HARDWARE STATE comment). PBL_TOUCH-only. See apply_touch_nav().
static bool s_touch_nav_enabled = false;
#endif
// Backlight override: 0 (system default) until the first sync, so an unconfigured
// watch never touches the backlight API. Negative (BACKLIGHT_MODE_ALWAYS_ON)
// forces it on while the app is open; positive is a relight-and-hold duration in
// seconds - see backlight_touch()/apply_backlight_mode(). aplite-excluded
// (aplite had 10 bytes of free RAM before this feature); backlight_touch()
// becomes a no-op macro there so its call sites need no #ifdef.
#ifndef PBL_PLATFORM_APLITE
#define BACKLIGHT_MODE_ALWAYS_ON -1
static int32_t s_backlight_mode = 0;
static AppTimer *s_backlight_timer = NULL;
#endif

// "Sync automatically on a timer" (config.autoSyncIntervalMin). schedule_next_wakeup()
// uses it to relaunch the app via wakeup_schedule() so a sync can run while the
// app is closed - PebbleKit JS only runs while this app is open, so a phone-side
// setInterval alone never fires once the watch moves on. aplite-excluded
// (overflowed it by 332 bytes) - aplite keeps the old open-only behavior.
#ifndef PBL_PLATFORM_APLITE
static int32_t s_auto_sync_interval_min = 0;
// Set once in init() from launch_reason() - whether this session exists because
// a wakeup fired. Gates the auto-exit in the MSG_SYNC_STATUS handler: a
// wakeup-launched session syncs quietly and returns; a manual one stays open.
static bool s_is_wakeup_launch = false;
// Guards schedule_next_wakeup() from running on every status push - only on the
// first confirmed interval this launch, and again if it changes.
static bool s_wakeup_rescheduled_this_launch = false;
// Guards window_stack_pop_all() from firing twice if multiple terminal statuses
// arrive in one wakeup-launched session (e.g. a retried sync after an error).
static bool s_wakeup_exit_triggered = false;
#endif

static TaskGroup s_groups[MAX_TASKS]; // worst case: every task its own group
static int s_group_count = 0;

// Marquee-scrolls the selected task row's title when it's too wide to fit
// (MenuLayer has no scrolling-text cell). Only the selected row scrolls.
#define SCROLL_GAP_PX 24

// --- Platform-scaled UI metrics ---------------------------------------------
// emery renders third-party apps at the SDK's "Large" content size, so its
// MenuLayer cells are ~61px vs 44px elsewhere. Everything that positions text
// in a cell needs an emery variant or the 44px offsets leave the title jammed
// up top and the subtitle stranded at the bottom. Non-emery values are
// unchanged from before this block.
//
// TITLE/SUBTITLE_FONT_KEY: a row's title and its due/time/"Done" subtitle.
// HEADING_FONT_KEY: the bold Resync/Habits/Add Task/Finish Day/project rows.
// TRACKING_FONT_KEY: the "TRACKING" pinned-section header strip.
// CHROME_FONT_KEY: secondary text (Resync status, version subtitle, "No tasks").
// *_STRIP_H: height reserved at the bottom of a cell for its subtitle.
// HEADING_TITLE_H: title-box height for the taller HEADING_FONT_KEY rows.
#ifdef PBL_PLATFORM_EMERY
#define SCROLL_INTERVAL_MS 100
#define SCROLL_STEP_PX 2
#define TITLE_FONT_KEY FONT_KEY_GOTHIC_24_BOLD
#define SUBTITLE_FONT_KEY FONT_KEY_GOTHIC_18
#define HEADING_FONT_KEY FONT_KEY_GOTHIC_28_BOLD
#define TRACKING_FONT_KEY FONT_KEY_GOTHIC_24_BOLD
#define CHROME_FONT_KEY FONT_KEY_GOTHIC_18
#define EMPTY_MSG_FONT_KEY FONT_KEY_GOTHIC_24
#define TITLE_BOX_X 10
#define TITLE_BOX_Y 3
#define SUBTITLE_STRIP_H 26
#define CHROME_STRIP_H 22
#define HEADING_TITLE_H 34
#define GROUP_HEADER_HEIGHT 48
#else
#define SCROLL_INTERVAL_MS 300
#define SCROLL_STEP_PX 6
#define TITLE_FONT_KEY FONT_KEY_GOTHIC_18_BOLD
#define SUBTITLE_FONT_KEY FONT_KEY_GOTHIC_18
#define HEADING_FONT_KEY FONT_KEY_GOTHIC_24_BOLD
#define TRACKING_FONT_KEY FONT_KEY_GOTHIC_18_BOLD
#define CHROME_FONT_KEY FONT_KEY_GOTHIC_14
#define EMPTY_MSG_FONT_KEY FONT_KEY_GOTHIC_18
#define TITLE_BOX_X 6
#define TITLE_BOX_Y 2
#define SUBTITLE_STRIP_H 22
#define CHROME_STRIP_H 18
#define HEADING_TITLE_H 30
#define GROUP_HEADER_HEIGHT 40
#endif

// Lays out a title line (height title_h) + a bottom subtitle strip (strip_h)
// in a cell_h-tall cell. Pure macros, no locals - aplite has no stack headroom
// for extra int16 slots in the draw paths, and non-emery expands to the exact
// fixed offsets it replaced. Non-emery: title at TITLE_BOX_Y, subtitle flush
// bottom. Emery: title+subtitle centred as one block in the taller cell.
// HEADING_TITLE_Y is the single-line form for the bold nav rows.
#ifdef PBL_PLATFORM_EMERY
#define ROW_TITLE_TOP_Y(cell_h, title_h, strip_h) \
  (((cell_h) - (title_h) - (strip_h)) / 2 < TITLE_BOX_Y \
   ? TITLE_BOX_Y : ((cell_h) - (title_h) - (strip_h)) / 2)
#define ROW_SUBTITLE_TOP_Y(cell_h, title_h, strip_h) \
  (ROW_TITLE_TOP_Y(cell_h, title_h, strip_h) + (title_h))
#define HEADING_TITLE_Y(cell_h) (((cell_h) - HEADING_TITLE_H) / 2)
#else
#define ROW_TITLE_TOP_Y(cell_h, title_h, strip_h) (TITLE_BOX_Y)
#define ROW_SUBTITLE_TOP_Y(cell_h, title_h, strip_h) ((cell_h) - (strip_h))
#define HEADING_TITLE_Y(cell_h) (TITLE_BOX_Y)
#endif

static AppTimer *s_scroll_timer = NULL;
static int s_scroll_offset_px = 0;

// Cycles "Syncing" -> "Syncing..." on the empty screen during the first sync
// (no cached list yet) so it doesn't sit on static text through the op-log replay.
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
#ifndef PBL_PLATFORM_APLITE
    strncpy(s_groups[s_group_count].project_id, s_tasks[i].project_id, MAX_PROJECT_ID_LEN - 1);
    s_groups[s_group_count].project_id[MAX_PROJECT_ID_LEN - 1] = '\0';
#endif
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

// Its own key pair, independent of save_tasks(), so a tracked session survives
// a resync that replaces s_tasks wholesale.
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

// Mirrors save_tracking()/load_tracking() for a tracked StopWatch/countdown
// habit. The pause fields only matter for an is_countdown session but persist
// unconditionally (a StopWatch just always saves paused: false).
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
static Task *find_task_by_id(const char *id);
#if PROJECTS_BROWSER
static void push_browse_window(const char *jump_to_project);
static void browse_update_empty(void);
static void request_project_list(void);
static void request_project_tasks(const char *project_id);
#endif
#if PROJECTS_CACHE
static void save_browse_projects(void);
#endif
#ifndef PBL_PLATFORM_APLITE
static void hide_overtime_banner(void);
static void maybe_notify_overtime(void);
static bool has_pinned_row(void);
static int pinned_task_index(void);
static void refresh_pinned_section(void);
static void push_live_window(void);
static void live_window_refresh(void);
static void stop_live_tick(void);
static const char *presence_state_phrase(void);
static void send_presence_stop(void);
#endif
#ifndef PBL_PLATFORM_APLITE
static void backlight_touch(void);
#else
#define backlight_touch() ((void)0)
#endif
#ifndef PBL_PLATFORM_APLITE
static void show_notes_overlay(Task *task);
static void show_project_notes_overlay(TaskGroup *group);
static void hide_notes_overlay(void);
static void push_notes_window(void);
static void pending_toggle_timer_callback(void *data);
static void pending_reschedule_timer_callback(void *data);
static void cancel_pending_reschedule(void);
static void begin_pending_reschedule(RescheduleKind kind);
static void send_task_reschedule(const char *task_id, bool tomorrow);
static TaskGroup *resolve_project_row_at(MenuIndex index);
#endif
#ifndef PBL_PLATFORM_APLITE
static void start_add_task_dictation(void);
static void start_note_append_dictation(void);
#endif

#if defined(PBL_TOUCH)
// Applies s_touch_nav_enabled: opts into the system touch-nav bridge and
// arms/disarms the raw long-press handler below. Called from init() and when a
// sync reports the setting changed.
static void apply_touch_nav(void);
#endif

// ---------- menu layer callbacks ----------

// Section 0 is "Resync", plus "Habits" and/or "Add Task" when enabled - a
// dynamic 1-3 rows. section0_row_count()/section0_row_kind() are the single
// source of truth; every menu callback defers to them. Add Task stays gated by
// PBL_IF_MICROPHONE_ELSE regardless of the setting.
// With tasks: sections 1..s_group_count are the project groups, then one final
// section (group_idx == s_group_count) with a single "Finish Day" row - always
// last, long-select archives every done task, plain Select is a no-op.
// Empty list: no further sections. For NOT_PAIRED/ERROR/initial-syncing,
// section 0 doubles as the phantom retry row; for STATUS_OK with nothing due
// (ACTIONABLE_EMPTY_ACTIVE()), section 0 shows its normal rows with "No tasks
// for today." as the header.
typedef enum {
  SECTION0_ROW_RESYNC,
  SECTION0_ROW_HABITS,
  SECTION0_ROW_PROJECTS, // projects browser, between Habits and Add Task (non-aplite)
  SECTION0_ROW_ADD_TASK,
  SECTION0_ROW_LIVE, // live tracking presence, row 0 when active (non-aplite)
} Section0RowKind;

// Whether the STATUS_OK/zero-tasks empty state shows section 0's normal
// interactive rows (with a "No tasks for today." header) instead of the hidden
// phantom retry row. aplite keeps the old behavior - the extra logic pushed it
// 176 bytes past its APP region. A compile-time macro, not a runtime check, so
// the dead branches are eliminated on aplite.
#ifdef PBL_PLATFORM_APLITE
#define ACTIONABLE_EMPTY_ACTIVE() false
#else
#define ACTIONABLE_EMPTY_ACTIVE() (s_status_code == STATUS_OK)
#endif

// Whether the "Projects" row sits in section 0. Compile-time false where the
// browser isn't built (aplite, emery - see PROJECTS_BROWSER).
#if PROJECTS_BROWSER
#define PROJECTS_ROW_ACTIVE() (s_projects_enabled)
#else
#define PROJECTS_ROW_ACTIVE() false
#endif

// Whether the "LIVE" presence row sits at the top of section 0. Compile-time
// false on aplite (the whole feature is excluded).
#ifdef PBL_PLATFORM_APLITE
#define LIVE_ROW_ACTIVE() false
#else
// The dark-blue section-0 row - only when a remote session isn't riding the
// pinned "TRACKING" section instead (see remote_in_pinned_section).
#define LIVE_ROW_ACTIVE() (s_presence_state != 0 && !remote_in_pinned_section())
#endif

static int section0_row_count(void) {
  int count = 1; // Resync always present.
  if (LIVE_ROW_ACTIVE()) {
    count++;
  }
  if (s_habits_enabled) {
    count++;
  }
  if (PROJECTS_ROW_ACTIVE()) {
    count++;
  }
  if (PBL_IF_MICROPHONE_ELSE(s_add_task_enabled, false)) {
    count++;
  }
  return count;
}

// Maps a section-0 row index to its action. Resync is row 0; Habits then Add
// Task fill in after it, matching section0_row_count()'s order.
static Section0RowKind section0_row_kind(int row) {
  int next = 0;
  if (LIVE_ROW_ACTIVE()) {
    if (row == 0) {
      return SECTION0_ROW_LIVE;
    }
    next = 1;
  }
  if (row == next) {
    return SECTION0_ROW_RESYNC;
  }
  next++;
  if (s_habits_enabled) {
    if (row == next) {
      return SECTION0_ROW_HABITS;
    }
    next++;
  }
  if (PROJECTS_ROW_ACTIVE()) {
    if (row == next) {
      return SECTION0_ROW_PROJECTS;
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

#ifndef PBL_PLATFORM_APLITE
// Index into s_tasks of the pinned "TRACKING" task, or -1 if s_pinned_task_id
// is unset or gone from the list. Does NOT check the enable flag - see
// has_pinned_row().
static int pinned_task_index(void) {
  if (s_pinned_task_id[0] == '\0') {
    return -1;
  }
  for (int i = 0; i < s_task_count; i++) {
    if (strncmp(s_tasks[i].id, s_pinned_task_id, MAX_ID_LEN) == 0) {
      return i;
    }
  }
  return -1;
}

// Whether the pinned "TRACKING" section is currently shown: the phone setting
// is on AND there's a real locally-tracked task to put in it, OR a remote
// presence session is riding this section (remote_in_pinned_section).
static bool has_pinned_row(void) {
  return pinned_task_index() >= 0 || remote_in_pinned_section();
}

// Section index of project group 0 - 1 normally, 2 when the pinned section sits
// between section 0 and the groups. Menu callbacks derive a group index as
// (section_index - group_section_base()).
static int group_section_base(void) {
  return 1 + (has_pinned_row() ? 1 : 0);
}
#endif

// group_section_base() as a plain constant on aplite (no pinned section), the
// runtime value elsewhere - lets callbacks skip an #ifdef at every site.
#ifdef PBL_PLATFORM_APLITE
#define GROUP_SECTION_BASE 1
#else
#define GROUP_SECTION_BASE group_section_base()
#endif

#ifndef PBL_PLATFORM_APLITE

// How many of group g's tasks are drawn - all, minus the pinned task if it's in
// this group. A group with 0 visible tasks collapses entirely.
static int group_visible_task_count(int g) {
  int count = s_groups[g].count;
  if (has_pinned_row()) {
    int pi = pinned_task_index();
    if (pi >= s_groups[g].start && pi < s_groups[g].start + s_groups[g].count) {
      count--;
    }
  }
  return count;
}
#endif

static uint16_t menu_get_num_sections(MenuLayer *menu_layer, void *context) {
  if (s_task_count == 0) {
    return 1;
  }
#ifndef PBL_PLATFORM_APLITE
  return (uint16_t)(1 + (has_pinned_row() ? 1 : 0) + s_group_count + 1);
#else
  return (uint16_t)(1 + s_group_count + 1);
#endif
}

static uint16_t menu_get_num_rows(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  if (s_task_count == 0) {
    // Actionable empty state (STATUS_OK, nothing due): the menu stays visible
    // with its normal section-0 rows. Every other empty reason hides the menu
    // but still needs one reportable row so SELECT dispatches ("Select to
    // retry"); that row is never drawn since the layer is hidden.
    return ACTIONABLE_EMPTY_ACTIVE() ? (uint16_t)section0_row_count() : 1;
  }
  if (section_index == 0) {
    return (uint16_t)section0_row_count();
  }
#ifndef PBL_PLATFORM_APLITE
  // The pinned "TRACKING" section at index 1 - one row, the tracked task.
  if (has_pinned_row() && section_index == 1) {
    return 1;
  }
#endif
  int group_idx = (int)section_index - GROUP_SECTION_BASE;
  if (group_idx == s_group_count) {
    return 1; // Finish Day row
  }
  if (group_idx > s_group_count) {
    return 0;
  }
#ifndef PBL_PLATFORM_APLITE
  // A named group gets one extra row up front - a selectable "project row"
  // standing in for the plain header (double-click Select shows its notes).
  // aplite keeps the plain non-interactive header.
  int visible = group_visible_task_count(group_idx);
  if (visible == 0) {
    return 0; // whole group collapsed - its only task is pinned at the top
  }
  if (s_groups[group_idx].name[0] != '\0') {
    return (uint16_t)(visible + 1);
  }
  return (uint16_t)visible;
#else
  return (uint16_t)s_groups[group_idx].count;
#endif
}

static int16_t menu_get_header_height(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  if (s_task_count == 0) {
    // Actionable empty state only - the header holds "No tasks for today."
    return (section_index == 0 && ACTIONABLE_EMPTY_ACTIVE()) ? GROUP_HEADER_HEIGHT : 0;
  }
  if (section_index == 0) {
    return 0;
  }
#ifndef PBL_PLATFORM_APLITE
  // The pinned "TRACKING" section gets a short labelled header strip.
  if (has_pinned_row() && section_index == 1) {
    return PINNED_HEADER_HEIGHT;
  }
#endif
  int group_idx = (int)section_index - GROUP_SECTION_BASE;
  // Empty group name = grouping off (one '' group) - no header, flat list.
  if (group_idx >= s_group_count || s_groups[group_idx].name[0] == '\0') {
    return 0;
  }
#ifndef PBL_PLATFORM_APLITE
  // A group whose only task is pinned at the top collapses whole - no header.
  if (group_visible_task_count(group_idx) == 0) {
    return 0;
  }
  // Non-aplite: the group name lives in a selectable project row instead
  // (menu_get_num_rows/menu_draw_row), so no separate header.
  return 0;
#else
  return GROUP_HEADER_HEIGHT;
#endif
}

#ifndef PBL_PLATFORM_APLITE
// A project's theme-colour swatch: a 16px square filled with the phone-packed
// GColor8 byte, centred vertically, drawn at x. Returns the x for following
// text - unchanged (nothing drawn) when color is 0. Shared by the grouped
// today view's project rows and the Projects browser's list.
static int16_t draw_project_swatch(GContext *ctx, int16_t x, int16_t cell_h, uint8_t color) {
  if (color == 0) {
    return x;
  }
  graphics_context_set_fill_color(ctx, (GColor8){ .argb = color });
  graphics_fill_rect(ctx, GRect(x, (cell_h - 16) / 2, 16, 16), 0, GCornerNone);
  return x + 22;
}
#endif

static void menu_draw_header(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *context) {
  if (s_task_count == 0) {
    if (section_index == 0 && ACTIONABLE_EMPTY_ACTIVE()) {
      // "No tasks for today." above the still-reachable section-0 rows.
      GRect bounds = layer_get_bounds(cell_layer);
      graphics_context_set_fill_color(ctx, GColorWhite);
      graphics_fill_rect(ctx, bounds, 0, GCornerNone);
      graphics_context_set_text_color(ctx, GColorBlack);
      graphics_draw_text(ctx, "No tasks for today.", fonts_get_system_font(EMPTY_MSG_FONT_KEY),
                          bounds, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
    }
    return;
  }
  if (section_index == 0) {
    return;
  }
#ifndef PBL_PLATFORM_APLITE
  // The pinned "TRACKING" header - a thin green strip, quieter than the bold
  // project headers (TRACKING_FONT_KEY is one step down from HEADING_FONT_KEY).
  if (has_pinned_row() && section_index == 1) {
    GRect hb = layer_get_bounds(cell_layer);
    graphics_context_set_fill_color(ctx, GColorGreen);
    graphics_fill_rect(ctx, hb, 0, GCornerNone);
    graphics_context_set_text_color(ctx, GColorBlack);
    graphics_draw_text(ctx, "TRACKING", fonts_get_system_font(TRACKING_FONT_KEY),
                        GRect(TITLE_BOX_X, 0, hb.size.w - TITLE_BOX_X * 2, hb.size.h),
                        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    graphics_context_set_stroke_color(ctx, GColorBlack);
    graphics_draw_line(ctx, GPoint(0, hb.size.h - 1), GPoint(hb.size.w, hb.size.h - 1));
    return;
  }
#endif
  int group_idx = (int)section_index - GROUP_SECTION_BASE;
  if (group_idx >= s_group_count || s_groups[group_idx].name[0] == '\0') {
    return;
  }
  const char *name = s_groups[group_idx].name;
  GRect bounds = layer_get_bounds(cell_layer);
  GFont bold_font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  GRect text_rect = GRect(6, 2, bounds.size.w - 12, bounds.size.h - 4);

  // Fill first - a MenuLayer header has no built-in background, so the text and
  // lines below would otherwise draw onto stale framebuffer content.
  graphics_context_set_fill_color(ctx, GColorGreen);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, name, bold_font, text_rect,
                      GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  // Full-width divider separating this group from its tasks.
  int16_t divider_y = bounds.size.h - 2;
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_draw_line(ctx, GPoint(0, divider_y), GPoint(bounds.size.w, divider_y));
}

// Looks up a task by id - a resync can rebuild s_tasks wholesale, so a Task*
// captured earlier (e.g. at start_tracking()) can't be trusted.
static Task *find_task_by_id(const char *id) {
  for (int i = 0; i < s_task_count; i++) {
    if (strncmp(s_tasks[i].id, id, MAX_ID_LEN) == 0) {
      return &s_tasks[i];
    }
  }
  return NULL;
}

#ifndef PBL_PLATFORM_APLITE
// find_task_by_id for the habits list - only used by the habit-tracking
// functions, themselves aplite-excluded.
static Habit *find_habit_by_id(const char *id) {
  for (int i = 0; i < s_habit_count; i++) {
    if (strncmp(s_habits[i].id, id, MAX_HABIT_ID_LEN) == 0) {
      return &s_habits[i];
    }
  }
  return NULL;
}
#endif

// Resolves a MenuIndex to the Task it points at, or NULL if it isn't on a task
// row (Resync, a project row, Finish Day, or out of range - an empty list has
// s_group_count 0 and falls through the same group_idx bounds check).
static Task *resolve_task_at(MenuIndex index) {
  if (index.section == 0) {
    return NULL;
  }
#ifndef PBL_PLATFORM_APLITE
  // The pinned "TRACKING" section's single row IS the tracked task - Select
  // (toggle), long-Select (stop tracking) and double-click (notes) all fall
  // through to the same handlers a normal task row uses.
  if (has_pinned_row() && index.section == 1) {
    int pi = pinned_task_index();
    return pi >= 0 ? &s_tasks[pi] : NULL;
  }
#endif
  int group_idx = (int)index.section - GROUP_SECTION_BASE;
  if (group_idx >= s_group_count) {
    return NULL;
  }
  int row = (int)index.row;
#ifndef PBL_PLATFORM_APLITE
  // Row 0 of a named group is the selectable project row, not a task; every
  // other row shifts down by one.
  if (s_groups[group_idx].name[0] != '\0') {
    if (row == 0) {
      return NULL;
    }
    row -= 1;
  }
  // Walk the group's tasks skipping the pinned one (drawn in the pinned
  // section), so visible row N is the Nth non-pinned task.
  int pinned_idx = has_pinned_row() ? pinned_task_index() : -1;
  int start = s_groups[group_idx].start;
  int end = start + s_groups[group_idx].count;
  int seen = 0;
  for (int i = start; i < end && i < s_task_count; i++) {
    if (i == pinned_idx) {
      continue;
    }
    if (seen == row) {
      return &s_tasks[i];
    }
    seen++;
  }
  return NULL;
#else
  int task_idx = s_groups[group_idx].start + row;
  if (task_idx < 0 || task_idx >= s_task_count) {
    return NULL;
  }
  return &s_tasks[task_idx];
#endif
}

#ifndef PBL_PLATFORM_APLITE
// Resolves a MenuIndex to a TaskGroup if it's on that group's selectable
// project row (row 0 of a named group), else NULL.
static TaskGroup *resolve_project_row_at(MenuIndex index) {
  if (index.section == 0) {
    return NULL;
  }
  if (has_pinned_row() && index.section == 1) {
    return NULL;
  }
  int group_idx = (int)index.section - GROUP_SECTION_BASE;
  if (group_idx >= s_group_count) {
    return NULL;
  }
  if (s_groups[group_idx].name[0] == '\0' || (int)index.row != 0) {
    return NULL;
  }
  // A fully-collapsed group (its only task pinned) shows no project row.
  if (group_visible_task_count(group_idx) == 0) {
    return NULL;
  }
  return &s_groups[group_idx];
}
#endif

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

// Formats due_min (minutes since local midnight) as "@ 9:41 AM" / "@ 21:41",
// respecting the watch's 12h/24h setting. The phone already sent local time.
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

// Formats tracked time as "1h 23m" (seconds dropped), "5m 09s", or "42s".
// is_tracking prefixes "> " so a live number reads as running vs a static total.
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

#ifndef PBL_PLATFORM_APLITE
// "Tracking on Desktop" / "Paused on Desktop" / "Was tracking on Desktop", or a
// device-less form when the presence payload could not be decoded phone-side.
static const char *presence_state_phrase(void) {
  static char buf[48];
  const char *verb = s_presence_state == 2 ? "Paused"
                     : s_presence_state == 3 ? "Was tracking"
                     : s_presence_state == 4 ? "Stopped"
                     : "Tracking";
  if (s_presence_device[0] != '\0') {
    snprintf(buf, sizeof(buf), "%s on %s", verb, s_presence_device);
  } else {
    snprintf(buf, sizeof(buf), "%s on another device", verb);
  }
  return buf;
}
#endif

static void scroll_timer_callback(void *data) {
  s_scroll_offset_px += SCROLL_STEP_PX;
  layer_mark_dirty(menu_layer_get_layer(s_menu_layer));
  s_scroll_timer = app_timer_register(SCROLL_INTERVAL_MS, scroll_timer_callback, NULL);
}

// Starts/stops the marquee timer to match whether the selected row needs it,
// optionally resetting the scroll position. Called on selection or list change.
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
  // Hands control back to automatic backlight behavior, not "force off". With
  // no button press at this instant, auto control has nothing to keep it lit,
  // which is what makes the custom timeout duration real.
  light_enable(false);
}

// Called on every button interaction (select/long-select/scroll) - NOT on a
// settings change (that's apply_backlight_mode()). Mode 0 is a no-op: the app
// never touches the backlight API unless the phone opts into another mode.
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

// Reacts to s_backlight_mode changing (a settings save via MSG_SYNC_STATUS),
// not to user interaction. Leaving always-on needs an explicit light_enable(false)
// here - backlight_touch() only turns it on, and the timeout timer never runs
// in always-on mode.
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
// (Re)arms the single wakeup this app uses, relative to now - once per launch
// when s_auto_sync_interval_min is first confirmed, and again when it changes.
// wakeup_cancel_all() is safe: this is the only wakeup the app schedules.
// Re-arming on every confirmed sync keeps the next wakeup one full interval past
// the most recent activity. notify_if_missed is false - it's a quiet resync.
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

// Draws one task row - marquee/ellipsized title plus the "@ due  > spent /
// estimate" subtitle. Shared by per-group rows and the pinned "TRACKING" row.
// show_project right-aligns task->project on the subtitle line; only the pinned
// row passes true (and only shows it when grouping is on). `bounds` is a
// MenuLayer cell's own bounds (origin 0,0).
static void draw_task_row(GContext *ctx, GRect bounds, Task *task, bool is_selected, bool show_project) {
  int16_t available = bounds.size.w - TITLE_BOX_X * 2;
  int16_t natural_width = title_natural_width(task->title);
  bool needs_marquee = is_selected && natural_width > available;

  GColor bg = is_selected ? GColorBlack : GColorWhite;
  GColor fg = is_selected ? GColorWhite : GColorBlack;
  if (task->done) {
    fg = is_selected ? GColorLightGray : GColorDarkGray;
  }
  graphics_context_set_fill_color(ctx, bg);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  graphics_context_set_text_color(ctx, fg);

  GFont title_font = fonts_get_system_font(TITLE_FONT_KEY);
  GSize one_line_size = graphics_text_layout_get_content_size(
      "Ag", title_font, GRect(0, 0, 200, 100), GTextOverflowModeFill, GTextAlignmentLeft);
  int16_t title_box_h = one_line_size.h > 0 ? one_line_size.h : (bounds.size.h - TITLE_BOX_Y);
  GRect title_box = GRect(TITLE_BOX_X, ROW_TITLE_TOP_Y(bounds.size.h, title_box_h, SUBTITLE_STRIP_H),
                           bounds.size.w - TITLE_BOX_X * 2, title_box_h);

  if (needs_marquee) {
    int16_t period = natural_width + SCROLL_GAP_PX;
    int16_t x = -(s_scroll_offset_px % period);
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

  GRect subtitle_box = GRect(TITLE_BOX_X, ROW_SUBTITLE_TOP_Y(bounds.size.h, title_box_h, SUBTITLE_STRIP_H),
                              bounds.size.w - TITLE_BOX_X * 2, SUBTITLE_STRIP_H);

#ifndef PBL_PLATFORM_APLITE
  // Pending reschedule: takes over the whole subtitle line (over "Done" and the
  // due/time text) for the 3s cancel window - see begin_pending_reschedule.
  if (s_pending_reschedule_kind != RESCHEDULE_NONE &&
      strncmp(s_pending_reschedule_task_id, task->id, MAX_ID_LEN) == 0) {
    const char *pending_msg = s_pending_reschedule_kind == RESCHEDULE_TOMORROW
                                  ? "Moving to tomorrow..."
                                  : "Un-Scheduling...";
    graphics_draw_text(ctx, pending_msg, fonts_get_system_font(SUBTITLE_FONT_KEY), subtitle_box,
                        GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    return;
  }
#endif

  if (task->done) {
    graphics_draw_text(ctx, "Done", fonts_get_system_font(SUBTITLE_FONT_KEY), subtitle_box,
                        GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    return;
  }

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
      const char *separator = is_tracking_this ? "  " : " - ";
      snprintf(subtitle + existing_len, sizeof(subtitle) - existing_len, "%s%s", separator, time_text);
    } else {
      strncpy(subtitle, time_text, sizeof(subtitle) - 1);
      subtitle[sizeof(subtitle) - 1] = '\0';
    }
  }

  GRect left_box = subtitle_box;
#ifndef PBL_PLATFORM_APLITE
  // Project name for the pinned row, right-aligned - reserve up to half the
  // width and shrink the left (due/time) box to match.
  if (show_project && task->project[0] != '\0') {
    GFont pfont = fonts_get_system_font(FONT_KEY_GOTHIC_14);
    GSize psize = graphics_text_layout_get_content_size(
        task->project, pfont, subtitle_box, GTextOverflowModeTrailingEllipsis, GTextAlignmentRight);
    int16_t pw = psize.w;
    if (pw > subtitle_box.size.w / 2) {
      pw = subtitle_box.size.w / 2;
    }
    graphics_draw_text(ctx, task->project, pfont,
                        GRect(subtitle_box.origin.x + subtitle_box.size.w - pw, subtitle_box.origin.y + 2,
                              pw, subtitle_box.size.h - 2),
                        GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
    left_box.size.w -= (pw + 6);
  }
#else
  (void)show_project;
#endif

  if (subtitle[0] != '\0') {
    graphics_draw_text(ctx, subtitle, fonts_get_system_font(SUBTITLE_FONT_KEY), left_box,
                        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  }
}

static void menu_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context) {
  if (s_task_count == 0 && !ACTIONABLE_EMPTY_ACTIVE()) {
    // Menu hidden for every empty reason except the actionable one - nothing to
    // draw. In the actionable case section 0 draws below as for a full list.
    return;
  }
  if (cell_index->section == 0) {
    bool is_selected = menu_layer_get_selected_index(s_menu_layer).section == cell_index->section &&
                        menu_layer_get_selected_index(s_menu_layer).row == cell_index->row;
    GRect bounds = layer_get_bounds(cell_layer);
    Section0RowKind kind = section0_row_kind((int)cell_index->row);

#ifndef PBL_PLATFORM_APLITE
    if (kind == SECTION0_ROW_LIVE) {
      // Dark blue - a distinct "this is another device" strip (the pinned
      // local-tracking header is green, Habits cerulean). Constant background,
      // text inverts on selection - same treatment as the Resync row.
      graphics_context_set_fill_color(ctx, GColorDukeBlue);
      graphics_fill_rect(ctx, bounds, 0, GCornerNone);
      graphics_context_set_text_color(ctx, is_selected ? GColorWhite : GColorBlack);
      // Title one size bigger (GOTHIC_24_BOLD) while a timer is actively
      // running, GOTHIC_18 otherwise - the paused / stopped / was-tracking
      // states pair with the long "a recently started task" fallback, which
      // needs the room.
      bool live_tracking = s_presence_state == 1;
      const char *live_title_font = live_tracking ? FONT_KEY_GOTHIC_24_BOLD : FONT_KEY_GOTHIC_18;
      int16_t live_title_h = live_tracking ? 26 : 22;
      GRect title_box = GRect(TITLE_BOX_X, ROW_TITLE_TOP_Y(bounds.size.h, live_title_h, CHROME_STRIP_H),
                               bounds.size.w - TITLE_BOX_X * 2, live_title_h);
      graphics_draw_text(ctx, s_presence_task[0] != '\0' ? s_presence_task : "Live tracking",
                          fonts_get_system_font(live_title_font), title_box,
                          GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
      GRect subtitle_box = GRect(TITLE_BOX_X, ROW_SUBTITLE_TOP_Y(bounds.size.h, live_title_h, CHROME_STRIP_H),
                                  bounds.size.w - TITLE_BOX_X * 2, CHROME_STRIP_H);
      graphics_draw_text(ctx, presence_state_phrase(), fonts_get_system_font(CHROME_FONT_KEY), subtitle_box,
                          GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
      return;
    }
#endif

    if (kind == SECTION0_ROW_HABITS) {
      // Navigates to the habits (SimpleCounter) page. Icon matches the real
      // app's "heart_check" icon for this feature.
      graphics_context_set_fill_color(ctx, GColorVividCerulean);
      graphics_fill_rect(ctx, bounds, 0, GCornerNone);
      graphics_context_set_text_color(ctx, is_selected ? GColorWhite : GColorBlack);
      GRect title_box = GRect(TITLE_BOX_X, HEADING_TITLE_Y(bounds.size.h),
                               bounds.size.w - TITLE_BOX_X * 2 - ROW_ICON_SIZE - 8, HEADING_TITLE_H);
      graphics_draw_text(ctx, "Habits", fonts_get_system_font(HEADING_FONT_KEY), title_box,
                          GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
      GRect icon_rect = GRect(bounds.size.w - ROW_ICON_SIZE - 10, (bounds.size.h - ROW_ICON_SIZE) / 2,
                               ROW_ICON_SIZE, ROW_ICON_SIZE);
      // GCompOpSet (not the GCompOpAssign default) so the bitmap's alpha (its
      // transparent background) takes effect - a raw graphics_draw_bitmap_in_rect
      // uses the context's own compositing mode.
      graphics_context_set_compositing_mode(ctx, GCompOpSet);
      graphics_draw_bitmap_in_rect(ctx, is_selected ? s_heart_white_bitmap : s_heart_bitmap, icon_rect);
      return;
    }

#if PROJECTS_BROWSER
    if (kind == SECTION0_ROW_PROJECTS) {
      // Navigates to the projects browser. Purple - its own colour among the
      // section-0 nav rows (Habits cerulean, Add Task green). No icon (would
      // need a new asset); the full-width label carries it.
      graphics_context_set_fill_color(ctx, GColorPurple);
      graphics_fill_rect(ctx, bounds, 0, GCornerNone);
      graphics_context_set_text_color(ctx, is_selected ? GColorWhite : GColorBlack);
      GRect title_box = GRect(TITLE_BOX_X, HEADING_TITLE_Y(bounds.size.h),
                               bounds.size.w - TITLE_BOX_X * 2, HEADING_TITLE_H);
      graphics_draw_text(ctx, "Projects", fonts_get_system_font(HEADING_FONT_KEY), title_box,
                          GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
      return;
    }
#endif

#ifndef PBL_PLATFORM_APLITE
    if (kind == SECTION0_ROW_ADD_TASK) {
      // Mic platforms with the feature enabled only. Starts dictation via
      // menu_select_click.
      graphics_context_set_fill_color(ctx, GColorJaegerGreen);
      graphics_fill_rect(ctx, bounds, 0, GCornerNone);
      graphics_context_set_text_color(ctx, is_selected ? GColorWhite : GColorBlack);
      GRect title_box = GRect(TITLE_BOX_X, HEADING_TITLE_Y(bounds.size.h),
                               bounds.size.w - TITLE_BOX_X * 2 - ROW_ICON_SIZE - 8, HEADING_TITLE_H);
      graphics_draw_text(ctx, "Add Task", fonts_get_system_font(HEADING_FONT_KEY), title_box,
                          GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
      GRect icon_rect = GRect(bounds.size.w - ROW_ICON_SIZE - 10, (bounds.size.h - ROW_ICON_SIZE) / 2,
                               ROW_ICON_SIZE, ROW_ICON_SIZE);
      graphics_context_set_compositing_mode(ctx, GCompOpSet);
      graphics_draw_bitmap_in_rect(ctx, is_selected ? s_mic_white_bitmap : s_mic_bitmap, icon_rect);
      return;
    }
#endif

    // Reflects live sync status so a resync failure is visible while the
    // cached list still shows, instead of being silently swallowed.
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
        subtitle = "Not paired - open phone app";
        break;
      default:
        break;
    }
    // Background stays red regardless of selection so this row reads as a
    // standing call-to-action, not a task; the text still inverts on select.
    graphics_context_set_fill_color(ctx, GColorRed);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    graphics_context_set_text_color(ctx, is_selected ? GColorWhite : GColorBlack);
    GRect title_box = GRect(TITLE_BOX_X, ROW_TITLE_TOP_Y(bounds.size.h, HEADING_TITLE_H, CHROME_STRIP_H),
                             bounds.size.w - TITLE_BOX_X * 2 - ROW_ICON_SIZE - 8, HEADING_TITLE_H);
    graphics_draw_text(ctx, "Resync", fonts_get_system_font(HEADING_FONT_KEY), title_box,
                        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    // Full row width - not truncating a "Failed: ..." status matters more than
    // dodging the icon, which sits up in the title band.
    GRect subtitle_box = GRect(TITLE_BOX_X, ROW_SUBTITLE_TOP_Y(bounds.size.h, HEADING_TITLE_H, CHROME_STRIP_H),
                                bounds.size.w - TITLE_BOX_X * 2, CHROME_STRIP_H);
    graphics_draw_text(ctx, subtitle, fonts_get_system_font(CHROME_FONT_KEY), subtitle_box,
                        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    // The sp checkmark logo, matching the Habits row's icon.
    GRect icon_rect = GRect(bounds.size.w - ROW_ICON_SIZE - 10,
                             ROW_TITLE_TOP_Y(bounds.size.h, HEADING_TITLE_H, CHROME_STRIP_H) + 2,
                             ROW_ICON_SIZE, ROW_ICON_SIZE);
    graphics_context_set_compositing_mode(ctx, GCompOpSet);
    graphics_draw_bitmap_in_rect(ctx, is_selected ? s_check_white_bitmap : s_check_bitmap, icon_rect);
    return;
  }
  if ((int)cell_index->section - GROUP_SECTION_BASE == s_group_count) {
#ifndef PBL_PLATFORM_APLITE
    // Finish Day row, always last - long-select archives every done task, plain
    // Select is a no-op. Inverts on selection like a task row. aplite-excluded -
    // the plain version-only footer stays there instead.
    bool is_selected = menu_layer_get_selected_index(s_menu_layer).section == cell_index->section &&
                        menu_layer_get_selected_index(s_menu_layer).row == cell_index->row;
    GRect bounds = layer_get_bounds(cell_layer);
    GColor bg = is_selected ? GColorBlack : GColorWhite;
    GColor fg = is_selected ? GColorWhite : GColorBlack;
    graphics_context_set_fill_color(ctx, bg);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    graphics_context_set_text_color(ctx, fg);
    GRect title_box = GRect(TITLE_BOX_X, ROW_TITLE_TOP_Y(bounds.size.h, HEADING_TITLE_H, CHROME_STRIP_H),
                             bounds.size.w - TITLE_BOX_X * 2, HEADING_TITLE_H);
    graphics_draw_text(ctx, "Finish Day", fonts_get_system_font(HEADING_FONT_KEY), title_box,
                        GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    // Version text as this row's subtitle - the same footer slot it's lived in
    // since v0.6.5, just no longer alone.
    GRect subtitle_box = GRect(TITLE_BOX_X, ROW_SUBTITLE_TOP_Y(bounds.size.h, HEADING_TITLE_H, CHROME_STRIP_H),
                                bounds.size.w - TITLE_BOX_X * 2, CHROME_STRIP_H);
    graphics_draw_text(ctx, "v" APP_VERSION, fonts_get_system_font(CHROME_FONT_KEY), subtitle_box,
                        GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
#else
    // Plain version-only footer - no tap/long-select action on aplite.
    GRect bounds = layer_get_bounds(cell_layer);
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    graphics_context_set_text_color(ctx, GColorBlack);
    graphics_draw_text(ctx, "v" APP_VERSION, fonts_get_system_font(FONT_KEY_GOTHIC_14), bounds,
                        GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
#endif
    return;
  }
#ifndef PBL_PLATFORM_APLITE
  // The project row - the old plain header's green/divider look, now a
  // selectable row (double-click Select shows the project's notes). Text, not
  // the green fill, inverts on selection.
  TaskGroup *project_row = resolve_project_row_at(*cell_index);
  if (project_row) {
    bool is_selected = menu_layer_get_selected_index(s_menu_layer).section == cell_index->section &&
                        menu_layer_get_selected_index(s_menu_layer).row == cell_index->row;
    GRect bounds = layer_get_bounds(cell_layer);
    GFont bold_font = fonts_get_system_font(HEADING_FONT_KEY);
    int16_t text_top = HEADING_TITLE_Y(bounds.size.h);
    GColor fg = is_selected ? GColorWhite : GColorBlack;

    graphics_context_set_fill_color(ctx, GColorGreen);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
#if TODAY_PROJECT_SWATCH
    int16_t text_x = draw_project_swatch(ctx, TITLE_BOX_X, bounds.size.h,
                                          s_tasks[project_row->start].project_color);
#else
    int16_t text_x = TITLE_BOX_X;
#endif
    GRect text_rect = GRect(text_x, text_top, bounds.size.w - text_x - TITLE_BOX_X, bounds.size.h - 4);
    graphics_context_set_text_color(ctx, fg);
    graphics_draw_text(ctx, project_row->name, bold_font, text_rect,
                        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

    // Full-width divider separating this project from its tasks.
    int16_t divider_y = bounds.size.h - 2;
    graphics_context_set_stroke_color(ctx, fg);
    graphics_draw_line(ctx, GPoint(0, divider_y), GPoint(bounds.size.w, divider_y));
    return;
  }
#endif
#ifndef PBL_PLATFORM_APLITE
  // A remote presence session in the pinned "TRACKING" section - drawn as a
  // task row would be (white cell under the green strip), but from s_presence_*
  // since the watch may not hold that task at all.
  if (remote_in_pinned_section() && cell_index->section == 1) {
    GRect bounds = layer_get_bounds(cell_layer);
    bool is_selected = menu_layer_get_selected_index(s_menu_layer).section == 1 &&
                        menu_layer_get_selected_index(s_menu_layer).row == 0;
    GColor bg = is_selected ? GColorBlack : GColorWhite;
    GColor fg = is_selected ? GColorWhite : GColorBlack;
    graphics_context_set_fill_color(ctx, bg);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    graphics_context_set_text_color(ctx, fg);
    GRect title_box = GRect(TITLE_BOX_X, ROW_TITLE_TOP_Y(bounds.size.h, HEADING_TITLE_H, CHROME_STRIP_H),
                             bounds.size.w - TITLE_BOX_X * 2, HEADING_TITLE_H);
    graphics_draw_text(ctx, s_presence_task[0] != '\0' ? s_presence_task : "Live tracking",
                        fonts_get_system_font(TITLE_FONT_KEY), title_box,
                        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    char sub[52];
    if (s_presence_state == 1) {
      int total_s = (int)(time(NULL) - s_presence_elapsed_base);
      if (total_s < 0) {
        total_s = 0;
      }
      int h = total_s / 3600;
      int m = (total_s % 3600) / 60;
      if (h > 0) {
        snprintf(sub, sizeof(sub), "%s  %d:%02d", presence_state_phrase(), h, m);
      } else {
        snprintf(sub, sizeof(sub), "%s  %dm", presence_state_phrase(), m);
      }
    } else {
      snprintf(sub, sizeof(sub), "%s", presence_state_phrase());
    }
    GRect subtitle_box = GRect(TITLE_BOX_X, ROW_SUBTITLE_TOP_Y(bounds.size.h, HEADING_TITLE_H, CHROME_STRIP_H),
                                bounds.size.w - TITLE_BOX_X * 2, CHROME_STRIP_H);
    graphics_draw_text(ctx, sub, fonts_get_system_font(SUBTITLE_FONT_KEY), subtitle_box,
                        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    return;
  }
#endif

  Task *task = resolve_task_at(*cell_index);
  if (!task) {
    return;
  }

  bool is_selected = menu_layer_get_selected_index(s_menu_layer).section == cell_index->section &&
                      menu_layer_get_selected_index(s_menu_layer).row == cell_index->row;
  bool is_pinned_row = false;
#ifndef PBL_PLATFORM_APLITE
  is_pinned_row = has_pinned_row() && cell_index->section == 1;
#endif
  draw_task_row(ctx, layer_get_bounds(cell_layer), task, is_selected, is_pinned_row);
}

// ---------- outbound send retry ----------
// A watch->phone send can fail transiently - APP_MSG_SEND_TIMEOUT (no ack in
// time) is the most common cause, from a briefly congested BT link or a
// backgrounded phone app, not a real pairing problem. No API extends the
// ack-wait, so this retries the same message with a short backoff before
// surfacing the error overlay. Every send funnels through begin_send() so one
// place knows how to rebuild the last message (AppMessage has no "resend").
//
// aplite-excluded: s_retry_str needs MAX_ID_LEN (96 bytes) to cover every
// message type, well past aplite's ~10-byte margin. aplite keeps the immediate
// error - a real resilience regression there, but honest: the user sees the
// same error and can Resync manually.
#ifndef PBL_PLATFORM_APLITE
#define MAX_SEND_RETRIES 3
#define RETRY_BACKOFF_BASE_MS 1000
static int s_retry_msg_type = 0; // 0 = no message to retry if this send fails
// Sized for the largest payload: a task id (MAX_ID_LEN), habit id, or dictated
// title. MSG_REQUEST_SYNC/MSG_FINISH_DAY leave it empty.
static char s_retry_str[MAX_ID_LEN];
// Second string, only for MSG_NOTE_APPEND (TASK_ID in s_retry_str + the note
// text here). Sized to MAX_TITLE_LEN - the dictation session's own buffer size.
static char s_retry_str2[MAX_TITLE_LEN];
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

// Rebuilds and (re)sends the message s_retry_msg_type/str/int describe - the
// single source of truth for every send's wire format, used for both the first
// attempt (via begin_send) and any retry.
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
    case MSG_TRACK_TIME_START:
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
    case MSG_NOTE_REQUEST:
      dict_write_cstring(iter, KEY_TASK_ID, s_retry_str);
      break;
    case MSG_TASK_PLAN_TOMORROW:
    case MSG_TASK_UNSCHEDULE:
      dict_write_cstring(iter, KEY_TASK_ID, s_retry_str);
      break;
    case MSG_PROJECT_NOTE_APPEND:
      dict_write_cstring(iter, KEY_PROJECT_ID, s_retry_str);
      dict_write_cstring(iter, KEY_NOTE_TEXT, s_retry_str2);
      break;
    case MSG_PROJECT_NOTE_REQUEST:
      dict_write_cstring(iter, KEY_PROJECT_ID, s_retry_str);
      break;
#if PROJECTS_BROWSER
    case MSG_PROJECT_TASKS_REQUEST:
      dict_write_cstring(iter, KEY_PROJECT_ID, s_retry_str);
      break;
    case MSG_PROJECT_LIST_REQUEST:
#endif
    case MSG_FINISH_DAY:
    case MSG_REQUEST_SYNC:
    case MSG_PRESENCE_STOP:
    default:
      break; // no extra keys
  }
  app_message_outbox_send();
}

// Every watch-initiated send starts here: stash what it takes to rebuild this
// message for a retry, reset the retry count, and cancel any older pending
// retry timer so a stale retry can't resend outdated data. str_val2 is non-NULL
// only for MSG_NOTE_APPEND.
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
// Phase 2 live-tracking presence: tells the phone this watch just started (or
// resumed, elapsed_ms > 0) tracking a task, so it can broadcast "Tracking on
// Pebble" to the account's other devices. aplite-excluded with the rest of
// the presence feature.
static void send_track_time_start(const char *task_id, int32_t elapsed_ms) {
  begin_send(MSG_TRACK_TIME_START, task_id, NULL, elapsed_ms);
}
#endif

#ifndef PBL_PLATFORM_APLITE
// Move the task to tomorrow (tomorrow=true) or clear its scheduling. The phone
// turns this into an updateTask op and pushes a fresh list back - the task may
// leave a Today-only view. aplite-excluded with the gesture.
static void send_task_reschedule(const char *task_id, bool tomorrow) {
  begin_send(tomorrow ? MSG_TASK_PLAN_TOMORROW : MSG_TASK_UNSCHEDULE, task_id, NULL, 0);
}
#endif

#ifndef PBL_PLATFORM_APLITE
// Set right before send_finish_day() queues its message; outbox_sent_handler
// checks it to close the app only when the Finish Day send is what just
// CONFIRMED (not just queued - an eager close would hide a later send failure).
// outbox_failed_handler clears it too.
static bool s_close_after_finish_day_sent = false;

// No extra keys - the watch's Task struct is a trimmed display projection, so it
// can't build a full archive payload; the phone's state.task cache has
// everything (handleFinishDay in index.js). aplite-excluded with the row.
static void send_finish_day(void) {
  begin_send(MSG_FINISH_DAY, NULL, NULL, 0);
}
#endif

#ifndef PBL_PLATFORM_APLITE
static void send_task_add(const char *title) {
  begin_send(MSG_TASK_ADD, title, NULL, 0);
}

static void send_note_append(const char *id, const char *note_text, bool is_project) {
  begin_send(is_project ? MSG_PROJECT_NOTE_APPEND : MSG_NOTE_APPEND, id, note_text, 0);
}

// Fires when dictation finishes (success, cancel, or failure). Mic platforms
// only. Shared by Add Task and note-append (s_dictation_is_note_append routes).
static void dictation_status_callback(DictationSession *session, DictationSessionStatus status,
                                       char *transcription, void *context) {
  s_dictation_pending = false;
  if (status == DictationSessionStatusSuccess) {
    if (s_dictation_is_note_append) {
      send_note_append(s_notes_overlay_subject_id, transcription, s_notes_overlay_is_project);
    } else {
      send_task_add(transcription);
    }
    return;
  }
  if (status == DictationSessionStatusFailureTranscriptionRejected) {
    // User declined the transcription on the confirmation screen - a cancel,
    // not a failure, so a silent no-op.
    return;
  }
  // Every other failure already gets a dialog from the OS's own dictation UI,
  // so no error overlay here. Logged only.
  APP_LOG(APP_LOG_LEVEL_INFO, "dictation failed, status=%d", (int)status);
}

static void start_add_task_dictation(void) {
  if (s_dictation_pending || !s_dictation_session) {
    // Ignore a rapid double-press, and a call before window_load created the
    // session (shouldn't happen).
    return;
  }
  s_dictation_is_note_append = false;
  s_dictation_pending = true;
  dictation_session_start(s_dictation_session);
}

// Long-select on the notes overlay - dictates text to append to the shown
// task's or project's notes. Same session/guard as start_add_task_dictation,
// just tagged for the callback to route differently.
static void start_note_append_dictation(void) {
  if (s_dictation_pending || !s_dictation_session) {
    return;
  }
  s_dictation_is_note_append = true;
  s_dictation_pending = true;
  dictation_session_start(s_dictation_session);
}
#endif

#ifndef PBL_PLATFORM_APLITE
static void overtime_banner_timeout_callback(void *data) {
  s_overtime_banner_timer = NULL;
  hide_overtime_banner();
}

// Hides the over-estimate banner and cancels its auto-dismiss timer - safe to
// call whether or not it's showing.
static void hide_overtime_banner(void) {
  if (s_overtime_banner_timer) {
    app_timer_cancel(s_overtime_banner_timer);
    s_overtime_banner_timer = NULL;
  }
  if (s_overtime_banner_layer) {
    layer_set_hidden(text_layer_get_layer(s_overtime_banner_layer), true);
  }
}

static void show_overtime_banner(const char *task_title) {
  if (!s_overtime_banner_layer) {
    return;
  }
  snprintf(s_overtime_banner_text, sizeof(s_overtime_banner_text),
            "Over estimate\n%s", task_title);
  text_layer_set_text(s_overtime_banner_layer, s_overtime_banner_text);
  layer_set_hidden(text_layer_get_layer(s_overtime_banner_layer), false);
  layer_mark_dirty(text_layer_get_layer(s_overtime_banner_layer));
  vibes_double_pulse();
  if (s_overtime_banner_timer) {
    app_timer_cancel(s_overtime_banner_timer);
  }
  s_overtime_banner_timer = app_timer_register(OVERTIME_BANNER_MS, overtime_banner_timeout_callback, NULL);
}

// Called once per tracking tick: fires the over-estimate banner the first time
// effective time (synced spent + this session's elapsed) reaches the estimate.
// Latched via s_overtime_notified (re-armed if effective time drops back under).
// With the "repeat every 5 minutes" sub-option, re-fires every
// OVERTIME_REPEAT_INTERVAL_S while the task stays over.
static void maybe_notify_overtime(void) {
  if (!s_overtime_notify_enabled || s_tracking_task_id[0] == '\0') {
    return;
  }
  Task *task = find_task_by_id(s_tracking_task_id);
  if (!task || task->time_estimate_ms <= 0) {
    return;
  }
  int effective_ms = task->time_spent_ms;
  time_t elapsed_s = time(NULL) - s_tracking_start_epoch;
  if (elapsed_s > 0) {
    effective_ms += (int)elapsed_s * 1000;
  }
  if (effective_ms < task->time_estimate_ms) {
    s_overtime_notified = false; // re-arm for a later crossing
    return;
  }
  if (s_error_overlay_active) {
    return;
  }
  if (!s_overtime_notified) {
    s_overtime_notified = true;
    s_overtime_last_notify_epoch = time(NULL);
    show_overtime_banner(task->title);
    return;
  }
  // Already notified this crossing - "repeat every 5 minutes" re-fires it.
  if (s_overtime_repeat_enabled &&
      time(NULL) - s_overtime_last_notify_epoch >= OVERTIME_REPEAT_INTERVAL_S) {
    s_overtime_last_notify_epoch = time(NULL);
    show_overtime_banner(task->title);
  }
}

// Re-lays-out the task list after the pinned "TRACKING" section appears or
// disappears - the section and row counts change, so a full reload_data plus a
// scroll-state refresh.
static void refresh_pinned_section(void) {
  if (!s_menu_layer) {
    return;
  }
  menu_layer_reload_data(s_menu_layer);
  refresh_scroll_state(true);
}

static void unpin_timer_callback(void *data) {
  s_unpin_timer = NULL;
  s_pinned_task_id[0] = '\0';
  refresh_pinned_section();
}

// Cancels a pending unpin-grace timer - the task is about to be re-pinned, or
// the app is shutting down.
static void cancel_unpin_timer(void) {
  if (s_unpin_timer) {
    app_timer_cancel(s_unpin_timer);
    s_unpin_timer = NULL;
  }
}
#endif

static void tracking_tick_callback(void *data) {
  // Only the elapsed-time text changes each tick - mark_dirty (repaint), not
  // reload_data (which also re-asks for section/row counts).
  layer_mark_dirty(menu_layer_get_layer(s_menu_layer));
#ifndef PBL_PLATFORM_APLITE
  maybe_notify_overtime();
#endif
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
#ifndef PBL_PLATFORM_APLITE
  // Fresh session - re-arm the over-estimate banner and clear any stale one.
  s_overtime_notified = false;
  s_overtime_last_notify_epoch = 0;
  hide_overtime_banner();
  // Pin this task to the top (if enabled); cancel any grace timer from a
  // just-stopped task and re-lay-out the list.
  cancel_unpin_timer();
  strncpy(s_pinned_task_id, task->id, MAX_ID_LEN - 1);
  s_pinned_task_id[MAX_ID_LEN - 1] = '\0';
#endif
  start_tracking_tick();
#ifndef PBL_PLATFORM_APLITE
  if (has_pinned_row()) {
    refresh_pinned_section();
    // Highlight the freshly-pinned row (section 1, row 0).
    menu_layer_set_selected_index(s_menu_layer, MenuIndex(1, 0), MenuRowAlignCenter, false);
  }
  // Let the phone broadcast this as "Tracking on Pebble" to other devices.
  send_track_time_start(task->id, 0);
#endif
}

// Stops whatever's being tracked (a no-op if nothing is) and reports the
// elapsed session for upload (handleTrackTimeStop in index.js).
static void stop_tracking_and_report(void) {
  if (s_tracking_task_id[0] == '\0') {
    return;
  }
  time_t elapsed_s = time(NULL) - s_tracking_start_epoch;
#ifndef PBL_PLATFORM_APLITE
  // Always sent (even a 0ms session) so the phone can end the "Tracking on
  // Pebble" presence broadcast; the phone ignores a 0 delta for the op upload.
  int32_t elapsed_ms = elapsed_s > 0 ? (int32_t)elapsed_s * 1000 : 0;
  send_track_time_stop(s_tracking_task_id, elapsed_ms);
  if (elapsed_ms > 0) {
    Task *tracked_task = find_task_by_id(s_tracking_task_id);
    if (tracked_task) {
      tracked_task->time_spent_ms += elapsed_ms;
      save_tasks();
    }
  }
#else
  if (elapsed_s > 0) {
    int32_t elapsed_ms = (int32_t)elapsed_s * 1000;
    send_track_time_stop(s_tracking_task_id, elapsed_ms);
    // Optimistic local bump so the subtitle doesn't revert to the pre-session
    // total until the next full sync reports the real merged total.
    Task *tracked_task = find_task_by_id(s_tracking_task_id);
    if (tracked_task) {
      tracked_task->time_spent_ms += elapsed_ms;
      save_tasks();
    }
  }
#endif
  s_tracking_task_id[0] = '\0';
  s_tracking_start_epoch = 0;
  save_tracking();
#ifndef PBL_PLATFORM_APLITE
  s_overtime_notified = false;
  s_overtime_last_notify_epoch = 0;
  hide_overtime_banner();
  // Keep the just-stopped task pinned for a short grace period so it slides
  // back into its group smoothly. A new start_tracking() cancels this.
  if (s_pinned_task_id[0] != '\0') {
    cancel_unpin_timer();
    s_unpin_timer = app_timer_register(UNPIN_GRACE_MS, unpin_timer_callback, NULL);
  }
#endif
  stop_tracking_tick();
}

static void menu_select_click(MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  backlight_touch();
#if defined(PBL_TOUCH)
  // A touch tap: the bridge has already moved the highlight to the tapped row,
  // so just leave it selected - don't toggle it done, don't cancel a pending
  // reschedule (a swipe's own synthesised SELECT lands here too). Checked
  // before everything else. Physical Select is never guarded.
  if (consume_tap_select_guard()) {
    return;
  }
#endif
#ifndef PBL_PLATFORM_APLITE
  // A pending move-to-tomorrow / unschedule is cancelled by a physical Select
  // during its window - checked before the normal row handling so the press
  // only cancels (no toggle, no notes).
  if (s_pending_reschedule_kind != RESCHEDULE_NONE) {
    cancel_pending_reschedule();
    return;
  }
  // The over-estimate banner is a plain layer on the menu - a Select while
  // it's up just dismisses it, like the error overlay.
  if (s_overtime_banner_layer &&
      !layer_get_hidden(text_layer_get_layer(s_overtime_banner_layer))) {
    hide_overtime_banner();
    return;
  }
#endif
  // No s_notes_overlay_active check - the notes overlay is a separate pushed
  // Window, so this callback never fires while it's on top.
  if (s_error_overlay_active) {
    // Click routing goes through MenuLayer's config even while its layer is
    // hidden. Retry immediately after dismissing ("Select to retry") rather
    // than making the user find Resync - the usual cause is a transient send
    // failure where "try again" is the fix.
    hide_error_overlay();
    request_sync();
    return;
  }
  if (s_task_count == 0 && !ACTIONABLE_EMPTY_ACTIVE()) {
    // The empty/error screen's phantom row 0 - this makes "Select to retry"
    // retry. The actionable empty state falls through to normal section-0
    // routing below.
    request_sync();
    return;
  }
  if (cell_index->section == 0) {
    Section0RowKind kind = section0_row_kind((int)cell_index->row);
    if (kind == SECTION0_ROW_HABITS) {
      push_habits_window();
#if PROJECTS_BROWSER
    } else if (kind == SECTION0_ROW_PROJECTS) {
      push_browse_window(NULL);
#endif
#ifndef PBL_PLATFORM_APLITE
    } else if (kind == SECTION0_ROW_ADD_TASK) {
      start_add_task_dictation();
    } else if (kind == SECTION0_ROW_LIVE) {
      push_live_window();
#endif
    } else {
      request_sync(); // the "Resync" row
    }
    return;
  }
#ifndef PBL_PLATFORM_APLITE
  // A remote presence session riding the pinned "TRACKING" section - open its
  // detail screen, same as tapping the dark-blue LIVE row would.
  if (remote_in_pinned_section() && cell_index->section == 1) {
    push_live_window();
    return;
  }
  // The project row - Select opens that project's tasks in the browser,
  // long-Select opens its notes (menu_select_long_click). Same split the
  // browser's own project list uses.
  TaskGroup *project_row = resolve_project_row_at(*cell_index);
  if (project_row) {
#if PROJECTS_BROWSER
    push_browse_window(project_row->project_id);
#endif
    return;
  }
#endif
  Task *task = resolve_task_at(*cell_index);
  if (!task) {
    return;
  }
#ifndef PBL_PLATFORM_APLITE
  // A second Select on the SAME task before the pending toggle commits shows
  // notes instead of toggling. aplite-excluded with the notes feature.
  if (s_pending_toggle_timer && strncmp(s_pending_toggle_task_id, task->id, MAX_ID_LEN) == 0) {
    app_timer_cancel(s_pending_toggle_timer);
    s_pending_toggle_timer = NULL;
    s_pending_toggle_task_id[0] = '\0';
    show_notes_overlay(task);
    return;
  }
  // A different task's toggle was still pending - let it through now (it's
  // clearly not being double-clicked) and start a fresh window for this click.
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

// Long-select toggles time tracking on the highlighted task. One task at a
// time, so starting a new one first stops-and-reports the previous - mirrors
// the real app's single global "current task".
static void menu_select_long_click(MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  backlight_touch();
  // No s_notes_overlay_active check - note-append is wired on s_notes_window's
  // own click config.
  if (s_error_overlay_active || s_task_count == 0 || cell_index->section == 0) {
    return;
  }
#ifndef PBL_PLATFORM_APLITE
  if ((int)cell_index->section - GROUP_SECTION_BASE == s_group_count) {
    // Finish Day row. No optimistic local change - archiving needs the phone's
    // full state.task cache; this is fire-and-forget and the phone pushes an
    // updated list back. Closes the app once the send confirms, not eagerly.
    s_close_after_finish_day_sent = true;
    send_finish_day();
    return;
  }
  // The project row - long-Select opens its notes (Select opens its tasks).
  TaskGroup *project_row = resolve_project_row_at(*cell_index);
  if (project_row) {
    show_project_notes_overlay(project_row);
    return;
  }
#endif
  Task *task = resolve_task_at(*cell_index);
  if (!task || task->done) {
    return; // tracking a completed task isn't a real scenario - just ignore it
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
  // Restore s_empty_layer's plain font and hide the percent/hint subtitle -
  // both only used while syncing.
  text_layer_set_font(s_empty_layer, fonts_get_system_font(EMPTY_MSG_FONT_KEY));
  layer_set_hidden(text_layer_get_layer(s_sync_progress_layer), true);
#endif
}

#ifndef PBL_PLATFORM_APLITE
// Refreshes s_sync_progress_layer from s_status_msg - "Decrypting NN%" while a
// page of ops decrypts, or the "may take a few minutes" fallback before a
// percentage is available. The percent gets a bigger font (short enough not to
// wrap; the fallback sentence would).
static void update_sync_progress_text(void) {
  if (s_status_msg[0] != '\0') {
    text_layer_set_font(s_sync_progress_layer, fonts_get_system_font(EMPTY_MSG_FONT_KEY));
    text_layer_set_text(s_sync_progress_layer, s_status_msg);
  } else {
    text_layer_set_font(s_sync_progress_layer, fonts_get_system_font(CHROME_FONT_KEY));
    text_layer_set_text(s_sync_progress_layer, "This may take a few minutes");
  }
}
#endif

static void syncing_timer_callback(void *data) {
  s_syncing_dots = (s_syncing_dots + 1) % 4;
#ifdef PBL_PLATFORM_APLITE
  // No spare TextLayer on aplite - the percentage rides the same line/font.
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
  // Bigger than the other empty-state messages - safe because "Syncing..." is
  // always short, unlike the not-paired/error text sharing this layer.
  text_layer_set_font(s_empty_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  layer_set_hidden(text_layer_get_layer(s_sync_progress_layer), false);
  if (s_syncing_timer) {
    update_sync_progress_text(); // new percent mid-sync; timer already running
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

// Shows (or re-affirms) the fullscreen error overlay, hiding the menu and
// empty-state layers under it. Safe regardless of s_task_count.
static void show_error_overlay(void) {
  s_error_overlay_active = true;
  static char s_error_overlay_text[MAX_STATUS_MSG_LEN + 48];
  if (s_status_msg[0] != '\0') {
    snprintf(s_error_overlay_text, sizeof(s_error_overlay_text),
              "Sync Error\n\n%s\n\nSelect to retry", s_status_msg);
  } else {
    snprintf(s_error_overlay_text, sizeof(s_error_overlay_text), "Sync Error\n\nSelect to retry");
  }
  text_layer_set_text(s_error_layer, s_error_overlay_text);
  layer_set_hidden(text_layer_get_layer(s_error_layer), false);
  layer_set_hidden(menu_layer_get_layer(s_menu_layer), true);
  layer_set_hidden(text_layer_get_layer(s_empty_layer), true);
  layer_set_hidden(bitmap_layer_get_layer(s_logo_layer), true);
#ifndef PBL_PLATFORM_APLITE
  // Drop the over-estimate banner so it doesn't half-cover the error.
  hide_overtime_banner();
#endif
  stop_syncing_animation();
}

static void update_empty_layer(void) {
  // The error overlay owns menu/empty-layer visibility while it's up - without
  // this guard a background status update (e.g. a retry's TASK_SYNC_END) would
  // un-hide the menu under the overlay. The notes overlay needs no such guard
  // (it's a separate pushed Window).
  if (s_error_overlay_active) {
    return;
  }
  bool show_empty = (s_task_count == 0);
  // STATUS_OK with zero tasks is an "actionable" empty state: the menu stays
  // visible with "No tasks for today." as section 0's header and the real
  // rows reachable, rather than the standalone s_empty_layer/s_logo_layer.
  bool show_actionable_empty = show_empty && ACTIONABLE_EMPTY_ACTIVE();
  layer_set_hidden(text_layer_get_layer(s_empty_layer), !show_empty || show_actionable_empty);
  layer_set_hidden(bitmap_layer_get_layer(s_logo_layer), !show_empty || show_actionable_empty);
  layer_set_hidden(menu_layer_get_layer(s_menu_layer), show_empty && !show_actionable_empty);

  // Only the first sync (no cached list) gets the animation; a resync with a
  // populated list shows status via the Resync row's subtitle.
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

  // Only STATUS_NOT_PAIRED and STATUS_ERROR reach here (STATUS_OK and
  // STATUS_SYNCING were intercepted above).
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

// Dismisses the error overlay (Select) and hands visibility back to
// update_empty_layer() (unguarded now, since the flag flips first) to restore
// the right layer for the current s_task_count/s_status_code.
static void hide_error_overlay(void) {
  s_error_overlay_active = false;
  layer_set_hidden(text_layer_get_layer(s_error_layer), true);
  update_empty_layer();
}

#ifndef PBL_PLATFORM_APLITE
// "Notes:\n\n" is written into s_notes_full_text's buffer ahead of the chunks,
// not composed at render time (there's no bound to size a second buffer to).
#define NOTES_HEADER "Notes:\n\n"

// Height a word-wrapped text block lays out to at the given width. On-device,
// graphics_text_layout_get_content_size runs short of what TextLayer needs and
// the shortfall scales with length, so the margin is proportional (10%) plus a
// two-line floor. A short note just gets harmless extra scroll room.
#define NOTES_TEXT_HEIGHT_MARGIN_FLOOR 48

static int16_t measure_notes_text_height(int16_t width, const char *text) {
  GSize size = graphics_text_layout_get_content_size(
      text, fonts_get_system_font(NOTES_BODY_FONT_KEY), GRect(0, 0, width, 20000),
      GTextOverflowModeWordWrap, GTextAlignmentLeft);
  int16_t margin = size.h / 10;
  if (margin < NOTES_TEXT_HEIGHT_MARGIN_FLOOR) {
    margin = NOTES_TEXT_HEIGHT_MARGIN_FLOOR;
  }
  return size.h + margin;
}

// The names line - the comma-joined tags, or NOTES_TAGS_EMPTY_TEXT for an
// untagged task. Shared by measure_notes_tags_parts and notes_tags_layer_draw
// so measured and drawn never disagree.
static const char *notes_tags_display_line(void) {
  return s_notes_tags_line[0] != '\0' ? s_notes_tags_line : NOTES_TAGS_EMPTY_TEXT;
}

// Measures the bold "Tags:" label and the (wrapped) tag names separately, given
// the padded content width - shared by render_notes_overlay_content (needs the
// total) and notes_tags_layer_draw (needs each height) so they can't disagree.
static void measure_notes_tags_parts(int16_t content_w, int16_t *out_label_h, int16_t *out_names_h) {
  GSize label_size = graphics_text_layout_get_content_size(
      NOTES_TAGS_LABEL, fonts_get_system_font(NOTES_LABEL_FONT_KEY),
      GRect(0, 0, content_w, 2000), GTextOverflowModeWordWrap, GTextAlignmentLeft);
  GSize names_size = graphics_text_layout_get_content_size(
      notes_tags_display_line(), fonts_get_system_font(NOTES_BODY_FONT_KEY),
      GRect(0, 0, content_w, 2000), GTextOverflowModeWordWrap, GTextAlignmentLeft);
  *out_label_h = label_size.h;
  *out_names_h = names_size.h;
}

// Applies s_notes_display_text and s_notes_tags_line to the created
// TextLayers/ScrollLayer and resizes to match. Shared by notes_window_load and
// every later update. A no-op if the window isn't loaded (s_notes_layer NULL).
static void render_notes_overlay_content(void) {
  if (!s_notes_layer) {
    return;
  }
  // Tags line: a fixed header outside the ScrollLayer, staying put while the
  // body scrolls. Shown for any task subject (untagged -> NOTES_TAGS_EMPTY_TEXT),
  // zero-height for a project subject. Measured against the padded width so the
  // wrap point matches what's drawn.
  int16_t tags_height = 0;
  if (!s_notes_overlay_is_project) {
    int16_t label_h, names_h;
    measure_notes_tags_parts(s_notes_content_bounds.size.w - NOTES_TAGS_PADDING_X * 2, &label_h, &names_h);
    tags_height = label_h + names_h + NOTES_TAGS_PADDING_Y * 2;
  }
  layer_set_frame(s_notes_tags_layer,
                   GRect(s_notes_content_bounds.origin.x, s_notes_content_bounds.origin.y,
                         s_notes_content_bounds.size.w, tags_height));
  layer_mark_dirty(s_notes_tags_layer);

  // The scroll area starts right below the tags header (or right at the top
  // when there's no header) and shrinks to make room for it.
  GRect scroll_frame = GRect(s_notes_content_bounds.origin.x, s_notes_content_bounds.origin.y + tags_height,
                              s_notes_content_bounds.size.w, s_notes_content_bounds.size.h - tags_height);
  layer_set_frame(scroll_layer_get_layer(s_notes_scroll_layer), scroll_frame);

  text_layer_set_text(s_notes_layer, s_notes_display_text);
  // The new text can be a very different length - resize the scroll content and
  // snap back to the top rather than leaving the position mid-way through text
  // that may no longer be there.
  int16_t text_height = measure_notes_text_height(scroll_frame.size.w, s_notes_display_text);
  if (text_height < scroll_frame.size.h) {
    text_height = scroll_frame.size.h;
  }
  GRect text_frame = layer_get_frame(text_layer_get_layer(s_notes_layer));
  text_frame.origin.y = 0;
  text_frame.size.h = text_height;
  layer_set_frame(text_layer_get_layer(s_notes_layer), text_frame);
  scroll_layer_set_content_size(s_notes_scroll_layer, GSize(scroll_frame.size.w, text_height));
  scroll_layer_set_content_offset(s_notes_scroll_layer, GPointZero, false);
}

// Frees s_notes_full_text (free(NULL) is fine) and resets the fetch state for a
// new request. Does NOT touch s_notes_display_text - callers set that next.
static void reset_notes_full_buffer(void) {
  free(s_notes_full_text);
  s_notes_full_text = NULL;
  s_notes_full_len = 0;
  s_notes_full_capacity = 0;
  s_notes_fetch_state = NOTES_FETCH_IDLE;
}

static void cancel_notes_load_timeout(void) {
  if (s_notes_load_timeout_timer) {
    app_timer_cancel(s_notes_load_timeout_timer);
    s_notes_load_timeout_timer = NULL;
  }
}

// Fires when a note fetch is unanswered for NOTES_LOAD_TIMEOUT_MS. Only acts if
// the overlay is still open and still waiting.
static void notes_load_timeout_callback(void *data) {
  s_notes_load_timeout_timer = NULL;
  if (s_notes_overlay_active && s_notes_is_loading) {
    s_notes_is_loading = false;
    s_notes_display_text = NOTES_TIMEOUT_TEXT;
    render_notes_overlay_content();
  }
}

static void start_notes_load_timeout(void) {
  cancel_notes_load_timeout();
  s_notes_load_timeout_timer = app_timer_register(NOTES_LOAD_TIMEOUT_MS, notes_load_timeout_callback, NULL);
}

// Asks the phone for this subject's full notes (MSG_NOTE_REQUEST /
// MSG_PROJECT_NOTE_REQUEST) - the SYNC_START/CHUNK/SYNC_END reply is matched
// back to s_notes_overlay_subject_id, not to a stale in-flight request.
static void request_notes_full(const char *id, bool is_project) {
  begin_send(is_project ? MSG_PROJECT_NOTE_REQUEST : MSG_NOTE_REQUEST, id, NULL, 0);
  start_notes_load_timeout();
}

// Shared by show_notes_overlay (task) and show_project_notes_overlay - same
// pushed Window, different subject id/fetch type. Also a live refresh while the
// window is already open (a successful append re-triggers this): with
// s_notes_layer non-NULL it re-renders instead of pushing a second copy.
static void show_notes_overlay_for(const char *id, bool is_project) {
  s_notes_overlay_active = true;
  s_notes_overlay_is_project = is_project;
  strncpy(s_notes_overlay_subject_id, id, MAX_ID_LEN - 1);
  s_notes_overlay_subject_id[MAX_ID_LEN - 1] = '\0';
  reset_notes_full_buffer();
  s_notes_display_text = NOTES_LOADING_TEXT;
  s_notes_is_loading = true;
  request_notes_full(id, is_project);
  if (s_notes_layer) {
    render_notes_overlay_content();
    return;
  }
  push_notes_window();
}

// Shows a task's notes (double-click Select). s_notes_tags_line is set here
// synchronously (tags are sent with every task) so it stays visible above the
// notes whatever the fetch resolves to.
static void show_notes_overlay(Task *task) {
  strncpy(s_notes_tags_line, task->tags, sizeof(s_notes_tags_line) - 1);
  s_notes_tags_line[sizeof(s_notes_tags_line) - 1] = '\0';
  show_notes_overlay_for(task->id, false);
}

// Shows a project's notes (double-click Select on its project row). See
// MSG_PROJECT_NOTE_APPEND for what "a project's notes" means. No tags line -
// no tags-on-project concept.
static void show_project_notes_overlay(TaskGroup *group) {
  s_notes_tags_line[0] = '\0';
  show_notes_overlay_for(group->project_id, true);
}

// Dismisses the notes overlay (Select) - notes_window_unload clears
// s_notes_overlay_active on the pop, same as a Back-triggered dismissal, so
// both paths share the cleanup.
static void hide_notes_overlay(void) {
  window_stack_pop(true);
}

// Commits a single-click task-done toggle once the double-click window passes.
// Looks the task up by id - a background sync can rebuild s_tasks while this
// timer is pending, dangling a raw Task*.
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


// Clears a pending move-to-tomorrow / unschedule (Select pressed within the
// window, or the window's own task vanished) and redraws so the subtitle
// reverts.
static void cancel_pending_reschedule(void) {
  if (s_pending_reschedule_timer) {
    app_timer_cancel(s_pending_reschedule_timer);
    s_pending_reschedule_timer = NULL;
  }
  s_pending_reschedule_kind = RESCHEDULE_NONE;
  s_pending_reschedule_task_id[0] = '\0';
  if (s_menu_layer) {
    menu_layer_reload_data(s_menu_layer);
  }
}

// Commits the pending move-to-tomorrow / unschedule once its 3s cancel window
// passes. Looks the task up by id for the same background-sync reason as
// pending_toggle_timer_callback.
static void pending_reschedule_timer_callback(void *data) {
  s_pending_reschedule_timer = NULL;
  RescheduleKind kind = s_pending_reschedule_kind;
  Task *task = find_task_by_id(s_pending_reschedule_task_id);
  s_pending_reschedule_kind = RESCHEDULE_NONE;
  s_pending_reschedule_task_id[0] = '\0';
  if (task && kind != RESCHEDULE_NONE) {
    send_task_reschedule(task->id, kind == RESCHEDULE_TOMORROW);
  }
  // Drop the pending subtitle now; the phone's list push handles the rest.
  if (s_menu_layer) {
    menu_layer_reload_data(s_menu_layer);
  }
}

// Starts (or replaces) the pending reschedule for the currently-selected task.
// A long-press Up passes RESCHEDULE_UNSCHEDULE, long-press Down / swipe-left
// RESCHEDULE_TOMORROW.
static void begin_pending_reschedule(RescheduleKind kind) {
  if (s_error_overlay_active || s_task_count == 0 || kind == RESCHEDULE_NONE) {
    return;
  }
  Task *task = resolve_selected_task();
  if (!task) {
    return; // selection is on a pinned/project/action row, not a task
  }
  // A pending done-toggle on the same tap sequence would otherwise commit
  // mid-window - drop it in favour of this.
  if (s_pending_toggle_timer) {
    app_timer_cancel(s_pending_toggle_timer);
    s_pending_toggle_timer = NULL;
    s_pending_toggle_task_id[0] = '\0';
  }
  if (s_pending_reschedule_timer) {
    app_timer_cancel(s_pending_reschedule_timer);
  }
  strncpy(s_pending_reschedule_task_id, task->id, MAX_ID_LEN - 1);
  s_pending_reschedule_task_id[MAX_ID_LEN - 1] = '\0';
  s_pending_reschedule_kind = kind;
  s_pending_reschedule_timer =
      app_timer_register(RESCHEDULE_WINDOW_MS, pending_reschedule_timer_callback, NULL);
  vibes_short_pulse();
  menu_layer_reload_data(s_menu_layer);
}

// True if a NOTE_SYNC_* reply for id/is_project is about what the notes overlay
// is currently showing, not a stale reply for a subject the user backed out of.
static bool notes_reply_matches(const char *id, bool is_project) {
  return is_project == s_notes_overlay_is_project &&
         strncmp(id, s_notes_overlay_subject_id, MAX_ID_LEN) == 0;
}

// Shared by the MSG_NOTE_SYNC_START / MSG_PROJECT_NOTE_SYNC_START handlers.
static void handle_notes_sync_start(const char *id, bool is_project, int32_t total_len) {
  if (!notes_reply_matches(id, is_project)) {
    return;
  }
  reset_notes_full_buffer();
  if (total_len <= 0) {
    s_notes_fetch_state = NOTES_FETCH_EMPTY;
    return; // No chunks will follow - a SYNC_END will render the empty-notes text.
  }
  s_notes_full_capacity = (int)strlen(NOTES_HEADER) + (int)total_len + 1;
  s_notes_full_text = malloc((size_t)s_notes_full_capacity);
  if (!s_notes_full_text) {
    s_notes_full_capacity = 0;
    s_notes_fetch_state = NOTES_FETCH_FAILED;
    return;
  }
  memcpy(s_notes_full_text, NOTES_HEADER, strlen(NOTES_HEADER));
  s_notes_full_len = (int)strlen(NOTES_HEADER);
  s_notes_fetch_state = NOTES_FETCH_STARTED;
}

// Shared by MSG_NOTE_CHUNK/MSG_PROJECT_NOTE_CHUNK below.
static void handle_notes_chunk(const char *id, bool is_project, const char *chunk) {
  if (s_notes_fetch_state != NOTES_FETCH_STARTED || !notes_reply_matches(id, is_project)) {
    return;
  }
  int chunk_len = (int)strlen(chunk);
  // Bounds-checked against the capacity SYNC_START malloc'd - a chunk that would
  // overflow (a phone/watch length mismatch) is dropped, not overrun.
  if (s_notes_full_len + chunk_len < s_notes_full_capacity) {
    memcpy(s_notes_full_text + s_notes_full_len, chunk, (size_t)chunk_len);
    s_notes_full_len += chunk_len;
  }
}

// Shared by MSG_NOTE_SYNC_END/MSG_PROJECT_NOTE_SYNC_END below.
static void handle_notes_sync_end(const char *id, bool is_project) {
  if (!notes_reply_matches(id, is_project)) {
    return;
  }
  cancel_notes_load_timeout();
  s_notes_is_loading = false;
  switch (s_notes_fetch_state) {
    case NOTES_FETCH_STARTED:
      s_notes_full_text[s_notes_full_len] = '\0';
      s_notes_display_text = s_notes_full_text;
      break;
    case NOTES_FETCH_EMPTY:
      s_notes_display_text = is_project ? PROJECT_NOTES_EMPTY_TEXT : NOTES_EMPTY_TEXT;
      break;
    case NOTES_FETCH_FAILED:
    case NOTES_FETCH_IDLE:
    default:
      // malloc failed, or SYNC_START never arrived - a real failure, not a
      // legitimately empty note (NOTES_FETCH_EMPTY, above).
      s_notes_display_text = NOTES_TIMEOUT_TEXT;
      break;
  }
  render_notes_overlay_content();
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

#ifndef PBL_PLATFORM_APLITE
// Asks the phone to stop the live-tracking session it's currently showing us.
// No keys - the phone holds the session id (CAS-guarded on its side).
static void send_presence_stop(void) {
  begin_send(MSG_PRESENCE_STOP, NULL, NULL, 0);
}
#endif

#if PROJECTS_BROWSER
// Projects browser: ask the phone for the project list / one project's task
// list. Both replies are chunked (START / ITEM* / END) - see the MSG_PROJECT_*
// handlers in inbox_received_handler.
static void request_project_list(void) {
  begin_send(MSG_PROJECT_LIST_REQUEST, NULL, NULL, 0);
}

static void request_project_tasks(const char *project_id) {
  begin_send(MSG_PROJECT_TASKS_REQUEST, project_id, NULL, 0);
}
#endif

// The single place s_status_code is assigned (from MSG_SYNC_STATUS and from
// outbox_failed_handler's local STATUS_ERROR).
//
// Forcing the backlight on during a sync is disabled (#if 0, not //, because
// the block contains its own #ifndef/#else the preprocessor would still see).
// Re-enable by flipping the 0 to 1.
static void set_status_code(int32_t new_status_code) {
#if 0
  bool was_syncing = (s_status_code == STATUS_SYNCING);
  bool now_syncing = (new_status_code == STATUS_SYNCING);
  if (now_syncing && !was_syncing) {
    light_enable(true);
  } else if (!now_syncing && was_syncing) {
#ifndef PBL_PLATFORM_APLITE
    if (s_backlight_mode != BACKLIGHT_MODE_ALWAYS_ON) {
      light_enable(false);
    }
#else
    light_enable(false);
#endif
  }
#endif
  s_status_code = new_status_code;
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
      set_status_code(STATUS_SYNCING);
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
#ifndef PBL_PLATFORM_APLITE
      {
        Tuple *project_id_tuple = dict_find(iterator, KEY_TASK_PROJECT_ID);
        strncpy(s_incoming[idx].project_id, project_id_tuple ? project_id_tuple->value->cstring : "",
                MAX_PROJECT_ID_LEN - 1);
        s_incoming[idx].project_id[MAX_PROJECT_ID_LEN - 1] = '\0';
        Tuple *tags_tuple = dict_find(iterator, KEY_TASK_TAGS);
        strncpy(s_incoming[idx].tags, tags_tuple ? tags_tuple->value->cstring : "", MAX_TASK_TAGS_LEN - 1);
        s_incoming[idx].tags[MAX_TASK_TAGS_LEN - 1] = '\0';
#if TODAY_PROJECT_SWATCH
        Tuple *pcolor_tuple = dict_find(iterator, KEY_TASK_PROJECT_COLOR);
        s_incoming[idx].project_color = pcolor_tuple ? (uint8_t)pcolor_tuple->value->int32 : 0;
#endif
      }
#endif
      s_incoming[idx].done = done_tuple && done_tuple->value->int32 != 0;
      // Key absent (not 0, a valid 12:00am) means "no dueWithTime" - the phone
      // only sends it when the task has one. Same for the two below.
      s_incoming[idx].due_min = due_min_tuple ? due_min_tuple->value->int32 : -1;
      s_incoming[idx].time_spent_ms = time_spent_tuple ? time_spent_tuple->value->int32 : 0;
      s_incoming[idx].time_estimate_ms = time_estimate_tuple ? time_estimate_tuple->value->int32 : 0;
      break;
    }
    case MSG_TASK_SYNC_END: {
      int count = s_incoming_total < MAX_TASKS ? s_incoming_total : MAX_TASKS;
      memcpy(s_tasks, s_incoming, sizeof(Task) * (size_t)count);
      s_task_count = count;
      set_status_code(STATUS_OK);
      recompute_groups();
      save_tasks();
#ifndef PBL_PLATFORM_APLITE
      // A local tracking session whose task is gone from the synced list - even
      // though the phone force-includes any real tracked task (watchTaskList /
      // handleTrackStart). The task was deleted elsewhere, or an old build left
      // a session that never got a matching task. Clear the dead session: while
      // s_tracking_task_id is set the watch counts as "tracking locally", which
      // keeps a remote presence session off the pinned "TRACKING" section and
      // on the old dark-blue LIVE row (see remote_in_pinned_section). 0-delta
      // stop - no time to report, just end the "Tracking on Pebble" broadcast.
      if (s_tracking_task_id[0] != '\0' && find_task_by_id(s_tracking_task_id) == NULL) {
        send_track_time_stop(s_tracking_task_id, 0);
        s_tracking_task_id[0] = '\0';
        s_tracking_start_epoch = 0;
        save_tracking();
        s_overtime_notified = false;
        stop_tracking_tick();
        if (s_presence_state == 1) {
          start_tracking_tick(); // now drives the remote row's live elapsed
        }
      }
      // If the pinned task is gone from the list and nothing's being tracked,
      // drop the stale pin so it can't spuriously re-appear.
      if (s_pinned_task_id[0] != '\0' && s_tracking_task_id[0] == '\0' &&
          find_task_by_id(s_pinned_task_id) == NULL) {
        cancel_unpin_timer();
        s_pinned_task_id[0] = '\0';
      }
#endif
      menu_layer_reload_data(s_menu_layer);
      update_empty_layer();
      refresh_scroll_state(true); // the selected row may now be different
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
      // Written directly into s_habits (no separate incoming buffer - safe, see
      // the Habit struct comment).
      strncpy(s_habits[idx].id, id_tuple->value->cstring, MAX_HABIT_ID_LEN - 1);
      s_habits[idx].id[MAX_HABIT_ID_LEN - 1] = '\0';
      strncpy(s_habits[idx].title, title_tuple->value->cstring, MAX_TITLE_LEN - 1);
      s_habits[idx].title[MAX_TITLE_LEN - 1] = '\0';
      s_habits[idx].done = done_tuple && done_tuple->value->int32 != 0;
      s_habits[idx].value = value_tuple ? value_tuple->value->int32 : 0;
      s_habits[idx].goal = goal_tuple ? goal_tuple->value->int32 : 0;
      // habit type: 0 = ClickCounter, 1 = StopWatch, 2 = RepeatedCountdownReminder.
#ifdef PBL_PLATFORM_APLITE
      // aplite has no code budget for the countdown path, and both timer types
      // are excluded from its visible list identically, so collapse both into
      // is_stopwatch (is_countdown is never read there).
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
#if PROJECTS_BROWSER
    case MSG_PROJECT_LIST_START: {
      if (!s_browse_projects) {
        break;
      }
      Tuple *total_tuple = dict_find(iterator, KEY_PROJECT_TOTAL);
      s_browse_project_incoming = total_tuple ? total_tuple->value->int32 : 0;
      if (s_browse_project_incoming > MAX_BROWSE_PROJECTS) {
        s_browse_project_incoming = MAX_BROWSE_PROJECTS;
      }
      break;
    }
    case MSG_PROJECT_LIST_ITEM: {
      if (!s_browse_projects) {
        break;
      }
      Tuple *idx_tuple = dict_find(iterator, KEY_PROJECT_INDEX);
      Tuple *id_tuple = dict_find(iterator, KEY_PROJECT_ID);
      Tuple *title_tuple = dict_find(iterator, KEY_PROJECT_TITLE);
      if (!idx_tuple || !id_tuple || !title_tuple) {
        break;
      }
      int idx = idx_tuple->value->int32;
      if (idx < 0 || idx >= MAX_BROWSE_PROJECTS) {
        break;
      }
      strncpy(s_browse_projects[idx].id, id_tuple->value->cstring, MAX_PROJECT_ID_LEN - 1);
      s_browse_projects[idx].id[MAX_PROJECT_ID_LEN - 1] = '\0';
      strncpy(s_browse_projects[idx].title, title_tuple->value->cstring, MAX_TITLE_LEN - 1);
      s_browse_projects[idx].title[MAX_TITLE_LEN - 1] = '\0';
      Tuple *color_tuple = dict_find(iterator, KEY_PROJECT_COLOR);
      s_browse_projects[idx].color = color_tuple ? color_tuple->value->int32 : 0;
      break;
    }
    case MSG_PROJECT_LIST_END: {
      if (!s_browse_projects) {
        break;
      }
      s_browse_project_count = s_browse_project_incoming;
      s_browse_projects_loading = false;
#if PROJECTS_CACHE
      save_browse_projects();
#endif
      if (s_browse_menu && s_browse_level == 0) {
        menu_layer_reload_data(s_browse_menu);
        browse_update_empty();
      }
      break;
    }
    case MSG_PROJECT_TASKS_START: {
      Tuple *pid_tuple = dict_find(iterator, KEY_PROJECT_ID);
      // A reply for a project the user has already navigated away from - drop it.
      if (!s_browse_tasks || !pid_tuple ||
          strncmp(pid_tuple->value->cstring, s_browse_project_id, MAX_PROJECT_ID_LEN) != 0) {
        break;
      }
      Tuple *total_tuple = dict_find(iterator, KEY_TASK_TOTAL);
      s_browse_task_incoming = total_tuple ? total_tuple->value->int32 : 0;
      if (s_browse_task_incoming > MAX_BROWSE_TASKS) {
        s_browse_task_incoming = MAX_BROWSE_TASKS;
      }
      // Regular list first, then backlog - the boundary is the lowest index
      // carrying PROJECT_TASK_BACKLOG=1 (see the ITEM handler). Start it past
      // the end so "no backlog" leaves every row in the regular section.
      s_browse_backlog_start = s_browse_task_incoming;
      break;
    }
    case MSG_PROJECT_TASKS_ITEM: {
      Tuple *pid_tuple = dict_find(iterator, KEY_PROJECT_ID);
      if (!s_browse_tasks || !pid_tuple ||
          strncmp(pid_tuple->value->cstring, s_browse_project_id, MAX_PROJECT_ID_LEN) != 0) {
        break;
      }
      Tuple *idx_tuple = dict_find(iterator, KEY_TASK_INDEX);
      Tuple *id_tuple = dict_find(iterator, KEY_TASK_ID);
      Tuple *title_tuple = dict_find(iterator, KEY_TASK_TITLE);
      if (!idx_tuple || !id_tuple || !title_tuple) {
        break;
      }
      int idx = idx_tuple->value->int32;
      if (idx < 0 || idx >= MAX_BROWSE_TASKS) {
        break;
      }
      Task *bt = &s_browse_tasks[idx];
      memset(bt, 0, sizeof(Task));
      strncpy(bt->id, id_tuple->value->cstring, MAX_ID_LEN - 1);
      bt->id[MAX_ID_LEN - 1] = '\0';
      strncpy(bt->title, title_tuple->value->cstring, MAX_TITLE_LEN - 1);
      bt->title[MAX_TITLE_LEN - 1] = '\0';
      bt->project[0] = '\0';
      Tuple *done_tuple = dict_find(iterator, KEY_TASK_DONE);
      bt->done = done_tuple && done_tuple->value->int32 != 0;
      Tuple *due_tuple = dict_find(iterator, KEY_TASK_DUE_MIN);
      bt->due_min = due_tuple ? due_tuple->value->int32 : -1;
      Tuple *spent_tuple = dict_find(iterator, KEY_TASK_TIME_SPENT_MS);
      bt->time_spent_ms = spent_tuple ? spent_tuple->value->int32 : 0;
      Tuple *estimate_tuple = dict_find(iterator, KEY_TASK_TIME_ESTIMATE_MS);
      bt->time_estimate_ms = estimate_tuple ? estimate_tuple->value->int32 : 0;
      Tuple *backlog_tuple = dict_find(iterator, KEY_PROJECT_TASK_BACKLOG);
      if (backlog_tuple && backlog_tuple->value->int32 != 0 && idx < s_browse_backlog_start) {
        s_browse_backlog_start = idx;
      }
      break;
    }
    case MSG_PROJECT_TASKS_END: {
      Tuple *pid_tuple = dict_find(iterator, KEY_PROJECT_ID);
      if (!s_browse_tasks || !pid_tuple ||
          strncmp(pid_tuple->value->cstring, s_browse_project_id, MAX_PROJECT_ID_LEN) != 0) {
        break;
      }
      s_browse_task_count = s_browse_task_incoming;
      if (s_browse_backlog_start > s_browse_task_count) {
        s_browse_backlog_start = s_browse_task_count;
      }
      s_browse_tasks_loading = false;
      if (s_browse_menu && s_browse_level == 1) {
        menu_layer_reload_data(s_browse_menu);
        browse_update_empty();
      }
      break;
    }
#endif
    case MSG_SYNC_STATUS: {
      Tuple *status_tuple = dict_find(iterator, KEY_STATUS_CODE);
      if (status_tuple) {
        set_status_code(status_tuple->value->int32);
      }
      Tuple *msg_tuple = dict_find(iterator, KEY_STATUS_MSG);
      if (msg_tuple) {
        strncpy(s_status_msg, msg_tuple->value->cstring, MAX_STATUS_MSG_LEN - 1);
        s_status_msg[MAX_STATUS_MSG_LEN - 1] = '\0';
      } else {
        s_status_msg[0] = '\0';
      }
      // Feature toggles from the phone's pairing settings - optional fields,
      // absent-means-unchanged so a version mismatch can't reset a flag. Read
      // before reload_data so a change shows in the same redraw.
      Tuple *habits_enabled_tuple = dict_find(iterator, KEY_HABITS_ENABLED);
      if (habits_enabled_tuple) {
        s_habits_enabled = habits_enabled_tuple->value->int32 != 0;
      }
      Tuple *add_task_enabled_tuple = dict_find(iterator, KEY_ADD_TASK_ENABLED);
      if (add_task_enabled_tuple) {
        s_add_task_enabled = add_task_enabled_tuple->value->int32 != 0;
      }
      Tuple *projects_enabled_tuple = dict_find(iterator, KEY_PROJECTS_ENABLED);
      if (projects_enabled_tuple) {
        s_projects_enabled = projects_enabled_tuple->value->int32 != 0;
      }
      // Only re-applied when the value actually changed - this field is sent on
      // every status push (including routine background syncs), and re-triggering
      // the backlight each time would defeat a custom timeout.
#ifndef PBL_PLATFORM_APLITE
      Tuple *backlight_mode_tuple = dict_find(iterator, KEY_BACKLIGHT_MODE);
      if (backlight_mode_tuple && backlight_mode_tuple->value->int32 != s_backlight_mode) {
        s_backlight_mode = backlight_mode_tuple->value->int32;
        apply_backlight_mode();
      }
#endif
#ifndef PBL_PLATFORM_APLITE
      // Re-arm the background-wakeup schedule on the first confirmed interval
      // this launch and whenever it changes - not on every status push.
      Tuple *auto_sync_interval_tuple = dict_find(iterator, KEY_AUTO_SYNC_INTERVAL_MIN);
      if (auto_sync_interval_tuple &&
          (auto_sync_interval_tuple->value->int32 != s_auto_sync_interval_min || !s_wakeup_rescheduled_this_launch)) {
        s_auto_sync_interval_min = auto_sync_interval_tuple->value->int32;
        schedule_next_wakeup();
        s_wakeup_rescheduled_this_launch = true;
      }
#endif
#if defined(PBL_TOUCH)
      // Only acted on when the value changed - a redundant touch_service_(un)subscribe
      // on every routine status push is wasteful.
      Tuple *touch_nav_tuple = dict_find(iterator, KEY_TOUCH_NAV_ENABLED);
      if (touch_nav_tuple && (touch_nav_tuple->value->int32 != 0) != s_touch_nav_enabled) {
        s_touch_nav_enabled = touch_nav_tuple->value->int32 != 0;
        apply_touch_nav();
      }
#endif
#ifndef PBL_PLATFORM_APLITE
      Tuple *overtime_notify_tuple = dict_find(iterator, KEY_OVERTIME_NOTIFY_ENABLED);
      if (overtime_notify_tuple) {
        s_overtime_notify_enabled = overtime_notify_tuple->value->int32 != 0;
        if (!s_overtime_notify_enabled) {
          hide_overtime_banner();
        }
      }
      // Sub-option of the above. If switched on while a task is already over but
      // the banner has already fired (s_overtime_last_notify_epoch unset), start
      // the 5-minute clock now rather than firing on the next tick.
      Tuple *overtime_repeat_tuple = dict_find(iterator, KEY_OVERTIME_REPEAT_ENABLED);
      if (overtime_repeat_tuple) {
        bool was = s_overtime_repeat_enabled;
        s_overtime_repeat_enabled = overtime_repeat_tuple->value->int32 != 0;
        if (!was && s_overtime_repeat_enabled && s_overtime_notified &&
            s_overtime_last_notify_epoch == 0) {
          s_overtime_last_notify_epoch = time(NULL);
        }
      }
#endif
      // reload_data refreshes the Resync row's status subtitle;
      // update_empty_layer() handles the empty screen. Both no-op while the
      // error overlay is up.
      menu_layer_reload_data(s_menu_layer);
      update_empty_layer();
      if (s_status_code == STATUS_ERROR) {
        show_error_overlay();
      }
#ifndef PBL_PLATFORM_APLITE
      // A wakeup-launched session syncs quietly and gets out of the way - once
      // the sync concludes (OK/ERROR/NOT_PAIRED, not the initial SYNCING), pop
      // back to whatever was on screen. A manual session never takes this
      // branch. Guarded against a retried sync's second terminal status.
      if (s_is_wakeup_launch && !s_wakeup_exit_triggered &&
          (s_status_code == STATUS_OK || s_status_code == STATUS_ERROR || s_status_code == STATUS_NOT_PAIRED)) {
        s_wakeup_exit_triggered = true;
        APP_LOG(APP_LOG_LEVEL_INFO, "wakeup sync done (status %d), exiting", (int)s_status_code);
        // Marks this a deliberate completed action so exiting lands on the
        // watchface, not the app launcher menu.
        exit_reason_set(APP_EXIT_ACTION_PERFORMED_SUCCESSFULLY);
        window_stack_pop_all(true);
      }
#endif
      break;
    }
#ifndef PBL_PLATFORM_APLITE
    case MSG_PRESENCE_UPDATE: {
      Tuple *state_tuple = dict_find(iterator, KEY_PRESENCE_STATE);
      int new_state = state_tuple ? state_tuple->value->int32 : 0;
      s_presence_state = new_state;
      if (new_state == 0) {
        s_presence_task[0] = '\0';
        s_presence_device[0] = '\0';
        s_presence_can_stop = false;
        s_presence_stopping = false;
      } else {
        Tuple *title_tuple = dict_find(iterator, KEY_PRESENCE_TASK_TITLE);
        strncpy(s_presence_task, title_tuple ? title_tuple->value->cstring : "", sizeof(s_presence_task) - 1);
        s_presence_task[sizeof(s_presence_task) - 1] = '\0';
        Tuple *device_tuple = dict_find(iterator, KEY_PRESENCE_DEVICE);
        strncpy(s_presence_device, device_tuple ? device_tuple->value->cstring : "", sizeof(s_presence_device) - 1);
        s_presence_device[sizeof(s_presence_device) - 1] = '\0';
        Tuple *elapsed_tuple = dict_find(iterator, KEY_PRESENCE_ELAPSED_S);
        s_presence_elapsed_base = time(NULL) - (time_t)(elapsed_tuple ? elapsed_tuple->value->int32 : 0);
        Tuple *can_stop_tuple = dict_find(iterator, KEY_PRESENCE_CAN_STOP);
        s_presence_can_stop = can_stop_tuple && can_stop_tuple->value->int32 != 0;
        // The phone confirms a stop by clearing (state 0), never by another
        // still-live update - so any fresh state drops the "Stopping..." latch.
        s_presence_stopping = false;
      }
      // reload + scroll refresh covers both the dark-blue section-0 row count
      // changing and the pinned "TRACKING" section appearing/disappearing.
      refresh_pinned_section();
      // The pinned-section remote row needs a per-second redraw for its live
      // elapsed. tracking_tick_callback just marks the menu dirty; it's a
      // no-op for the over-estimate banner when nothing is tracked locally.
      if (s_tracking_task_id[0] == '\0') {
        if (remote_in_pinned_section() && s_presence_state == 1) {
          start_tracking_tick();
        } else {
          stop_tracking_tick();
        }
      }
      live_window_refresh(); // updates or pops the detail window if it's open
      break;
    }
    case MSG_PRESENCE_STOP_LOCAL: {
      // A remote device stopped the timer this watch is running. Stop it the
      // same as a long-press would - stop_tracking_and_report() sends the
      // MSG_TRACK_TIME_STOP the phone turns into the presence "stopped" ack.
      if (s_tracking_task_id[0] != '\0') {
        stop_tracking_and_report();
        menu_layer_reload_data(s_menu_layer);
        refresh_scroll_state(true);
      }
      break;
    }
    case MSG_NOTE_SYNC_START: {
      Tuple *id_tuple = dict_find(iterator, KEY_TASK_ID);
      Tuple *total_tuple = dict_find(iterator, KEY_NOTE_TOTAL_LEN);
      if (!id_tuple || !total_tuple) {
        break;
      }
      handle_notes_sync_start(id_tuple->value->cstring, false, total_tuple->value->int32);
      break;
    }
    case MSG_NOTE_CHUNK: {
      Tuple *id_tuple = dict_find(iterator, KEY_TASK_ID);
      Tuple *chunk_tuple = dict_find(iterator, KEY_NOTE_CHUNK_TEXT);
      if (!id_tuple || !chunk_tuple) {
        break;
      }
      handle_notes_chunk(id_tuple->value->cstring, false, chunk_tuple->value->cstring);
      break;
    }
    case MSG_NOTE_SYNC_END: {
      Tuple *id_tuple = dict_find(iterator, KEY_TASK_ID);
      if (!id_tuple) {
        break;
      }
      handle_notes_sync_end(id_tuple->value->cstring, false);
      break;
    }
    case MSG_PROJECT_NOTE_SYNC_START: {
      Tuple *id_tuple = dict_find(iterator, KEY_PROJECT_ID);
      Tuple *total_tuple = dict_find(iterator, KEY_NOTE_TOTAL_LEN);
      if (!id_tuple || !total_tuple) {
        break;
      }
      handle_notes_sync_start(id_tuple->value->cstring, true, total_tuple->value->int32);
      break;
    }
    case MSG_PROJECT_NOTE_CHUNK: {
      Tuple *id_tuple = dict_find(iterator, KEY_PROJECT_ID);
      Tuple *chunk_tuple = dict_find(iterator, KEY_NOTE_CHUNK_TEXT);
      if (!id_tuple || !chunk_tuple) {
        break;
      }
      handle_notes_chunk(id_tuple->value->cstring, true, chunk_tuple->value->cstring);
      break;
    }
    case MSG_PROJECT_NOTE_SYNC_END: {
      Tuple *id_tuple = dict_find(iterator, KEY_PROJECT_ID);
      if (!id_tuple) {
        break;
      }
      handle_notes_sync_end(id_tuple->value->cstring, true);
      break;
    }
#endif
    default:
      break;
  }
}

static void inbox_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_WARNING, "AppMessage dropped: %d", (int)reason);
}

#ifndef PBL_PLATFORM_APLITE
// Fires once the phone confirms delivery (not just "queued") of any outbound
// message - closes the app only when the Finish Day send just succeeded, and
// clears the retry state so a later failure can't re-send a message that went
// through.
static void outbox_sent_handler(DictionaryIterator *iterator, void *context) {
  clear_pending_retry();
  if (s_close_after_finish_day_sent) {
    s_close_after_finish_day_sent = false;
    window_stack_pop_all(true);
  }
}

// The AppMessageResult reasons that are transient (a busy/backgrounded phone or
// BT link) and worth a short retry. Excludes structural problems with the
// call/message itself, which would just fail identically.
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
  s_close_after_finish_day_sent = false; // don't let a later send close the app
  // Retry the same message (short backoff) before the error overlay - transient
  // failures only, up to MAX_SEND_RETRIES. A truly unreachable phone still ends
  // at the same error, just seconds later.
  if (s_retry_msg_type != 0 && is_retryable_failure(reason) && s_retry_count < MAX_SEND_RETRIES) {
    s_retry_count++;
    uint32_t delay_ms = (uint32_t)RETRY_BACKOFF_BASE_MS << (s_retry_count - 1); // 1s, 2s, 4s
    s_retry_timer = app_timer_register(delay_ms, retry_timer_callback, NULL);
    return;
  }
  clear_pending_retry();
#endif
  // Route the failure through the same fullscreen overlay as a phone->watch
  // sync error so every send failure is visible, not silently swallowed.
  // Reached immediately on aplite (no retry) or once the retries above run out.
  set_status_code(STATUS_ERROR);
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
  // Timer-type habits are skipped entirely on aplite, so row indices walk only
  // the plain-count subset (is_stopwatch alone is enough - is_countdown is
  // never true there).
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

// Everything through stop_habit_tracking_and_report is the StopWatch/countdown
// habit timer - aplite-excluded. habits_menu_draw_row/select_long_click have
// narrower guards so a StopWatch still shows its progress read-only there.
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

// Total elapsed ms for the current countdown session, paused or running. Only
// meaningful while s_tracking_habit_id is an is_countdown habit.
static int countdown_elapsed_ms(void) {
  if (s_habit_countdown_paused) {
    return s_habit_countdown_frozen_elapsed_ms;
  }
  time_t elapsed_s = time(NULL) - s_tracking_habit_start_epoch;
  int running_ms = elapsed_s > 0 ? (int)elapsed_s * 1000 : 0;
  return s_habit_countdown_frozen_elapsed_ms + running_ms;
}

// A RepeatedCountdownReminder's timer reaching zero: +1 to today's count via
// the same HABIT_ADJUST path a ClickCounter's Select uses (idempotent - the
// phone applies the delta and uploads a plain replace). Fires automatically at
// zero, but does NOT auto-restart the next round (that would rack up completions
// unattended); long-select starts the next round.
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
// tracking_tick_callback - mark_dirty, not reload_data (except a countdown
// reaching zero). Guarded on s_habits_menu_layer being non-NULL (it's torn down
// on habits-window unload).
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
  // A fresh round always starts running - clear any leftover pause state.
  s_habit_countdown_paused = false;
  s_habit_countdown_frozen_elapsed_ms = 0;
  save_habit_tracking();
  start_habit_tracking_tick();
}

// Select on a tracking is_countdown row toggles paused/running; long-select
// still ends the round. Not offered for a StopWatch (pause is meaningless for
// an open-ended up-count).
static void toggle_habit_countdown_pause(void) {
  if (s_habit_countdown_paused) {
    s_habit_countdown_paused = false;
    s_tracking_habit_start_epoch = time(NULL); // start a fresh running segment
    start_habit_tracking_tick();
  } else {
    s_habit_countdown_frozen_elapsed_ms = countdown_elapsed_ms(); // fold in the running segment before flipping the flag
    s_habit_countdown_paused = true;
    stop_habit_tracking_tick();
  }
  save_habit_tracking();
  if (s_habits_menu_layer) {
    menu_layer_reload_data(s_habits_menu_layer);
  }
}

// Stops whatever StopWatch/countdown habit is being tracked (a no-op if none) -
// mirrors stop_tracking_and_report, including the optimistic local bump. For a
// RepeatedCountdownReminder this only means "cancelled before completion" (zero
// is handled by complete_habit_countdown), so it's a silent no-upload cancel.
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
  // Matches resolve_habit_at's aplite-only filtering.
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
  // Cerulean selected background, matching the Habits nav row, so a highlighted
  // habit ties back to the row that brought you here.
  GColor bg = is_selected ? GColorVividCerulean : GColorWhite;
  // A done habit's title stays full-strength (unlike a done task's, which dims):
  // the "- Done" subtitle carries the signal, and a habit gets incremented past
  // goal / decremented below it on the same day, so dimming would flicker.
  // Text stays black even when selected - the cerulean is light enough.
  GColor fg = GColorBlack;
  graphics_context_set_fill_color(ctx, bg);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  graphics_context_set_text_color(ctx, fg);

  // Non-emery: the fixed 30px top-aligned title box, unchanged. Emery: a
  // measured, vertically-centred block matching draw_task_row.
#ifdef PBL_PLATFORM_EMERY
  GFont habit_title_font = fonts_get_system_font(TITLE_FONT_KEY);
  GSize habit_line = graphics_text_layout_get_content_size(
      "Ag", habit_title_font, GRect(0, 0, 200, 100), GTextOverflowModeFill, GTextAlignmentLeft);
  int16_t habit_title_h = habit_line.h > 0 ? habit_line.h : HEADING_TITLE_H;
  GRect title_box = GRect(TITLE_BOX_X, ROW_TITLE_TOP_Y(bounds.size.h, habit_title_h, SUBTITLE_STRIP_H),
                           bounds.size.w - TITLE_BOX_X * 2, habit_title_h);
  graphics_draw_text(ctx, habit->title, habit_title_font, title_box,
                      GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
#else
  GRect title_box = GRect(TITLE_BOX_X, TITLE_BOX_Y, bounds.size.w - TITLE_BOX_X * 2, 30);
  graphics_draw_text(ctx, habit->title, fonts_get_system_font(TITLE_FONT_KEY), title_box,
                      GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
#endif

  // "value/goal" always visible, " - Done" appended once the count reaches goal
  // (kept alongside "Done" since incrementing past goal is possible - "Done"
  // alone can't tell 3/3 from 7/3).
  // Sized for the StopWatch worst case: two format_duration_ms outputs joined by
  // " / " plus " - Done".
  char subtitle[56];
#ifndef PBL_PLATFORM_APLITE
  if (habit->is_stopwatch) {
    // "spent / goal" via format_duration_ms - value/goal are ms here.
    // effective_ms adds this session's running elapsed to the synced value.
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
    // Running: remaining time counting to zero, with format_duration_ms's "> "
    // prefix as the "live" marker (dropped while paused). Otherwise: a plain
    // completed-rounds count, same shape as a ClickCounter's value/goal.
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
  // On aplite both branches above are compiled out (resolve_habit_at filters
  // timer habits out), leaving just this plain-count path.
  if (habit->done) {
    snprintf(subtitle, sizeof(subtitle), "%d/%d - Done", habit->value, habit->goal);
  } else {
    snprintf(subtitle, sizeof(subtitle), "%d/%d", habit->value, habit->goal);
  }
#ifdef PBL_PLATFORM_EMERY
  GRect subtitle_box = GRect(TITLE_BOX_X, ROW_SUBTITLE_TOP_Y(bounds.size.h, habit_title_h, SUBTITLE_STRIP_H),
                              bounds.size.w - TITLE_BOX_X * 2, SUBTITLE_STRIP_H);
#else
  GRect subtitle_box = GRect(TITLE_BOX_X, bounds.size.h - SUBTITLE_STRIP_H,
                              bounds.size.w - TITLE_BOX_X * 2, SUBTITLE_STRIP_H);
#endif
  graphics_draw_text(ctx, subtitle, fonts_get_system_font(SUBTITLE_FONT_KEY), subtitle_box,
                      GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
}

// Select +1, long-select -1 (never below 0). The phone applies the delta to its
// cached value and uploads a plain replace, so this local bump is a guess
// corrected by the next full sync.
static void adjust_habit(MenuIndex index, int32_t delta) {
  Habit *habit = resolve_habit_at(index);
  if (!habit || habit->value + delta < 0) {
    return; // already at 0, trying to go lower - silent no-op
  }
  habit->value += delta;
  habit->done = habit->value >= habit->goal;
  save_habits();
  menu_layer_reload_data(s_habits_menu_layer);
  send_habit_adjust(habit, delta);
}

static void habits_menu_select_click(MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  backlight_touch();
#if defined(PBL_TOUCH)
  // A touch tap only selects the row - see menu_select_click's matching guard.
  if (consume_tap_select_guard()) {
    return;
  }
#endif
  Habit *habit = resolve_habit_at(*cell_index);
#ifndef PBL_PLATFORM_APLITE
  // Select pauses/resumes an is_countdown habit's timer while it's tracking
  // (long-select still ends the round). Not for a StopWatch. Otherwise falls
  // through to the no-op below - long-select starts a fresh round.
  if (habit && habit->is_countdown) {
    bool is_tracking_this = s_tracking_habit_id[0] != '\0' &&
                             strncmp(s_tracking_habit_id, habit->id, MAX_HABIT_ID_LEN) == 0;
    if (is_tracking_this) {
      toggle_habit_countdown_pause();
    }
    return;
  }
#endif
  // is_countdown is never true on aplite - is_stopwatch alone saves a few bytes.
#ifdef PBL_PLATFORM_APLITE
  if (habit && habit->is_stopwatch) {
#else
  if (habit && (habit->is_stopwatch || habit->is_countdown)) {
#endif
    return; // no plain-count action for a timer habit - long-select runs its timer
  }
  adjust_habit(*cell_index, 1);
}

// Long-select toggles the timer on a StopWatch/countdown habit (one at a time,
// mirroring the task timer), or decrements a ClickCounter by one.
// stop_habit_tracking_and_report() handles the two timer types itself.
static void habits_menu_select_long_click(MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  backlight_touch();
  Habit *habit = resolve_habit_at(*cell_index);
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
    return; // no tracking on aplite - a long-press on a timer row is a no-op there
  }
  adjust_habit(*cell_index, -1);
}

static void update_habits_empty_layer(void) {
  // habits_menu_get_num_rows(), not s_habit_count - on aplite a list of only
  // StopWatch habits has s_habit_count > 0 but zero visible rows.
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
  text_layer_set_font(s_habits_empty_layer, fonts_get_system_font(EMPTY_MSG_FONT_KEY));
  text_layer_set_text(s_habits_empty_layer, "No habits synced.");
  layer_add_child(window_layer, text_layer_get_layer(s_habits_empty_layer));

  update_habits_empty_layer();

#ifndef PBL_PLATFORM_APLITE
  // Resume the live-ticking redraw if a habit was already being tracked (the
  // elapsed time comes from the persisted start timestamp; this just restarts
  // the redraw). Not while paused - nothing to tick.
  if (s_tracking_habit_id[0] != '\0' && !s_habit_countdown_paused) {
    start_habit_tracking_tick();
  }
#endif
}

static void habits_window_unload(Window *window) {
#ifndef PBL_PLATFORM_APLITE
  // Cancel before destroying s_habits_menu_layer - a still-running timer
  // touching a destroyed layer is what this ordering avoids.
  stop_habit_tracking_tick();
#endif
  menu_layer_destroy(s_habits_menu_layer);
  text_layer_destroy(s_habits_empty_layer);
  status_bar_layer_destroy(s_habits_status_bar);
}

// Created once and reused (pushed again on every visit) - only its layers are
// torn down and rebuilt each time, so at most one window's worth exists.
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

#if PROJECTS_BROWSER
// ================= Projects browser =================
// One window, one MenuLayer. s_browse_level: 0 = the project list, 1 = one
// project's tasks. Back at level 1 returns to the list; at level 0 it pops
// the window. Data lives in s_browse_*; the project list is persisted, task
// lists are always fetched.

static BrowseProject *resolve_browse_project_at(MenuIndex index) {
  if (!s_browse_projects || index.section != 0 || (int)index.row >= s_browse_project_count) {
    return NULL;
  }
  return &s_browse_projects[index.row];
}

#if PROJECTS_CACHE
// Persist the project list (same one-blob shape as save_tasks) so it renders
// immediately on the next open and stays viewable with the phone away.
static void save_browse_projects(void) {
  if (s_browse_projects && s_browse_project_count > 0) {
    persist_write_data(PERSIST_KEY_BROWSE_PROJECTS, s_browse_projects,
                       sizeof(BrowseProject) * (size_t)s_browse_project_count);
    persist_write_int(PERSIST_KEY_BROWSE_PROJECTS + 1, s_browse_project_count);
  }
}

static void load_browse_projects(void) {
  if (!s_browse_projects || !persist_exists(PERSIST_KEY_BROWSE_PROJECTS + 1)) {
    return;
  }
  int count = persist_read_int(PERSIST_KEY_BROWSE_PROJECTS + 1);
  if (count > 0 && count <= MAX_BROWSE_PROJECTS) {
    int want = (int)(sizeof(BrowseProject) * (size_t)count);
    if (persist_read_data(PERSIST_KEY_BROWSE_PROJECTS, s_browse_projects, want) == want) {
      s_browse_project_count = count;
    }
  }
}
#endif // PROJECTS_CACHE

// ---- one project's task list (level 1) ----
// Rows [0, s_browse_backlog_start) are the regular list; the rest are the
// backlog, shown as a second section under a "Backlog" header (the divider).

static int pt_regular_count(void) {
  int r = s_browse_backlog_start;
  if (r > s_browse_task_count) {
    r = s_browse_task_count;
  }
  if (r < 0) {
    r = 0;
  }
  return r;
}

static int pt_backlog_count(void) {
  return s_browse_task_count - pt_regular_count();
}

// Whether section `section` is the backlog one. When there's no regular list,
// the backlog takes section 0; otherwise it's section 1.
static bool pt_section_is_backlog(int section) {
  if (pt_regular_count() == 0) {
    return pt_backlog_count() > 0;
  }
  return section == 1;
}

static Task *resolve_browse_task_at(MenuIndex index) {
  if (!s_browse_tasks || s_browse_task_count == 0) {
    return NULL;
  }
  int base = pt_section_is_backlog((int)index.section) ? pt_regular_count() : 0;
  int i = base + (int)index.row;
  if (i >= s_browse_task_count) {
    return NULL;
  }
  return &s_browse_tasks[i];
}

// ---- shared menu callbacks (branch on s_browse_level) ----

static uint16_t browse_menu_get_num_sections(MenuLayer *menu_layer, void *context) {
  if (s_browse_level == 0) {
    return 1;
  }
  int n = (pt_regular_count() > 0 ? 1 : 0) + (pt_backlog_count() > 0 ? 1 : 0);
  return (uint16_t)(n > 0 ? n : 1);
}

static uint16_t browse_menu_get_num_rows(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  if (s_browse_level == 0) {
    return (uint16_t)s_browse_project_count;
  }
  if (s_browse_task_count == 0) {
    return 0;
  }
  return (uint16_t)(pt_section_is_backlog((int)section_index) ? pt_backlog_count() : pt_regular_count());
}

static int16_t browse_menu_get_header_height(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  return (s_browse_level == 1 && pt_section_is_backlog((int)section_index)) ? MENU_CELL_BASIC_HEADER_HEIGHT : 0;
}

static void browse_menu_draw_header(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *context) {
  if (s_browse_level == 1 && pt_section_is_backlog((int)section_index)) {
    menu_cell_basic_header_draw(ctx, cell_layer, "Backlog");
  }
}

static void browse_menu_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context) {
  GRect bounds = layer_get_bounds(cell_layer);
  MenuIndex sel = menu_layer_get_selected_index(s_browse_menu);
  bool is_selected = sel.section == cell_index->section && sel.row == cell_index->row;
  if (s_browse_level == 0) {
    BrowseProject *p = resolve_browse_project_at(*cell_index);
    if (!p) {
      return;
    }
    // Green with bold black text - the same treatment the today view gives a
    // project group header (menu_draw_header). The selected row darkens to
    // GColorIslamicGreen with white text so it stands out (a bare text-colour
    // flip on the bright green barely read).
    graphics_context_set_fill_color(ctx, is_selected ? GColorIslamicGreen : GColorGreen);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    // The project's theme colour as a swatch on the left (draw_project_swatch,
    // shared with the today view). The phone already packed it to a GColor8.
    int16_t text_x = draw_project_swatch(ctx, TITLE_BOX_X, bounds.size.h, (uint8_t)p->color);
    graphics_context_set_text_color(ctx, is_selected ? GColorWhite : GColorBlack);
    graphics_draw_text(ctx, p->title, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD),
                        GRect(text_x, HEADING_TITLE_Y(bounds.size.h),
                              bounds.size.w - text_x - TITLE_BOX_X, HEADING_TITLE_H),
                        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    return;
  }
  Task *bt = resolve_browse_task_at(*cell_index);
  if (!bt) {
    return;
  }
  // draw_task_row reads the same fields the today list draws; a browsed task
  // isn't in s_tasks but the struct is identical, so this reuses it wholesale
  // (including the live-ticking "spent / estimate" when it's the tracked one).
  draw_task_row(ctx, bounds, bt, is_selected, false);
}

// Switch to level 1 for `project_id` and fetch its tasks. Shared by a
// project-row Select in the browser, by push_browse_window's jump-straight-to
// path, and by the today view's project row (via push_browse_window).
static void browse_descend(const char *project_id) {
  strncpy(s_browse_project_id, project_id, MAX_PROJECT_ID_LEN - 1);
  s_browse_project_id[MAX_PROJECT_ID_LEN - 1] = '\0';
  if (!s_browse_tasks) {
    s_browse_tasks = malloc(sizeof(Task) * MAX_BROWSE_TASKS);
  }
  s_browse_task_count = 0;
  s_browse_task_incoming = 0;
  s_browse_backlog_start = 0;
  s_browse_tasks_loading = true;
  s_browse_level = 1;
  if (s_browse_menu) {
    menu_layer_set_selected_index(s_browse_menu, MenuIndex(0, 0), MenuRowAlignTop, false);
    menu_layer_reload_data(s_browse_menu);
    browse_update_empty();
  }
  request_project_tasks(s_browse_project_id);
}

// Level 0: Select opens the project's tasks. Level 1: Select toggles the task
// done - same send the today list uses, mirrored onto the today list too.
static void browse_menu_select_click(MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  backlight_touch();
  if (s_browse_level == 0) {
    BrowseProject *p = resolve_browse_project_at(*cell_index);
    if (!p) {
      return;
    }
    browse_descend(p->id);
    return;
  }
  Task *bt = resolve_browse_task_at(*cell_index);
  if (!bt) {
    return;
  }
  bt->done = !bt->done;
  Task *in_today = find_task_by_id(bt->id);
  if (in_today) {
    in_today->done = bt->done;
    save_tasks();
    menu_layer_reload_data(s_menu_layer);
  }
  menu_layer_reload_data(s_browse_menu);
  send_task_toggle(bt);
}

// Level 0: long-Select opens the selected project's notes (a project has no
// tags, hence the cleared tags line). Level 1: long-Select starts / stops
// tracking the task, same as a long-Select on the today list.
static void browse_menu_select_long_click(MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  backlight_touch();
  if (s_browse_level == 0) {
    BrowseProject *p = resolve_browse_project_at(*cell_index);
    if (!p) {
      return;
    }
    s_notes_tags_line[0] = '\0';
    show_notes_overlay_for(p->id, true);
    return;
  }
  Task *bt = resolve_browse_task_at(*cell_index);
  if (!bt) {
    return;
  }
  bool already_tracking_this = s_tracking_task_id[0] != '\0' &&
                                strncmp(s_tracking_task_id, bt->id, MAX_ID_LEN) == 0;
  stop_tracking_and_report();
  if (!already_tracking_this) {
    // Prefer the real today-list Task so the over-estimate latch sees the right
    // estimate; the browsed struct works too (start_tracking only reads ->id).
    Task *real = find_task_by_id(bt->id);
    start_tracking(real ? real : bt);
    // start_tracking's MSG_TRACK_TIME_START tells the phone the id; the phone
    // then re-pushes the task list with this task force-included (see
    // handleTrackStart in index.js) so the today page's pinned "TRACKING"
    // section picks it up. A sync request from here would just collide with
    // that MSG_TRACK_TIME_START on the single outbox slot and be dropped.
  }
  menu_layer_reload_data(s_browse_menu);
  vibes_short_pulse();
}

static void browse_update_empty(void) {
  bool empty;
  const char *msg;
  if (s_browse_level == 0) {
    empty = s_browse_project_count == 0;
    msg = s_browse_projects_loading ? "Loading projects..." : "No projects.";
  } else {
    empty = s_browse_task_count == 0;
    msg = s_browse_tasks_loading ? "Loading..." : "No tasks in this project.";
  }
  text_layer_set_text(s_browse_empty, msg);
  layer_set_hidden(text_layer_get_layer(s_browse_empty), !empty);
  layer_set_hidden(menu_layer_get_layer(s_browse_menu), empty);
}

// Back at level 1 returns to the (still-loaded) project list; at level 0 it
// pops the window. MenuLayer owns UP/DOWN/SELECT via its own click config;
// this wraps that provider to add BACK, the same pattern the main window uses
// for its long-press gestures.
static ClickConfigProvider s_browse_menu_ccp = NULL;

static void browse_back_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_browse_level == 1) {
    backlight_touch();
    free(s_browse_tasks);
    s_browse_tasks = NULL;
    s_browse_task_count = 0;
    s_browse_project_id[0] = '\0';
    s_browse_tasks_loading = false;
    s_browse_level = 0;
    // Entering the browser via a today-view project row jumps straight to
    // level 1 and does NOT fetch the list up front (two back-to-back
    // begin_send()s collide on the one outbox slot). Fetch it now, on the
    // first Back, if it isn't already in hand.
    if (s_browse_project_count == 0 && !s_browse_projects_loading) {
      s_browse_projects_loading = true;
      request_project_list();
    }
    menu_layer_reload_data(s_browse_menu);
    menu_layer_set_selected_index(s_browse_menu, MenuIndex(0, 0), MenuRowAlignTop, false);
    browse_update_empty();
    return;
  }
  window_stack_pop(true);
}

static void browse_menu_click_config_provider(void *context) {
  if (s_browse_menu_ccp) {
    s_browse_menu_ccp(context);
  }
  window_single_click_subscribe(BUTTON_ID_BACK, browse_back_click_handler);
}

static void browse_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  backlight_touch();

  s_browse_status_bar = status_bar_layer_create();
  layer_add_child(window_layer, status_bar_layer_get_layer(s_browse_status_bar));

  GRect content = GRect(bounds.origin.x, bounds.origin.y + STATUS_BAR_LAYER_HEIGHT,
                         bounds.size.w, bounds.size.h - STATUS_BAR_LAYER_HEIGHT);

  s_browse_menu = menu_layer_create(content);
  menu_layer_set_callbacks(s_browse_menu, NULL, (MenuLayerCallbacks) {
    .get_num_sections = browse_menu_get_num_sections,
    .get_num_rows = browse_menu_get_num_rows,
    .get_header_height = browse_menu_get_header_height,
    .draw_header = browse_menu_draw_header,
    .draw_row = browse_menu_draw_row,
    .select_click = browse_menu_select_click,
    .select_long_click = browse_menu_select_long_click,
  });
  menu_layer_set_click_config_onto_window(s_browse_menu, window);
  s_browse_menu_ccp = window_get_click_config_provider(window);
  window_set_click_config_provider_with_context(window, browse_menu_click_config_provider,
                                                window_get_click_config_context(window));
  layer_add_child(window_layer, menu_layer_get_layer(s_browse_menu));

  s_browse_empty = text_layer_create(content);
  text_layer_set_text_alignment(s_browse_empty, GTextAlignmentCenter);
  text_layer_set_font(s_browse_empty, fonts_get_system_font(EMPTY_MSG_FONT_KEY));
  layer_add_child(window_layer, text_layer_get_layer(s_browse_empty));

  browse_update_empty();
}

static void browse_window_unload(Window *window) {
  menu_layer_destroy(s_browse_menu);
  text_layer_destroy(s_browse_empty);
  status_bar_layer_destroy(s_browse_status_bar);
  s_browse_menu = NULL;
  s_browse_empty = NULL;
  s_browse_status_bar = NULL;
  s_browse_menu_ccp = NULL;
  free(s_browse_projects);
  s_browse_projects = NULL;
  free(s_browse_tasks);
  s_browse_tasks = NULL;
  s_browse_project_count = 0;
  s_browse_task_count = 0;
  s_browse_projects_loading = false;
  s_browse_tasks_loading = false;
  s_browse_project_id[0] = '\0';
  s_browse_level = 0;
}

// jump_to_project non-NULL opens straight at that project's tasks (level 1) -
// the today view's project row uses this. Back from there lands on the project
// list, fetched then (see browse_back_click_handler).
static void push_browse_window(const char *jump_to_project) {
  s_browse_level = 0;
  if (!s_browse_projects) {
    s_browse_projects = malloc(sizeof(BrowseProject) * MAX_BROWSE_PROJECTS);
  }
  s_browse_project_count = 0;
  s_browse_project_incoming = 0;
#if PROJECTS_CACHE
  load_browse_projects();  // instant render from the cache; the fetch below refreshes it
#endif
  bool jumping = jump_to_project && jump_to_project[0] != '\0';
  // Only one begin_send() may be in flight (single outbox slot). A jump sends
  // just the task fetch now; browse_back_click_handler fetches the list later.
  s_browse_projects_loading = !jumping && (s_browse_project_count == 0);
  if (!s_browse_window) {
    s_browse_window = window_create();
    window_set_window_handlers(s_browse_window, (WindowHandlers) {
      .load = browse_window_load,
      .unload = browse_window_unload,
    });
  }
  window_stack_push(s_browse_window, true);
  if (jumping) {
    browse_descend(jump_to_project);
  } else {
    request_project_list();
  }
}
#endif // PROJECTS_BROWSER

#ifndef PBL_PLATFORM_APLITE
// Select dismisses the notes window; Back also does, for free, via Pebble's
// default pop behavior.
static void notes_window_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  hide_notes_overlay();
}

// Long-select dictates text to append to the currently-shown task's notes
// - see start_note_append_dictation.
static void notes_window_select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  start_note_append_dictation();
}

#if defined(PBL_TOUCH)
// --- Touch navigation (emery and any other PBL_TOUCH platform) ---
//
// Hybrid model. The system touch-nav bridge (app_touch_navigation_enable)
// handles most of it: a swipe scrolls, a tap on a row moves the MenuLayer
// highlight there and is delivered as a SELECT single-click.
//
// This raw touch_service handler runs alongside the bridge and adds what the
// bridge can't:
//   - long-press -> the selected row's long-click (tracking, Finish Day,
//     habit -1, dictate a note); no point->row mapping needed (MenuLayer
//     has no API), it just acts on whatever is selected.
//   - left-swipe on the main list -> move the selected task to tomorrow
//     (see begin_pending_reschedule; the Down button long-press does the same).
//   - tap -> SELECT-ONLY: the bridge's synthesised SELECT click is swallowed
//     (arm_tap_select_guard) so a tap just selects the row, it never toggles
//     it done or bumps a habit. Those stay button-only actions.
// It does NOT handle swipe-right = Back (the bridge already does; a second pop
// double-popped out of the app) or vertical drags (the bridge's scrolling).
//
// The long-press acts on the MenuLayer's selected row, like the physical
// long-click - no need to map a touch point to a row (MenuLayer has no API).
//
// HARDWARE STATE (first-gen Time 2, firmware ~4.33): the touch driver reports a
// large drift toward screen centre on edge touches (~130-170px over ~200ms), so
// the bridge reads an edge tap as a swipe and the long-press slop trips.
// Centre taps and holds are clean. This is why the pairing setting defaults
// OFF - opt-in until a firmware fix lands. TOUCH_SLOP_PX is generous to ride
// out the smaller drift on a near-centre hold.

#define TOUCH_LONGPRESS_MS 700
#define TOUCH_SLOP_PX 25  // movement past this cancels the long-press / is not a tap
// A decisive leftward drag on the task list = move the selected task to
// tomorrow (the button long-press Down does the same). Right/vertical drags
// stay the bridge's (Back / scroll). Gated on a shallow dy so a diagonal
// scroll doesn't count. Kept modest (a real flick clears it in a frame or two)
// because the old 60px value plus the slop-disarm below meant a swipe that
// began even slightly vertical never registered.
#define TOUCH_SWIPE_PX 45

static AppTimer *s_touch_longpress_timer = NULL;
static GPoint s_touch_down_point;
static bool s_touch_armed = false;       // a gesture is in progress (armed at Touchdown)
static bool s_touch_moved = false;       // finger has left the tap zone (scroll / swipe)
static bool s_touch_swipe_fired = false; // left-swipe already handled this gesture

static void touch_longpress_timer_cancel(void) {
  if (s_touch_longpress_timer) {
    app_timer_cancel(s_touch_longpress_timer);
    s_touch_longpress_timer = NULL;
  }
}

// The guard window: any menu SELECT click that lands within this long of a
// touch event is the bridge's synthesised tap-click, not a physical button
// press. Generous because the bridge can emit the click slightly before OR
// after the raw Liftoff, and the Time-2 driver's latency is not tight.
#define TAP_GUARD_MS 450

static void tap_guard_timer_cb(void *data) {
  s_tap_guard_timer = NULL;
  s_ignore_next_menu_select = false;
}

// Arm / refresh the guard. Called on every touch event that could precede a
// synthesised SELECT (Touchdown, Liftoff, long-press fire) - NOT gated on a
// motion "was it a tap" test, which is unreliable given the driver drift the
// HARDWARE STATE note describes. A scroll / swipe / long-press just leaves the
// guard to expire unused.
static void arm_tap_select_guard(void) {
  s_ignore_next_menu_select = true;
  if (s_tap_guard_timer) {
    app_timer_cancel(s_tap_guard_timer);
  }
  s_tap_guard_timer = app_timer_register(TAP_GUARD_MS, tap_guard_timer_cb, NULL);
}

static void clear_tap_select_guard(void) {
  s_ignore_next_menu_select = false;
  if (s_tap_guard_timer) {
    app_timer_cancel(s_tap_guard_timer);
    s_tap_guard_timer = NULL;
  }
}

// True (and disarms) if a touch-synthesised SELECT is what's being handled -
// the caller should treat this click as "select the row only", not activate it.
static bool consume_tap_select_guard(void) {
  if (!s_ignore_next_menu_select) {
    return false;
  }
  clear_tap_select_guard();
  return true;
}

static void touch_longpress_fire(void *data) {
  s_touch_longpress_timer = NULL;
  s_touch_armed = false;  // consumed - the eventual liftoff does nothing more
  arm_tap_select_guard();  // swallow any SELECT the bridge emits on the release
  backlight_touch();
  vibes_short_pulse();  // the only "it registered" cue before the action lands
  Window *top = window_stack_get_top_window();
  if (top == s_notes_window) {
    start_note_append_dictation();
  } else if (top == s_habits_window && s_habits_menu_layer) {
    MenuIndex idx = menu_layer_get_selected_index(s_habits_menu_layer);
    habits_menu_select_long_click(s_habits_menu_layer, &idx, NULL);
  } else if (top == s_main_window) {
    MenuIndex idx = menu_layer_get_selected_index(s_menu_layer);
    menu_select_long_click(s_menu_layer, &idx, NULL);
  }
}

// Left-swipe test: a decisive leftward drag with a shallow vertical component,
// on the main list only.
static bool touch_is_left_swipe(int dx, int dy) {
  int adx = dx < 0 ? -dx : dx;
  int ady = dy < 0 ? -dy : dy;
  return dx <= -TOUCH_SWIPE_PX && ady * 2 <= adx;
}

static void touch_handler(const TouchEvent *event, void *context) {
  switch (event->type) {
  case TouchEvent_Touchdown:
    touch_longpress_timer_cancel();
    s_touch_moved = false;
    s_touch_swipe_fired = false;
    // non_navigational: contact without the watch being woken first - don't arm.
    s_touch_armed = !event->non_navigational;
    if (!s_touch_armed) {
      clear_tap_select_guard();
      break;
    }
    s_touch_down_point = GPoint(event->x, event->y);
    s_touch_longpress_timer = app_timer_register(TOUCH_LONGPRESS_MS, touch_longpress_fire, NULL);
    // Arm now - the bridge can synthesise its tap SELECT click before the raw
    // Liftoff even reaches us. Re-armed on Liftoff to cover a late one.
    arm_tap_select_guard();
    break;

  case TouchEvent_PositionUpdate: {
    if (!s_touch_armed || s_touch_swipe_fired) {
      break;
    }
    int dx = event->x - s_touch_down_point.x;
    int dy = event->y - s_touch_down_point.y;
    int adx = dx < 0 ? -dx : dx;
    int ady = dy < 0 ? -dy : dy;
    if (adx > TOUCH_SLOP_PX || ady > TOUCH_SLOP_PX) {
      s_touch_moved = true;
      touch_longpress_timer_cancel();  // a moving finger isn't a long-press
    }
    // Keep evaluating dx for the whole drag - the old code disarmed at the slop,
    // so a swipe that started even slightly vertical could never be recognised.
    if (touch_is_left_swipe(dx, dy) && window_stack_get_top_window() == s_main_window) {
      s_touch_swipe_fired = true;
      begin_pending_reschedule(RESCHEDULE_TOMORROW);
    }
    break;
  }

  case TouchEvent_Liftoff: {
    touch_longpress_timer_cancel();
    if (s_touch_armed && !s_touch_swipe_fired) {
      int dx = event->x - s_touch_down_point.x;
      int dy = event->y - s_touch_down_point.y;
      if (touch_is_left_swipe(dx, dy) && window_stack_get_top_window() == s_main_window) {
        // A fast flick can arrive as Touchdown -> Liftoff with no
        // PositionUpdate between - catch it from the endpoint delta.
        begin_pending_reschedule(RESCHEDULE_TOMORROW);
        s_touch_swipe_fired = true;
      }
    }
    // Re-arm the tap guard so a SELECT the bridge emits just after the release
    // is swallowed too (tap = select-only; and after a swipe, a stray SELECT
    // must not reach the pending-reschedule cancel path).
    arm_tap_select_guard();
    s_touch_armed = false;
    s_touch_moved = false;
    s_touch_swipe_fired = false;
    break;
  }
  }
}

static void apply_touch_nav(void) {
  app_touch_navigation_enable(s_touch_nav_enabled);
  if (s_touch_nav_enabled) {
    touch_service_subscribe(touch_handler, NULL);
  } else {
    touch_service_unsubscribe();
    touch_longpress_timer_cancel();
    clear_tap_select_guard();
    s_touch_armed = false;
    s_touch_moved = false;
    s_touch_swipe_fired = false;
  }
}
#endif  // PBL_TOUCH

// Installed onto the ScrollLayer (not the window) via
// scroll_layer_set_click_config_onto_window, which wires UP/DOWN to scrolling
// then calls this for SELECT. On touch builds the bridge also scrolls by finger.
static void notes_window_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, notes_window_select_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 0, notes_window_select_long_click_handler, NULL);
}

// Fills the layer with NOTES_TAGS_BG_COLOR and draws the bold "Tags:" label
// then the tag names below it. Two graphics_draw_text calls since one can't mix
// font weights. A no-op when the frame is zero-height (a project subject).
static void notes_tags_layer_draw(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, NOTES_TAGS_BG_COLOR);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  graphics_context_set_text_color(ctx, GColorBlack);

  int16_t content_w = bounds.size.w - NOTES_TAGS_PADDING_X * 2;
  int16_t label_h, names_h;
  measure_notes_tags_parts(content_w, &label_h, &names_h);

  GRect label_rect = GRect(NOTES_TAGS_PADDING_X, NOTES_TAGS_PADDING_Y, content_w, label_h);
  graphics_draw_text(ctx, NOTES_TAGS_LABEL, fonts_get_system_font(NOTES_LABEL_FONT_KEY), label_rect,
                      GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);

  GRect names_rect = GRect(NOTES_TAGS_PADDING_X, NOTES_TAGS_PADDING_Y + label_h, content_w, names_h);
  graphics_draw_text(ctx, notes_tags_display_line(), fonts_get_system_font(NOTES_BODY_FONT_KEY), names_rect,
                      GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
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
  s_notes_content_bounds = content_bounds;

  // Fixed header added to window_layer, not the ScrollLayer, so it stays put
  // while the body scrolls. Provisional zero-height frame -
  // render_notes_overlay_content sizes it and slices the ScrollLayer below it.
  s_notes_tags_layer = layer_create(GRect(content_bounds.origin.x, content_bounds.origin.y,
                                           content_bounds.size.w, 0));
  layer_set_update_proc(s_notes_tags_layer, notes_tags_layer_draw);
  layer_add_child(window_layer, s_notes_tags_layer);

  // Provisional full-content frame - render_notes_overlay_content shrinks it
  // to make room for the tags header.
  s_notes_scroll_layer = scroll_layer_create(content_bounds);
  scroll_layer_set_content_size(s_notes_scroll_layer, content_bounds.size);
  scroll_layer_set_click_config_onto_window(s_notes_scroll_layer, window);
  scroll_layer_set_callbacks(s_notes_scroll_layer, (ScrollLayerCallbacks) {
    .click_config_provider = notes_window_click_config_provider,
  });

  // Left-aligned, top-anchored body text (unlike s_error_layer's centered
  // status text), smaller font. Created at the viewport size, resized to the
  // real content height by render_notes_overlay_content.
  s_notes_layer = text_layer_create(GRect(0, 0, content_bounds.size.w, content_bounds.size.h));
  text_layer_set_text_alignment(s_notes_layer, GTextAlignmentLeft);
  text_layer_set_font(s_notes_layer, fonts_get_system_font(NOTES_BODY_FONT_KEY));
  text_layer_set_background_color(s_notes_layer, GColorWhite);
  text_layer_set_text_color(s_notes_layer, GColorBlack);
  text_layer_set_overflow_mode(s_notes_layer, GTextOverflowModeWordWrap);
  scroll_layer_add_child(s_notes_scroll_layer, text_layer_get_layer(s_notes_layer));

  layer_add_child(window_layer, scroll_layer_get_layer(s_notes_scroll_layer));

  // s_notes_display_text was set by show_notes_overlay() before the push
  // (usually the loading placeholder - the fetch is still in flight).
  render_notes_overlay_content();
}

static void notes_window_unload(Window *window) {
  layer_destroy(s_notes_tags_layer);
  s_notes_tags_layer = NULL;
  text_layer_destroy(s_notes_layer);
  s_notes_layer = NULL;
  scroll_layer_destroy(s_notes_scroll_layer);
  s_notes_scroll_layer = NULL;
  status_bar_layer_destroy(s_notes_status_bar);
  reset_notes_full_buffer();
  cancel_notes_load_timeout();
  // Cleared here, not in hide_notes_overlay(), so a Back-triggered dismissal
  // (which never calls hide_notes_overlay) clears it the same way.
  s_notes_overlay_active = false;
}

// Created once and reused, like push_habits_window - only its layers are
// rebuilt each visit.
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

// ---------- live tracking window ----------

#ifndef PBL_PLATFORM_APLITE
static void live_tick_callback(void *data);

static void stop_live_tick(void) {
  if (s_live_tick_timer) {
    app_timer_cancel(s_live_tick_timer);
    s_live_tick_timer = NULL;
  }
}

// Rewrites the live window's text from the current s_presence_* state, and
// (re)arms the 1s elapsed tick only while the session is actively tracking.
// A no-op unless the live window is the top window. State 0 pops it.
static void live_window_refresh(void) {
  if (!s_live_window || window_stack_get_top_window() != s_live_window) {
    return;
  }
  if (s_presence_state == 0) {
    stop_live_tick();
    window_stack_pop(true);
    return;
  }

  text_layer_set_text(s_live_state_layer, presence_state_phrase());
  text_layer_set_text(s_live_task_layer, s_presence_task);

  static char elapsed_buf[16];
  if (s_presence_state == 1) {
    int total_s = (int)(time(NULL) - s_presence_elapsed_base);
    if (total_s < 0) {
      total_s = 0;
    }
    int h = total_s / 3600;
    int m = (total_s % 3600) / 60;
    int s = total_s % 60;
    if (h > 0) {
      snprintf(elapsed_buf, sizeof(elapsed_buf), "%d:%02d:%02d", h, m, s);
    } else {
      snprintf(elapsed_buf, sizeof(elapsed_buf), "%d:%02d", m, s);
    }
    text_layer_set_text(s_live_elapsed_layer, elapsed_buf);
    layer_set_hidden(text_layer_get_layer(s_live_elapsed_layer), false);
    if (!s_live_tick_timer) {
      s_live_tick_timer = app_timer_register(TRACKING_TICK_INTERVAL_MS, live_tick_callback, NULL);
    }
  } else {
    layer_set_hidden(text_layer_get_layer(s_live_elapsed_layer), true);
    stop_live_tick();
  }

  const char *hint = "";
  if (s_presence_stopping) {
    hint = "Stopping...";
  } else if (s_presence_can_stop) {
    hint = "Select to stop";
  }
  text_layer_set_text(s_live_hint_layer, hint);
}

static void live_tick_callback(void *data) {
  s_live_tick_timer = NULL;
  if (s_presence_state == 1) {
    live_window_refresh(); // re-arms the timer, or stops if the window closed
  }
}

static void live_window_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_presence_can_stop && !s_presence_stopping) {
    s_presence_stopping = true;
    send_presence_stop();
    live_window_refresh(); // show "Stopping..."; the phone clears us on its ack
  }
}

static void live_window_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, live_window_select_click_handler);
}

static void live_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  backlight_touch();

  const int16_t status_bar_height = STATUS_BAR_LAYER_HEIGHT;
  s_live_status_bar = status_bar_layer_create();
  layer_add_child(window_layer, status_bar_layer_get_layer(s_live_status_bar));

  int16_t x = bounds.origin.x + 6;
  int16_t w = bounds.size.w - 12;
  int16_t y = bounds.origin.y + status_bar_height + 6;

  // Task name first (what the user asked to see), wrapping to two lines.
  s_live_task_layer = text_layer_create(GRect(x, y, w, 50));
  text_layer_set_font(s_live_task_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_live_task_layer, GTextAlignmentCenter);
  text_layer_set_overflow_mode(s_live_task_layer, GTextOverflowModeWordWrap);
  layer_add_child(window_layer, text_layer_get_layer(s_live_task_layer));
  y += 54;

  // Then the state line ("Tracking on Desktop" / "Stopped on Desktop"), smaller.
  s_live_state_layer = text_layer_create(GRect(x, y, w, 22));
  text_layer_set_font(s_live_state_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_alignment(s_live_state_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_live_state_layer));
  y += 26;

  s_live_elapsed_layer = text_layer_create(GRect(x, y, w, 30));
  text_layer_set_font(s_live_elapsed_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(s_live_elapsed_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_live_elapsed_layer));

  s_live_hint_layer = text_layer_create(GRect(x, bounds.size.h - 20, w, 18));
  text_layer_set_font(s_live_hint_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_live_hint_layer, GTextAlignmentCenter);
  text_layer_set_text_color(s_live_hint_layer, GColorDarkGray);
  layer_add_child(window_layer, text_layer_get_layer(s_live_hint_layer));

  window_set_click_config_provider(window, live_window_click_config_provider);
  live_window_refresh();
}

static void live_window_unload(Window *window) {
  stop_live_tick();
  text_layer_destroy(s_live_state_layer);
  text_layer_destroy(s_live_task_layer);
  text_layer_destroy(s_live_elapsed_layer);
  text_layer_destroy(s_live_hint_layer);
  status_bar_layer_destroy(s_live_status_bar);
  s_live_state_layer = NULL;
  s_live_task_layer = NULL;
  s_live_elapsed_layer = NULL;
  s_live_hint_layer = NULL;
  s_live_status_bar = NULL;
}

// Created once, reused (only its layers are rebuilt each visit) - matches
// push_habits_window / push_notes_window.
static void push_live_window(void) {
  if (s_presence_state == 0) {
    return;
  }
  if (!s_live_window) {
    s_live_window = window_create();
    window_set_window_handlers(s_live_window, (WindowHandlers) {
      .load = live_window_load,
      .unload = live_window_unload,
    });
  }
  window_stack_push(s_live_window, true);
}
#endif

// ---------- window lifecycle ----------

#ifndef PBL_PLATFORM_APLITE
// MenuLayer owns the main window's click config (UP/DOWN scroll, SELECT single
// and long). menu_layer_set_click_config_onto_window installs its provider on
// the window; this wraps that provider to add a long-press on UP (unschedule)
// and DOWN (move to tomorrow) without disturbing anything else - see
// begin_pending_reschedule. The wrapped provider is called with MenuLayer's own
// context (the MenuLayer*), which its handlers require.
static ClickConfigProvider s_menu_click_config_provider = NULL;
static void *s_menu_click_config_context = NULL;

static void reschedule_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  backlight_touch();
  begin_pending_reschedule(click_recognizer_get_button_id(recognizer) == BUTTON_ID_UP
                               ? RESCHEDULE_UNSCHEDULE
                               : RESCHEDULE_TOMORROW);
}

static void main_window_click_config_provider(void *context) {
  if (s_menu_click_config_provider) {
    s_menu_click_config_provider(context);
  }
  window_long_click_subscribe(BUTTON_ID_UP, RESCHEDULE_LONGPRESS_MS,
                              reschedule_long_click_handler, NULL);
  window_long_click_subscribe(BUTTON_ID_DOWN, RESCHEDULE_LONGPRESS_MS,
                              reschedule_long_click_handler, NULL);
}
#endif

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
#ifndef PBL_PLATFORM_APLITE
  // Wrap MenuLayer's just-installed provider to add the long-press UP/DOWN
  // reschedule gestures (see main_window_click_config_provider).
  s_menu_click_config_provider = window_get_click_config_provider(window);
  s_menu_click_config_context = window_get_click_config_context(window);
  window_set_click_config_provider_with_context(window, main_window_click_config_provider,
                                                s_menu_click_config_context);
#endif
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
  // A cached list may already have a selection that needs to scroll.
  refresh_scroll_state(true);

  // Logo in a fixed strip at the bottom of the empty-state area; the text layer
  // gets the space above it.
  #define LOGO_SIZE 50
  #define LOGO_STRIP_HEIGHT 58
  GRect empty_text_bounds = GRect(content_bounds.origin.x, content_bounds.origin.y,
                                   content_bounds.size.w, content_bounds.size.h - LOGO_STRIP_HEIGHT);
  s_empty_layer = text_layer_create(empty_text_bounds);
  text_layer_set_text_alignment(s_empty_layer, GTextAlignmentCenter);
  text_layer_set_font(s_empty_layer, fonts_get_system_font(EMPTY_MSG_FONT_KEY));
  layer_add_child(window_layer, text_layer_get_layer(s_empty_layer));

#ifndef PBL_PLATFORM_APLITE
  // Bottom slice of the text area for s_sync_progress_layer, sized off
  // empty_text_bounds. Hidden except during the initial sync.
  #ifdef PBL_PLATFORM_EMERY
  #define SYNC_PROGRESS_HEIGHT 42
  #else
  #define SYNC_PROGRESS_HEIGHT 36
  #endif
  GRect sync_progress_bounds = GRect(empty_text_bounds.origin.x,
                                      empty_text_bounds.origin.y + empty_text_bounds.size.h - SYNC_PROGRESS_HEIGHT,
                                      empty_text_bounds.size.w, SYNC_PROGRESS_HEIGHT);
  s_sync_progress_layer = text_layer_create(sync_progress_bounds);
  text_layer_set_text_alignment(s_sync_progress_layer, GTextAlignmentCenter);
  text_layer_set_font(s_sync_progress_layer, fonts_get_system_font(CHROME_FONT_KEY));
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

  // Row icons for the Resync/Habits rows - loaded once, not per-draw.
  s_check_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MENU_ICON);
  s_check_white_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CHECK_WHITE);
  s_heart_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HEART_CHECK);
  s_heart_white_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HEART_CHECK_WHITE);

  // Add Task row + dictation session - mic platforms only.
#ifndef PBL_PLATFORM_APLITE
  s_mic_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MICROPHONE);
  s_mic_white_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MICROPHONE_WHITE);
  s_dictation_session = dictation_session_create(MAX_TITLE_LEN, dictation_status_callback, NULL);
  // Let the user review/retry the transcription before it's sent.
  dictation_session_enable_confirmation(s_dictation_session, true);
#endif

  // Full content_bounds, added last so it draws on top of every other layer.
  // Hidden by default; only show_error_overlay() reveals it.
  s_error_layer = text_layer_create(content_bounds);
  text_layer_set_text_alignment(s_error_layer, GTextAlignmentCenter);
  text_layer_set_font(s_error_layer, fonts_get_system_font(TITLE_FONT_KEY));
  text_layer_set_background_color(s_error_layer, GColorRed);
  // White, not black - better contrast on red, and on aplite GColorRed may
  // reduce to black, which would make black text invisible.
  text_layer_set_text_color(s_error_layer, GColorWhite);
  text_layer_set_overflow_mode(s_error_layer, GTextOverflowModeWordWrap);
  layer_set_hidden(text_layer_get_layer(s_error_layer), true);
  layer_add_child(window_layer, text_layer_get_layer(s_error_layer));

#ifndef PBL_PLATFORM_APLITE
  // Over-estimate banner - a red strip across the top of the list, same
  // GColorRed + white-bold as s_error_layer. Hidden until a tracked task first
  // crosses its estimate.
  GRect overtime_bounds = GRect(content_bounds.origin.x, content_bounds.origin.y,
                                 content_bounds.size.w, OVERTIME_BANNER_HEIGHT);
  s_overtime_banner_layer = text_layer_create(overtime_bounds);
  text_layer_set_text_alignment(s_overtime_banner_layer, GTextAlignmentCenter);
  text_layer_set_font(s_overtime_banner_layer, fonts_get_system_font(TITLE_FONT_KEY));
  text_layer_set_background_color(s_overtime_banner_layer, GColorRed);
  text_layer_set_text_color(s_overtime_banner_layer, GColorWhite);
  text_layer_set_overflow_mode(s_overtime_banner_layer, GTextOverflowModeTrailingEllipsis);
  layer_set_hidden(text_layer_get_layer(s_overtime_banner_layer), true);
  layer_add_child(window_layer, text_layer_get_layer(s_overtime_banner_layer));
#endif

  update_empty_layer();
  request_sync();

  // Resume the live-ticking redraw if a session was running at last close (the
  // elapsed total comes from the persisted start timestamp).
  if (s_tracking_task_id[0] != '\0') {
    start_tracking_tick();
#ifndef PBL_PLATFORM_APLITE
    // Re-announce the resumed session so the phone can broadcast it again.
    time_t resumed_s = time(NULL) - s_tracking_start_epoch;
    send_track_time_start(s_tracking_task_id, resumed_s > 0 ? (int32_t)resumed_s * 1000 : 0);
#endif
  }
}

static void window_unload(Window *window) {
  stop_scroll_timer();
  // NOT stopping tracking here - only this window's redraw timer, since
  // s_menu_layer is about to be destroyed.
  stop_tracking_tick();
#ifndef PBL_PLATFORM_APLITE
  hide_overtime_banner(); // cancels its auto-dismiss timer
  cancel_unpin_timer();   // the pinned-section grace timer
#endif
  stop_syncing_animation();
  menu_layer_destroy(s_menu_layer);
  text_layer_destroy(s_empty_layer);
#ifndef PBL_PLATFORM_APLITE
  text_layer_destroy(s_sync_progress_layer);
  text_layer_destroy(s_overtime_banner_layer);
  s_overtime_banner_layer = NULL;
#endif
  text_layer_destroy(s_error_layer);
#ifndef PBL_PLATFORM_APLITE
  if (s_pending_toggle_timer) {
    app_timer_cancel(s_pending_toggle_timer);
    s_pending_toggle_timer = NULL;
  }
  if (s_pending_reschedule_timer) {
    app_timer_cancel(s_pending_reschedule_timer);
    s_pending_reschedule_timer = NULL;
  }
  s_pending_reschedule_kind = RESCHEDULE_NONE;
  s_pending_reschedule_task_id[0] = '\0';
  s_menu_click_config_provider = NULL;
  s_menu_click_config_context = NULL;
#endif
#if defined(PBL_TOUCH)
  clear_tap_select_guard();
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
  // Set the starting status through set_status_code() (not its static
  // initializer - see s_status_code's comment) so this first "Syncing..."
  // stretch goes through the same chokepoint as every later status.
  set_status_code(STATUS_SYNCING);
#ifndef PBL_PLATFORM_APLITE
  // Whether this session exists because a wakeup fired vs the user opening the app.
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

#ifndef PBL_PLATFORM_APLITE
  // Re-pin a resumed tracking session (the pin setting isn't known until the
  // first sync; the section appears then). A stopped-but-in-grace task is NOT
  // re-pinned - the grace period isn't persisted state.
  if (s_tracking_task_id[0] != '\0') {
    strncpy(s_pinned_task_id, s_tracking_task_id, MAX_ID_LEN - 1);
    s_pinned_task_id[MAX_ID_LEN - 1] = '\0';
  }
#endif

#ifndef PBL_PLATFORM_APLITE
  // If a resumed session is already past its estimate at open, latch
  // s_overtime_notified so the first tick doesn't fire the banner - only a
  // crossing while the app is open should notify. maybe_notify_overtime re-arms
  // it if effective time later dips back under.
  if (s_tracking_task_id[0] != '\0') {
    Task *resumed = find_task_by_id(s_tracking_task_id);
    if (resumed && resumed->time_estimate_ms > 0) {
      int effective_ms = resumed->time_spent_ms;
      time_t elapsed_s = time(NULL) - s_tracking_start_epoch;
      if (elapsed_s > 0) {
        effective_ms += (int)elapsed_s * 1000;
      }
      if (effective_ms >= resumed->time_estimate_ms) {
        s_overtime_notified = true;
        // Count any "repeat every 5 minutes" interval from launch, not from the
        // first tick.
        s_overtime_last_notify_epoch = time(NULL);
      }
    }
  }
#endif

  app_message_register_inbox_received(inbox_received_handler);
  app_message_register_inbox_dropped(inbox_dropped_handler);
  app_message_register_outbox_failed(outbox_failed_handler);
#ifndef PBL_PLATFORM_APLITE
  app_message_register_outbox_sent(outbox_sent_handler);
#endif
  // Not app_message_*_size_maximum() (~8 KB each): two maxed buffers eat ~16 KB
  // of the ~25 KB heap, leaving too little for the Habits window's layers.
  // Largest messages are MSG_TASK_ITEM (~500 B) and MSG_NOTE_CHUNK (~1 KB with
  // UTF-8); 2 KB in / 1 KB out clears both.
  app_message_open(2048, 1024);

  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_main_window, true);

#if defined(PBL_TOUCH)
  // Apply the default (off); the first sync turns it on if the phone says so.
  apply_touch_nav();
#endif
}

static void deinit(void) {
#if defined(PBL_TOUCH)
  s_touch_nav_enabled = false;
  apply_touch_nav();  // unsubscribe + cancel the long-press timer
#endif
#ifndef PBL_PLATFORM_APLITE
  // Relinquish the backlight to automatic control before exiting - otherwise an
  // always-on or mid-timeout override persists past the app. Unconditional
  // (not gated on s_backlight_mode) since set_status_code() can also force it
  // on for a mid-sync exit. Harmless if the app never touched the backlight.
  if (s_backlight_timer) {
    app_timer_cancel(s_backlight_timer);
  }
  light_enable(false);
#endif
  window_destroy(s_main_window);
  if (s_habits_window) {
    window_destroy(s_habits_window);
  }
#ifndef PBL_PLATFORM_APLITE
  if (s_notes_window) {
    window_destroy(s_notes_window);
  }
  stop_live_tick();
  if (s_live_window) {
    window_destroy(s_live_window);
  }
#endif
#if PROJECTS_BROWSER
  if (s_browse_window) {
    window_destroy(s_browse_window);
  }
#endif
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
