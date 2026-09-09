#include <pebble.h>

// Keep in sync by hand with package.json "version" on every bump - no runtime
// API exposes it to C.
#define APP_VERSION "0.6.48"

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
#define KEY_TASK_REMIND_MIN MESSAGE_KEY_TASK_REMIND_MIN
#define KEY_TASK_ISSUE_KEY MESSAGE_KEY_TASK_ISSUE_KEY
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
#define KEY_HABIT_STREAK MESSAGE_KEY_HABIT_STREAK
#define KEY_HABIT_BEST_STREAK MESSAGE_KEY_HABIT_BEST_STREAK
#define KEY_HABIT_STREAK_NUDGE MESSAGE_KEY_HABIT_STREAK_NUDGE
#define KEY_CHECK_INDEX MESSAGE_KEY_CHECK_INDEX
#define KEY_CHECK_VALUE MESSAGE_KEY_CHECK_VALUE
#define KEY_METRIC_ENERGY MESSAGE_KEY_METRIC_ENERGY
#define KEY_METRIC_RATING MESSAGE_KEY_METRIC_RATING
#define KEY_METRIC_REFLECT_TEXT MESSAGE_KEY_METRIC_REFLECT_TEXT
#define KEY_REFLECT_ENABLED MESSAGE_KEY_REFLECT_ENABLED
#define KEY_HABITS_ENABLED MESSAGE_KEY_HABITS_ENABLED
#define KEY_ADD_TASK_ENABLED MESSAGE_KEY_ADD_TASK_ENABLED
#define KEY_BACKLIGHT_MODE MESSAGE_KEY_BACKLIGHT_MODE
#define KEY_NOTE_TEXT MESSAGE_KEY_NOTE_TEXT
#define KEY_NOTE_TOTAL_LEN MESSAGE_KEY_NOTE_TOTAL_LEN
#define KEY_NOTE_CHUNK_TEXT MESSAGE_KEY_NOTE_CHUNK_TEXT
#define KEY_TASK_PROJECT_ID MESSAGE_KEY_TASK_PROJECT_ID
#define KEY_TASK_PROJECT_COLOR MESSAGE_KEY_TASK_PROJECT_COLOR
#define KEY_TASK_DEADLINE_DAYS MESSAGE_KEY_TASK_DEADLINE_DAYS
#define KEY_TASK_RECURS MESSAGE_KEY_TASK_RECURS
#define KEY_PROJECT_ID MESSAGE_KEY_PROJECT_ID
#define KEY_PROJECT_INDEX MESSAGE_KEY_PROJECT_INDEX
#define KEY_PROJECT_TITLE MESSAGE_KEY_PROJECT_TITLE
#define KEY_PROJECT_COLOR MESSAGE_KEY_PROJECT_COLOR
#define KEY_PROJECT_TASK_COUNT MESSAGE_KEY_PROJECT_TASK_COUNT
#define KEY_PROJECT_TOTAL MESSAGE_KEY_PROJECT_TOTAL
#define KEY_PROJECT_TASK_BACKLOG MESSAGE_KEY_PROJECT_TASK_BACKLOG
#define KEY_PROJECTS_ENABLED MESSAGE_KEY_PROJECTS_ENABLED
#define KEY_TAGS_ENABLED MESSAGE_KEY_TAGS_ENABLED
#define KEY_IS_TAGS MESSAGE_KEY_IS_TAGS
#define KEY_TASK_TAGS MESSAGE_KEY_TASK_TAGS
#define KEY_TOUCH_NAV_ENABLED MESSAGE_KEY_TOUCH_NAV_ENABLED
#define KEY_OVERTIME_NOTIFY_ENABLED MESSAGE_KEY_OVERTIME_NOTIFY_ENABLED
#define KEY_OVERTIME_REPEAT_ENABLED MESSAGE_KEY_OVERTIME_REPEAT_ENABLED
#define KEY_AUDIBLE_NOTIFICATIONS MESSAGE_KEY_AUDIBLE_NOTIFICATIONS
#define KEY_AUDIBLE_VOLUME MESSAGE_KEY_AUDIBLE_VOLUME
#define KEY_BREAK_REMINDER_MIN MESSAGE_KEY_BREAK_REMINDER_MIN
#define KEY_IDLE_REMINDER_MIN MESSAGE_KEY_IDLE_REMINDER_MIN
#define KEY_DUE_REMINDER_MIN MESSAGE_KEY_DUE_REMINDER_MIN
#define KEY_FOCUS_LEN_MIN MESSAGE_KEY_FOCUS_LEN_MIN
#define KEY_POMODORO_WORK_MIN MESSAGE_KEY_POMODORO_WORK_MIN
#define KEY_POMODORO_BREAK_MIN MESSAGE_KEY_POMODORO_BREAK_MIN
#define KEY_USE_POMODORO_CFG MESSAGE_KEY_USE_POMODORO_CFG
#define KEY_STOP_AT_MIDNIGHT MESSAGE_KEY_STOP_AT_MIDNIGHT
#define KEY_PRESENCE_STATE MESSAGE_KEY_PRESENCE_STATE
#define KEY_PRESENCE_TASK_TITLE MESSAGE_KEY_PRESENCE_TASK_TITLE
#define KEY_PRESENCE_DEVICE MESSAGE_KEY_PRESENCE_DEVICE
#define KEY_PRESENCE_ELAPSED_S MESSAGE_KEY_PRESENCE_ELAPSED_S
#define KEY_PRESENCE_CAN_STOP MESSAGE_KEY_PRESENCE_CAN_STOP
#define KEY_PRESENCE_SPENT_MS MESSAGE_KEY_PRESENCE_SPENT_MS
#define KEY_PRESENCE_ESTIMATE_MS MESSAGE_KEY_PRESENCE_ESTIMATE_MS
#define KEY_PRESENCE_NEW_SESSION MESSAGE_KEY_PRESENCE_NEW_SESSION
#define KEY_STATS_ENABLED MESSAGE_KEY_STATS_ENABLED
#define KEY_STATS_EST_REMAINING_MS MESSAGE_KEY_STATS_EST_REMAINING_MS
#define KEY_STATS_WORKED_TODAY_MS MESSAGE_KEY_STATS_WORKED_TODAY_MS
#define KEY_STATS_DONE_TODAY MESSAGE_KEY_STATS_DONE_TODAY
#define KEY_STATS_TEXT MESSAGE_KEY_STATS_TEXT
#define KEY_STATS_WORKED_YESTERDAY_MS MESSAGE_KEY_STATS_WORKED_YESTERDAY_MS
#define KEY_STATS_DONE_YESTERDAY MESSAGE_KEY_STATS_DONE_YESTERDAY
#define KEY_YESTERDAY_STATS_ENABLED MESSAGE_KEY_YESTERDAY_STATS_ENABLED
#define KEY_SCHEDULE_ENABLED MESSAGE_KEY_SCHEDULE_ENABLED
#define KEY_UPCOMING_ENABLED MESSAGE_KEY_UPCOMING_ENABLED
#define KEY_UPCOMING_TEXT MESSAGE_KEY_UPCOMING_TEXT
#define KEY_NOTESPAGE_ENABLED MESSAGE_KEY_NOTESPAGE_ENABLED
#define KEY_NOTESPAGE_TEXT MESSAGE_KEY_NOTESPAGE_TEXT
#define KEY_TASK_REPEAT_TEXT MESSAGE_KEY_TASK_REPEAT_TEXT
#define KEY_TASK_REPEAT_PAUSED MESSAGE_KEY_TASK_REPEAT_PAUSED

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
  // IS_TAGS on the two *_REQUEST messages makes this whole path serve the
  // optional Tags page instead: LIST returns each tag (PROJECT_ID = tag id,
  // PROJECT_TASK_COUNT = open-task count), TASKS returns that tag's tasks from
  // every project (all PROJECT_TASK_BACKLOG = 0). See s_browse_mode.
  MSG_PROJECT_LIST_REQUEST = 31,    // watch -> phone: IS_TAGS
  MSG_PROJECT_LIST_START = 32,      // phone -> watch: PROJECT_TOTAL + IS_TAGS
  MSG_PROJECT_LIST_ITEM = 33,       // phone -> watch: PROJECT_INDEX + PROJECT_ID + PROJECT_TITLE + PROJECT_TASK_COUNT
  MSG_PROJECT_LIST_END = 34,        // phone -> watch: (no keys)
  MSG_PROJECT_TASKS_REQUEST = 35,   // watch -> phone: PROJECT_ID + IS_TAGS
  MSG_PROJECT_TASKS_START = 36,     // phone -> watch: PROJECT_ID + TASK_TOTAL
  MSG_PROJECT_TASKS_ITEM = 37,      // phone -> watch: PROJECT_ID + TASK_INDEX + TASK_* + PROJECT_TASK_BACKLOG
  MSG_PROJECT_TASKS_END = 38,       // phone -> watch: PROJECT_ID
  MSG_TASK_PLAN_TODAY = 39,         // watch -> phone: TASK_ID (set the task's dueDay to today)
  // Stats page (config.enableStats, non-aplite). One request, one reply: the
  // two headline durations as ms plus STATS_TEXT (the project list
  // preformatted as "Title\tcount" lines the watch prints verbatim).
  MSG_STATS_REQUEST = 40,           // watch -> phone: (no keys)
  MSG_STATS_DATA = 41,             // phone -> watch: STATS_EST_REMAINING_MS + STATS_WORKED_TODAY_MS + STATS_TEXT
  // Upcoming page (config.enableUpcoming, non-aplite). One request, one reply:
  // UPCOMING_TEXT - preformatted lines, each day's tasks under a "\x02" header.
  MSG_UPCOMING_REQUEST = 42,        // watch -> phone: (no keys)
  MSG_UPCOMING_DATA = 43,           // phone -> watch: UPCOMING_TEXT
  // Projects browser: long-Select on a task row moves it between that
  // project's regular list and its backlog. PROJECT_TASK_BACKLOG 1 = into
  // the backlog, 0 = back to the regular list. Direction is chosen watch-
  // side from which section the row is in.
  MSG_TASK_SET_BACKLOG = 44,        // watch -> phone: TASK_ID + PROJECT_ID + PROJECT_TASK_BACKLOG
  // Set a task's time estimate (the estimate picker, reached by long-Up on the
  // notes overlay). 0 clears it. Replays as a plain [Task Shared] updateTask.
  MSG_TASK_SET_ESTIMATE = 45,       // watch -> phone: TASK_ID + TASK_TIME_ESTIMATE_MS
  // Set a task's deadline (long-Down on the notes overlay). The value is days
  // from today; -1 clears it. Phone turns it into { deadlineDay } (or nulls).
  MSG_TASK_SET_DEADLINE = 46,       // watch -> phone: TASK_ID + TASK_DEADLINE_DAYS
  // Add / remove a tag on a task (the tag editor, BROWSE_TAG_EDIT). PROJECT_ID
  // is the tag id, PROJECT_TASK_BACKLOG is 1 to add / 0 to remove. Replays as
  // a plain [Task Shared] updateTask { tagIds }.
  MSG_TASK_TOGGLE_TAG = 47,         // watch -> phone: TASK_ID + PROJECT_ID + PROJECT_TASK_BACKLOG
  // "Schedule at <hour>" from the action menu. TASK_DUE_MIN carries the hour as
  // minutes since midnight; the phone sets dueWithTime to the next occurrence
  // of that time (today if still future, else tomorrow).
  MSG_TASK_SET_DUE_TIME = 48,       // watch -> phone: TASK_ID + TASK_DUE_MIN
  // Move a task (+ its subtasks) to another project - the "Move to project" row
  // opens a project picker, Select sends this. Replays as [Task Shared]
  // moveToOtherProject.
  MSG_TASK_MOVE_PROJECT = 49,       // watch -> phone: TASK_ID + PROJECT_ID (target)
  // "Wipe watch cache" from the pairing page's danger zone. No keys - drop the
  // persisted task/habit/project-list blobs and pull a fresh list.
  MSG_WIPE_CACHE = 50,              // phone -> watch: (no keys)
  // Toggle the CHECK_INDEX-th "- [ ]" / "- [x]" checklist line in a task's
  // notes markdown. Opened from the notes window (Select) when it has any.
  MSG_TASK_TOGGLE_CHECK = 51,       // watch -> phone: TASK_ID + CHECK_INDEX + CHECK_VALUE
  // The Reflect window (Select on Finish Day) - today's metric fields.
  MSG_METRIC_ENERGY = 52,           // watch -> phone: METRIC_ENERGY (1 low / 2 ok / 3 good)
  MSG_METRIC_RATING = 53,           // watch -> phone: METRIC_RATING (impactOfWork 1-4)
  MSG_METRIC_REFLECT = 54,          // watch -> phone: METRIC_REFLECT_TEXT (dictated improvement)
  // Notes page (today-pinned standalone notes) - shares the Upcoming window.
  MSG_NOTESPAGE_REQUEST = 55,       // watch -> phone: (no keys)
  MSG_NOTESPAGE_DATA = 56,          // phone -> watch: NOTESPAGE_TEXT
  // Action menu "Repeat" row: the pattern text (fetched on menu open) and a
  // pause toggle.
  MSG_TASK_REPEAT_REQUEST = 57,     // watch -> phone: TASK_ID
  MSG_TASK_REPEAT_DATA = 58,        // phone -> watch: TASK_ID + TASK_REPEAT_TEXT + TASK_REPEAT_PAUSED
  MSG_TASK_REPEAT_PAUSE = 59,       // watch -> phone: TASK_ID + TASK_REPEAT_PAUSED
  // Desktop Pomodoro timing (globalConfig.pomodoro) for the watch's focus mode,
  // pushed once per sync. Only used when config.usePomodoroCfg is on.
  MSG_POMODORO_CFG = 60,            // phone -> watch: POMODORO_WORK_MIN + POMODORO_BREAK_MIN
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
// ceiling. On emery the big list arrays (s_tasks, s_incoming, s_habits,
// s_groups) are heap-allocated in init() via alloc_heap_lists() rather than
// living in .bss, so a slot costs runtime heap - of which emery has ~90 KB
// free - not the scarce virtual-size budget. That's what lets emery carry a
// 50-task list again (the cap fell 50 -> 40 -> 36 only while those arrays were
// static). Only s_schedule_order (int per task) stays in .bss on emery, at a
// trivial 200 B. Other platforms keep static arrays and a lower cap.
#ifdef PBL_PLATFORM_EMERY
#define MAX_TASKS 50
#else
#define MAX_TASKS 30
#endif

// On emery the list arrays move out of .bss onto the heap (alloc_heap_lists(),
// called first thing in init()) to stay under the 64 KB virtual-size ceiling.
// Elsewhere they stay plain static arrays.
#ifdef PBL_PLATFORM_EMERY
#define HEAP_BACKED_LISTS 1
#else
#define HEAP_BACKED_LISTS 0
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
// The phone's synthetic id for the "No Project" bucket (task-store.js
// NO_PROJECT_ID). It has no backlog, so the move-to-backlog gesture skips it.
#define NO_PROJECT_ID_STR "__NO_PROJECT__"
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
  // Issue-tracker badge: key + optional story points + "!" when the upstream
  // issue changed ("PROJ-123 3p!" / "#42"). '' when not linked to an issue.
  // Drawn at the start of the subtitle line.
  char issue_key[22];
#endif
  bool done;
  bool recurs; // has a repeat config - draws a small ↻ glyph on the row
#ifndef PBL_PLATFORM_APLITE
  // The task's own reminder fired this app-open session (minute_tick_handler).
  // Not preserved across a sync - a still-due reminder re-fires once after one.
  bool remind_fired;
#endif
#if TODAY_PROJECT_SWATCH
  // Packed GColor8 byte for this task's project's theme-colour swatch (0 =
  // none). Sits in the padding after `done`, costing the double-buffered
  // s_tasks/s_incoming arrays nothing. The grouped today view reads it off
  // whichever task starts each project group.
  uint8_t project_color;
#endif
  int due_min;       // minutes since local midnight, or -1 when the task has no dueWithTime
#ifndef PBL_PLATFORM_APLITE
  // task.remindAt as minutes since local midnight (only sent when it's today),
  // or -1. When set, it supersedes the global "notify before due" lead.
  int remind_min;
#endif
  int time_spent_ms; // total tracked time (all days, all devices), 0 if none
  int time_estimate_ms; // 0 if none
  // Days from today to the task's deadlineDay (negative = overdue, 0 = today),
  // or DEADLINE_NONE when it has none. The phone omits the key when absent.
  int deadline_days;
} Task;
#define DEADLINE_NONE 0x40000000

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
  // Consecutive days (back from today, or yesterday if today's not met yet) the
  // count reached its goal. 0 = none; the phone omits the key then.
  int streak;
  // Longest run of goal-met days ever, sent only when it beats `streak` (0
  // otherwise). Shown as "best N" - a target once the current streak lapses.
  int best_streak;
#endif
} Habit;

// Single buffer, not the s_tasks/s_incoming double-buffer, to save RAM on
// aplite. Safe because nothing redraws the habits menu until MSG_HABIT_SYNC_END
// bumps s_habit_count.
#if HEAP_BACKED_LISTS
static Habit *s_habits; // calloc'd in alloc_heap_lists()
#else
static Habit s_habits[MAX_HABITS];
#endif
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
#ifndef PBL_PLATFORM_APLITE
// Folder glyph for the section-0 "Projects" nav row - black normally, white
// when selected, matching the Habits / Add Task icon pair.
static GBitmap *s_project_bitmap;
static GBitmap *s_project_white_bitmap;
// Bar-chart glyph for the section-0 "Stats" nav row - same black/white pair.
static GBitmap *s_stats_bitmap;
static GBitmap *s_stats_white_bitmap;
// Inbox-tray glyph drawn in place of a colour swatch for Super Productivity's
// default "Inbox" project (fixed id, present on every instance, usually has no
// theme colour). Black/white pair for row selection.
static GBitmap *s_inbox_bitmap;
static GBitmap *s_inbox_white_bitmap;
#endif
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
// Routes a dictation_status_callback: Add Task, a note-append, or a Reflect
// "improvement" entry - all share the single s_dictation_session/pending pair.
typedef enum { DICT_ADD_TASK, DICT_NOTE_APPEND, DICT_REFLECT } DictationTarget;
static DictationTarget s_dictation_target = DICT_ADD_TASK;
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
// each open a 5s cancel window on the selected task, reusing the pending-toggle
// pattern above: the row's subtitle shows "Moving to tomorrow..." / "Un-
// Scheduling..." and a single Select cancels before the timer commits. Tracked
// by id for the same background-sync reason. The same window fronts the action
// menu's schedule rows (today / tomorrow / at an hour / unschedule) and the
// done-toggle ("Marking done..."). aplite-excluded for RAM (like the send-retry
// buffer) - and the swipe half is PBL_TOUCH-only regardless.
#define RESCHEDULE_WINDOW_MS 5000
// Below the SDK's 500ms default so a deliberate hold commits before UP/DOWN's
// repeat-scroll walks the selection too far off the intended row.
#define RESCHEDULE_LONGPRESS_MS 400
typedef enum {
  RESCHEDULE_NONE,
  RESCHEDULE_TODAY,
  RESCHEDULE_TOMORROW,
  RESCHEDULE_UNSCHEDULE,
  // Projects-browser only: move the task into / out of its project's backlog.
  RESCHEDULE_TO_BACKLOG,
  RESCHEDULE_FROM_BACKLOG,
  // Action menu "Schedule at...": commit is send_task_set_due_time, hour in
  // s_pending_reschedule_at_hour.
  RESCHEDULE_AT,
} RescheduleKind;
static AppTimer *s_pending_reschedule_timer = NULL;
static char s_pending_reschedule_task_id[MAX_ID_LEN] = "";
static RescheduleKind s_pending_reschedule_kind = RESCHEDULE_NONE;
// Frame ticks (one per scroll_timer_callback, SCROLL_INTERVAL_MS apart) since
// the pending-reschedule window opened - drives the emery shrinking countdown
// bar under its subtitle. Kept alive via refresh_scroll_state.
static int s_pending_reschedule_tick = 0;
// Set when the gesture came from the Projects browser task view - the phone
// uses it to move a scheduled backlog task into the regular list and re-push
// that view. Empty for a today-list reschedule.
static char s_pending_reschedule_project_id[MAX_PROJECT_ID_LEN] = "";
#ifdef PBL_PLATFORM_EMERY
// RESCHEDULE_AT's target hour (0-23) - see the commit callback's send path.
static int s_pending_reschedule_at_hour = 0;
// The done-toggle gets its own copy of the same cancel window: mark a task done
// and its subtitle shows "Marking done..." + the shrinking bar for
// DONE_WINDOW_MS, a Select cancels, the timer commits. Un-completing a task is
// immediate. By id, like the reschedule window. emery only - the 144px slice
// has no code-space headroom, and commits the toggle straight away.
#define DONE_WINDOW_MS 5000
static AppTimer *s_pending_done_timer = NULL;
static char s_pending_done_task_id[MAX_ID_LEN] = "";
static int s_pending_done_tick = 0;
// The Resync row flashes green for SYNC_CHECK_MS on a SYNCING -> OK edge; the
// Add Task row flashes "Added" for the same span once a dictated task is sent.
// Both driven by the scroll timer's tick, like the bar. emery only.
#define SYNC_CHECK_MS 700
static bool s_sync_check_active = false;
static int s_sync_check_tick = 0;
static bool s_addtask_flash_active = false;
static int s_addtask_flash_tick = 0;
// A habit row pulses green for HABIT_FLASH_MS the moment a bump takes it to its
// goal. Its own short repeating timer (the habits list has no idle ticker).
#define HABIT_FLASH_MS 720
#define HABIT_FLASH_STEP_MS 90
static AppTimer *s_habit_flash_timer = NULL;
static char s_habit_flash_id[MAX_HABIT_ID_LEN] = "";
static int s_habit_flash_tick = 0;
#endif
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
// The banner's resting on-screen frame (top of the content area), stamped in
// window_load - banner_slide animates the layer in/out from just above it.
static GRect s_banner_frame;
#define OVERTIME_BANNER_MS 6000
#ifdef PBL_PLATFORM_EMERY
#define OVERTIME_BANNER_HEIGHT 60
#else
#define OVERTIME_BANNER_HEIGHT 52
#endif
#define OVERTIME_REPEAT_INTERVAL_S (5 * 60)

// "Time for a break" banner - shares the over-estimate banner's layer, timer
// and auto-dismiss (show_top_banner). Fires the first time the tracked time
// this watch has banked since the last real break reaches s_break_reminder_min:
// s_break_accum_s (whole sessions that have ended) plus the running session's
// elapsed. A gap between a stop and the next start of at least BREAK_RESET_GAP_S
// counts as a break and zeroes the tally (see start_tracking). Local task
// tracking only; app-open only, like the over-estimate banner. aplite-excluded
// (RAM) - the BREAK_REMINDER guard also kept it off emery until the background
// auto-sync-timer feature was dropped, which freed the room.
#ifndef PBL_PLATFORM_APLITE
#define BREAK_REMINDER
#endif

#ifdef BREAK_REMINDER
static int s_break_reminder_min = 0;         // config.breakReminderMin, 0 = off
static int s_break_accum_s = 0;              // tracked secs banked since the last break
static time_t s_break_last_stop_epoch = 0;   // when the last session ended (gap detection)
// Latched once the banner fires; only re-armed when start_tracking sees a real
// break, so a quick stop/restart doesn't re-nag.
static bool s_break_notified = false;
#define BREAK_RESET_GAP_S (5 * 60)
// Today's break tally for the Stats page: how many gaps >= BREAK_RESET_GAP_S
// between a stop and the next start, and their total seconds. A single break
// is capped (BREAK_MAX_S) so an untracked afternoon doesn't dwarf the number.
// s_break_day is the local_day_id() the counts belong to. Watch-local.
#define BREAK_MAX_S (4 * 3600)
static int s_break_count_today = 0;
static int s_break_total_s_today = 0;
static int s_break_day = 0;

// "Not tracking" nudge - purely time-based, no step count. Counts minutes
// since the last stop (s_break_last_stop_epoch) and repeats a banner every
// s_idle_reminder_min minutes until tracking resumes; s_untracked_notify_count
// is how many of those have fired since the last stop, reset by start_tracking.
static int s_idle_reminder_min = 0;          // config.idleReminderMin, 0 = off
static int s_untracked_notify_count = 0;
#endif

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

// The Task double-buffer. emery keeps both on the heap permanently
// (alloc_heap_lists). basalt/chalk/diorite hit the same 64 KB virtual-size
// ceiling but have too little heap for a permanent copy, so s_incoming there is
// malloc'd only for the span of one sync batch (MSG_TASK_SYNC_START..END) and
// freed after - reclaiming ~9 KB of .bss. On malloc failure it points at
// s_tasks and the batch is parsed in place (a brief torn list, like s_habits
// already accepts). aplite has the .bss room but not the heap, so it keeps the
// plain static pair.
#if !HEAP_BACKED_LISTS && !defined(PBL_PLATFORM_APLITE)
#define INCOMING_MALLOCED 1
#else
#define INCOMING_MALLOCED 0
#endif
#if HEAP_BACKED_LISTS
static Task *s_tasks;             // calloc'd in alloc_heap_lists()
static Task *s_incoming;          // calloc'd in alloc_heap_lists()
#elif INCOMING_MALLOCED
static Task s_tasks[MAX_TASKS];
static Task *s_incoming = NULL;   // malloc'd per sync, freed at SYNC_END
#else
static Task s_tasks[MAX_TASKS];
static Task s_incoming[MAX_TASKS];
#endif
static int s_task_count = 0;      // tasks currently shown (committed)
static int s_incoming_total = 0;  // total announced by the current sync batch
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
#ifndef PBL_PLATFORM_APLITE
// "Schedule" page row (config.enableSchedule, default on). A time-ordered
// view of today's timed tasks - see the schedule-page block far below.
// aplite-excluded (RAM), same as Stats.
static bool s_schedule_enabled = true;
// "Notify before a task is due" (config.dueReminderMin, 0 = off). A MINUTE_UNIT
// tick finds the soonest not-yet-past dueWithTime in s_tasks; when it's within
// this many minutes and s_due_notified_min doesn't already hold that time, the
// banner fires. App-open only; re-arms once that task's time passes.
static int s_due_reminder_min = 0;
static int s_due_notified_min = -1;
// "Stop tracking at midnight" (config.stopAtMidnight, default off). The
// MINUTE_UNIT tick (app-open only) stops a local session that started before
// the current local day, logging only the time up to 00:00; a remote session
// gets a plain stop request. Catches a timer left running overnight the next
// time the app is opened, not just at the stroke of midnight.
static bool s_stop_at_midnight = false;
// "Nudge me about unfinished streaks" (config.habitStreakNudge, default off).
// From 18:00, once per local day, the MINUTE_UNIT tick vibrates a "Keep your
// streak" banner if any habit with a 2+ day streak isn't done yet. App-open
// only. s_streak_nudge_day latches on tm_yday so it fires at most once a day.
static bool s_habit_streak_nudge = false;
static int s_streak_nudge_day = -1;
// "Log energy on Finish Day" (config.enableReflect, default off). Select on the
// Finish Day row opens the Reflect window; long-Select still archives. State
// for the window itself (declared here so dictation_status_callback can flip
// s_reflect_note_set) - see the "reflect" section far below.
static bool s_reflect_enabled = false;
static Window *s_reflect_window = NULL;
static MenuLayer *s_reflect_menu = NULL;
static StatusBarLayer *s_reflect_status_bar = NULL;
static int s_reflect_energy = 0;  // 0 unset, 1 Low .. 3 Good
static int s_reflect_rating = 0;  // 0 unset, 1..4
static bool s_reflect_note_set = false;
// "Stats" page row (config.enableStats, default on) and the last
// MSG_STATS_DATA payload - kept across visits so a re-open shows the previous
// numbers immediately while a fresh request is in flight. Declared here (not
// in the stats-page block far below) because inbox_received_handler and the
// section-0 callbacks reference them. s_stats_have_data starts false so the
// first visit shows "Loading…". All aplite-excluded - that build has no
// Stats row (STATS_ROW_ACTIVE() is a compile-time false) and ~zero RAM to
// spare.
static bool s_stats_enabled = true;
// The Stats page's scrollable text: a "\x02"-prefixed section header line, then
// its lines, repeated - "Last 7 days" (per-day tracked time) then "Projects".
// Heap-backed, alive only while the Stats window is open (stats_window_load /
// _unload) - saves ~830 B of steady-state .bss on basalt/chalk/diorite, which
// run near the heap floor. NULL when the window is closed; the MSG_STATS_DATA
// handler drops the text then (the page re-requests on open anyway).
#define STATS_TEXT_CAP 832
static char *s_stats_projects = NULL;
static int s_stats_est_remaining_ms = 0;
static int s_stats_worked_today_ms = 0;
static int s_stats_done_today = 0;
static bool s_stats_have_data = false;
// "Yesterday toggle" (config.yesterdayStats). When on, long Up/Down on the
// Stats page flips s_stats_show_yesterday: worked/completed switch to the
// phone-sent yesterday figures, the live metrics show a dash.
static bool s_yesterday_stats_enabled = false;
static bool s_stats_show_yesterday = false;
static int s_stats_worked_yesterday_ms = 0;
static int s_stats_done_yesterday = 0;
// "Upcoming" page row (config.enableUpcoming, default on) and its last
// MSG_UPCOMING_DATA payload - future-dated tasks preformatted phone-side into
// "\x02"-prefixed day headers + task lines the watch prints verbatim. The
// same scroll window + text buffer also serves the optional "Notes" page
// (config.enableNotesPage, default off - today-pinned standalone notes); the
// two are never open at once. s_page_mode picks which request the window
// sends and which empty-state text it shows.
static bool s_upcoming_enabled = true;
static bool s_notespage_enabled = false;
typedef enum { PAGE_UPCOMING, PAGE_NOTES } PageMode;
static PageMode s_page_mode = PAGE_UPCOMING;
// Heap-backed, alive only while the shared page window is open (see
// s_stats_projects for the same pattern / rationale). NULL when closed.
#define PAGE_TEXT_CAP 640
static char *s_upcoming_text = NULL;
static bool s_upcoming_have_data = false;
// "Tags" page row (config.enableTags) - default OFF, unlike every other
// optional row. Reuses the Projects-browser window (s_browse_*) with
// s_browse_mode == BROWSE_TAGS; see TAGS_ROW_ACTIVE(). aplite-excluded with the
// browser it borrows.
static bool s_tags_enabled = false;
#endif

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
  int task_count; // active main tasks in the regular list (no backlog, no done)
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
// Which flavour this window is showing. Set before push_browse_window / any
// request; the *_REQUEST sends carry IS_TAGS off it, and the draw / long-press
// paths branch on it (tags have no backlog, no notes, no offline cache).
// BROWSE_TAG_EDIT / BROWSE_MOVE are level-0-only pickers driven from the task
// action menu, both keyed off s_browse_edit_task_id: TAG_EDIT toggles a tag on
// it (checkbox list), MOVE reassigns its project (plain project list).
typedef enum { BROWSE_PROJECTS, BROWSE_TAGS, BROWSE_TAG_EDIT, BROWSE_MOVE } BrowseMode;
static BrowseMode s_browse_mode = BROWSE_PROJECTS;
static char s_browse_edit_task_id[MAX_ID_LEN] = ""; // the task TAG_EDIT / MOVE acts on
// Tag-flavoured modes want the TAG list (IS_TAGS) and have no notes/backlog/
// cache. BROWSE_MOVE is NOT one - it uses the plain project list.
static bool browse_wants_tags(void) {
  return s_browse_mode == BROWSE_TAGS || s_browse_mode == BROWSE_TAG_EDIT;
}
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
// "Enable audible notifications" (config.audibleNotifications): sound a short
// ping alongside every banner - over-estimate, break, idle, task-due. Only
// PBL_SPEAKER hardware (Pebble Time 2) can honour it; the call is compiled out
// everywhere else. Respects the watch's system mute.
static bool s_audible_notify = false;
// config.audibleVolume, 0-100 (speaker_play_notes' volume arg). Default matches
// the pre-slider hardcoded value.
static int s_audible_volume = 80;
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
// The remote task's synced time-spent / estimate (0 = unknown / no estimate).
// When both are set the detail window shows "spent / estimate" like a task row.
static int s_presence_spent_ms = 0;
static int s_presence_estimate_ms = 0;
static Window *s_live_window = NULL;
static TextLayer *s_live_state_layer = NULL;
static TextLayer *s_live_task_layer = NULL;
static TextLayer *s_live_elapsed_layer = NULL;
static TextLayer *s_live_hint_layer = NULL;
static StatusBarLayer *s_live_status_bar = NULL;
static AppTimer *s_live_tick_timer = NULL;
#ifdef PBL_PLATFORM_EMERY
// A depleting ring drawn behind the M:SS during a focus session (only). Its own
// canvas layer, alive only while the live window is - see live_arc_update_proc.
// emery only - no code-space headroom on the 144px slice.
static Layer *s_live_arc_layer = NULL;
#endif

// A remote presence session shows in the pinned "TRACKING" section (the same
// slot local tracking uses) whenever nothing is tracked locally - keeps
// "something is being tracked" looking the same everywhere. If this watch is
// itself tracking, the pinned slot shows the local task and the remote
// session is not surfaced separately (there used to be a dark-blue section-0
// "LIVE" row for that overlap; it was removed as redundant).
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

// One per project run in the grouped today view. Bounded well below MAX_TASKS
// (which would be one task per project - never happens): a real today list has
// a handful of projects. recompute_groups() stretches the last slot over any
// overflow rather than dropping tasks, so a pathological list still renders.
#define MAX_GROUPS 20
#if HEAP_BACKED_LISTS
static TaskGroup *s_groups; // calloc'd in alloc_heap_lists()
#else
static TaskGroup s_groups[MAX_GROUPS];
#endif
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
#define CHROME_FONT_BOLD_KEY FONT_KEY_GOTHIC_18_BOLD
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
#define CHROME_FONT_BOLD_KEY FONT_KEY_GOTHIC_14_BOLD
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

// strncpy that always NUL-terminates - the copy-into-fixed-buffer pattern used
// throughout. `cap` is the full buffer size (a sizeof or a MAX_* constant).
static void str_copy(char *dst, const char *src, size_t cap) {
  strncpy(dst, src, cap - 1);
  dst[cap - 1] = '\0';
}

// graphics_draw_text with the system-font lookup and the always-NULL text
// attributes arg folded in - the shape nearly every draw_* proc uses.
static void draw_text(GContext *ctx, const char *text, const char *font_key,
                      GRect box, GTextOverflowMode overflow, GTextAlignment align) {
  graphics_draw_text(ctx, text, fonts_get_system_font(font_key), box, overflow, align, NULL);
}

// Flat colour fill of `r` - the set-fill-colour + square-cornered fill_rect
// pair every row/header draw proc opens with.
static void fill_bg(GContext *ctx, GRect r, GColor color) {
  graphics_context_set_fill_color(ctx, color);
  graphics_fill_rect(ctx, r, 0, GCornerNone);
}

// The create + add-to-parent pair every window_load repeats.
static StatusBarLayer *add_status_bar(Layer *parent) {
  StatusBarLayer *sb = status_bar_layer_create();
  layer_add_child(parent, status_bar_layer_get_layer(sb));
  return sb;
}

// A text layer with the system font, an alignment, and parented - the common
// case in the sub-window loads. Callers add any extra text_layer_set_* after.
static TextLayer *make_text_layer(Layer *parent, GRect frame, const char *font_key,
                                  GTextAlignment align) {
  TextLayer *tl = text_layer_create(frame);
  text_layer_set_font(tl, fonts_get_system_font(font_key));
  text_layer_set_text_alignment(tl, align);
  layer_add_child(parent, text_layer_get_layer(tl));
  return tl;
}

// One section-0 nav row: a solid colour fill, a left title in the heading font
// (inverting on select), and a right-hand bitmap icon (its white variant on
// select). Shared by Habits / Projects / Stats / Add Task; `icon_rect` keeps
// each row's own icon geometry.
static void draw_nav_row(GContext *ctx, GRect bounds, bool is_selected, GColor bg,
                         const char *label, GBitmap *icon, GBitmap *icon_white, GRect icon_rect) {
  fill_bg(ctx, bounds, bg);
  graphics_context_set_text_color(ctx, is_selected ? GColorWhite : GColorBlack);
  draw_text(ctx, label, HEADING_FONT_KEY,
            GRect(TITLE_BOX_X, HEADING_TITLE_Y(bounds.size.h),
                  bounds.size.w - TITLE_BOX_X * 2 - ROW_ICON_SIZE - 8, HEADING_TITLE_H),
            GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(ctx, is_selected ? icon_white : icon, icon_rect);
}

// Read an optional AppMessage tuple, `fb` when the key is absent. The task /
// habit / project / stats / presence payloads are full of "present it if sent,
// else a default" fields.
static int32_t tuple_int(DictionaryIterator *it, uint32_t key, int32_t fb) {
  Tuple *t = dict_find(it, key);
  return t ? t->value->int32 : fb;
}
static const char *tuple_str(DictionaryIterator *it, uint32_t key, const char *fb) {
  Tuple *t = dict_find(it, key);
  return t ? t->value->cstring : fb;
}

static void recompute_groups(void) {
  s_group_count = 0;
  int i = 0;
  while (i < s_task_count && s_group_count < MAX_GROUPS) {
    int j = i + 1;
    while (j < s_task_count && strncmp(s_tasks[j].project, s_tasks[i].project, MAX_PROJECT_LEN) == 0) {
      j++;
    }
    // Last slot absorbs every remaining task instead of dropping the tail.
    if (s_group_count == MAX_GROUPS - 1) {
      j = s_task_count;
    }
    str_copy(s_groups[s_group_count].name, s_tasks[i].project, MAX_PROJECT_LEN);
#ifndef PBL_PLATFORM_APLITE
    str_copy(s_groups[s_group_count].project_id, s_tasks[i].project_id, MAX_PROJECT_ID_LEN);
#endif
    s_groups[s_group_count].start = i;
    s_groups[s_group_count].count = j - i;
    s_group_count++;
    i = j;
  }
}

// ---------- persistence (so the list survives a watchapp relaunch) ----------

// Task caching is aplite-excluded: its ~24 KB RAM has no margin (the app runs
// within a few hundred bytes of the heap) and its list never persisted anyway
// (see below), so nothing is lost. Everywhere else the cached list is what an
// offline open shows instead of the sync-error screen.
// persist_write_data / persist_read_data silently cap each key at
// PERSIST_DATA_MAX_LENGTH (256 B) - PebbleOS applib/persist.c does
// MIN(buffer_size, PERSIST_DATA_MAX_LENGTH). The old caches wrote a whole
// record array (many KB) to one key, so it was truncated to 256 B on write
// and the exact-length check failed on read - the list never cached at all.
// These helpers split the array across consecutive keys instead.
#ifndef PBL_PLATFORM_APLITE
// Copies `total` bytes between `buf` and keys `base_key`, `base_key+1`, ... in
// 256 B slices. Returns true only if every slice round-tripped its full length.
static bool persist_blob_rw(bool writing, uint32_t base_key, uint8_t *buf, size_t total) {
  size_t off = 0;
  uint32_t key = base_key;
  while (off < total) {
    size_t len = total - off;
    if (len > PERSIST_DATA_MAX_LENGTH) {
      len = PERSIST_DATA_MAX_LENGTH;
    }
    int n = writing ? persist_write_data(key, buf + off, len)
                    : persist_read_data(key, buf + off, len);
    if (n != (int)len) {
      return false;
    }
    off += len;
    key++;
  }
  return true;
}

// A cache of up to `max_count` fixed-size records (`stride` bytes each):
// `count_key` holds the live count, `count_key+1` a stride guard (so a struct
// layout change discards a stale cache), and `chunk_base..` the raw bytes.
// `max_bytes` (>= max_count*stride) bounds the delete sweep.
static void save_blob_cache(uint32_t count_key, uint32_t chunk_base, uint8_t *buf,
                            int count, size_t stride, size_t max_bytes) {
  for (size_t i = 0; i <= max_bytes / PERSIST_DATA_MAX_LENGTH; i++) {
    persist_delete(chunk_base + i);
  }
  if (count <= 0) {
    persist_delete(count_key);
    persist_delete(count_key + 1);
    return;
  }
  if (persist_blob_rw(true, chunk_base, buf, (size_t)count * stride)) {
    persist_write_int(count_key + 1, (int32_t)stride);
    persist_write_int(count_key, count);
  } else {
    persist_delete(count_key);
    persist_delete(count_key + 1);
  }
}

// Returns the loaded record count (0 if absent, stale-layout, or incomplete).
static int load_blob_cache(uint32_t count_key, uint32_t chunk_base, uint8_t *buf,
                           int max_count, size_t stride) {
  if (!persist_exists(count_key) || !persist_exists(count_key + 1) ||
      persist_read_int(count_key + 1) != (int32_t)stride) {
    return 0;
  }
  int count = persist_read_int(count_key);
  if (count <= 0 || count > max_count) {
    return 0;
  }
  return persist_blob_rw(false, chunk_base, buf, (size_t)count * stride) ? count : 0;
}
#endif // !PBL_PLATFORM_APLITE

// ---------- persistence (so the list survives a watchapp relaunch) ----------

// Task caching is aplite-excluded: its ~24 KB RAM has no margin (the app runs
// within a few hundred bytes of the heap) and its list never persisted anyway,
// so nothing is lost. Everywhere else the cached list is what an offline open
// shows instead of the sync-error screen.
#ifdef PBL_PLATFORM_APLITE
static void save_tasks(void) {}
static void load_tasks(void) {}
#else
static void save_tasks(void) {
  persist_delete(100); // legacy single-blob key (pre-chunk builds)
  save_blob_cache(101, 200, (uint8_t *)s_tasks, s_task_count, sizeof(Task),
                  (size_t)MAX_TASKS * sizeof(Task));
}

static void load_tasks(void) {
  s_task_count = load_blob_cache(101, 200, (uint8_t *)s_tasks, MAX_TASKS, sizeof(Task));
}
#endif

static const uint32_t PERSIST_KEY_HABITS = 120;

// aplite fits its 2-habit array in one 256 B key, so it keeps the plain
// single-blob form; everywhere else MAX_HABITS * sizeof(Habit) overflows a
// key and has to be chunked (see save_tasks / save_blob_cache).
#ifdef PBL_PLATFORM_APLITE
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
#else
static void save_habits(void) {
  persist_delete(PERSIST_KEY_HABITS); // legacy single-blob key (pre-chunk builds)
  save_blob_cache(121, 260, (uint8_t *)s_habits, s_habit_count, sizeof(Habit),
                  (size_t)MAX_HABITS * sizeof(Habit));
}

static void load_habits(void) {
  s_habit_count = load_blob_cache(121, 260, (uint8_t *)s_habits, MAX_HABITS, sizeof(Habit));
}
#endif

// Danger-zone "Wipe watch cache" (pairing page -> MSG_WIPE_CACHE): drop every
// persisted list blob - tasks, habits, and the project list - so the next
// offline open starts empty instead of from a stale snapshot. In-memory counts
// are zeroed too; the caller pulls a fresh list from the phone right after.
// Credentials, feature toggles, and tracking/focus state are left alone - those
// aren't "the cache".
static void clear_persisted_caches(void) {
#ifdef PBL_PLATFORM_APLITE
  // aplite only ever persists the small single-blob habit list.
  persist_delete(PERSIST_KEY_HABITS);
  persist_delete(PERSIST_KEY_HABITS + 1);
#else
  persist_delete(100); // legacy single-blob task key (pre-chunk builds)
  save_blob_cache(101, 200, NULL, 0, sizeof(Task), (size_t)MAX_TASKS * sizeof(Task));
  persist_delete(PERSIST_KEY_HABITS); // legacy single-blob habit key
  save_blob_cache(121, 260, NULL, 0, sizeof(Habit), (size_t)MAX_HABITS * sizeof(Habit));
#if PROJECTS_CACHE
  persist_delete(PERSIST_KEY_BROWSE_PROJECTS); // legacy single-blob key
  save_blob_cache(PERSIST_KEY_BROWSE_PROJECTS + 1, 280, NULL, 0, sizeof(BrowseProject),
                  (size_t)MAX_BROWSE_PROJECTS * sizeof(BrowseProject));
  s_browse_project_count = 0;
#endif
#endif
  s_task_count = 0;
  s_habit_count = 0;
}

static const uint32_t PERSIST_KEY_TRACKING_ID = 110;
static const uint32_t PERSIST_KEY_TRACKING_START = 111;

#ifndef PBL_PLATFORM_APLITE
// Focus mode: a watch-local pomodoro wrapped around the current LOCAL
// tracking session (the desktop's own focus mode is not in the SuperSync
// op-log, so it can't be mirrored - this is the watch's own). s_focus_end_epoch
// is the wall-clock time the session ends; 0 = not focusing. Only meaningful
// while s_tracking_task_id is set. s_focus_len_min is the configured length,
// pushed from the pairing page (FOCUS_LEN_MIN). Persisted so the focus screen
// comes straight back if the firmware's inactivity timeout closed the app.
static const uint32_t PERSIST_KEY_FOCUS_END = 114;
static const uint32_t PERSIST_KEY_FOCUS_ON_BREAK = 121;
static time_t s_focus_end_epoch = 0;
static int s_focus_len_min = 25;
// Inherit the desktop Pomodoro timing (globalConfig.pomodoro) instead of the
// FOCUS_LEN_MIN dropdown - config.usePomodoroCfg. When on, a completed focus
// session chains straight into a s_pomodoro_break_min break (s_focus_on_break),
// then buzzes "Break over". MSG_POMODORO_CFG pushes the two minute values.
static bool s_use_pomodoro_cfg = false;
static int s_pomodoro_work_min = 25;
static int s_pomodoro_break_min = 5;
static bool s_focus_on_break = false;
// Anti-inactivity-close lever: pulse the backlight (light_enable_interaction -
// a fading flash, like a button press, not a latch) this often while a focus
// screen is up. There is no real API for this - see the focus-mode comment.
#define FOCUS_LIGHT_POKE_S 300
static time_t s_focus_last_poke_epoch = 0;

// Count of focus sessions that ran ALL the way to 0:00 today (not ones ended
// early). Watch-local - SP has no syncable focus/pomodoro entity, so this
// can't reach the desktop's end-of-day review; it only feeds the Stats page,
// same as "Current session" and "Without a break". s_focus_done_day is a
// local-calendar day id (tm_year*400 + tm_yday) for the rollover check.
static const uint32_t PERSIST_KEY_FOCUS_DONE_COUNT = 115;
static const uint32_t PERSIST_KEY_FOCUS_DONE_DAY = 116;
static int s_focus_completed_today = 0;
static int s_focus_done_day = 0;

static bool focus_active(void) {
  return s_focus_end_epoch != 0 && s_tracking_task_id[0] != '\0';
}

// A local-calendar day id for "resets at midnight" counters (focus sessions,
// break tally). Shared by focus_roll_day and break_roll_day.
static int local_day_id(void) {
  time_t now = time(NULL);
  struct tm *lt = localtime(&now);
  return lt->tm_year * 400 + lt->tm_yday;
}

// Zeroes the completed count when the local day has rolled over. Called before
// every read and before an increment.
static void focus_roll_day(void) {
  int d = local_day_id();
  if (d != s_focus_done_day) {
    s_focus_done_day = d;
    s_focus_completed_today = 0;
  }
}

static void save_focus(void) {
  if (s_focus_end_epoch != 0) {
    persist_write_int(PERSIST_KEY_FOCUS_END, (int)s_focus_end_epoch);
    persist_write_int(PERSIST_KEY_FOCUS_ON_BREAK, s_focus_on_break ? 1 : 0);
  } else {
    persist_delete(PERSIST_KEY_FOCUS_END);
    persist_delete(PERSIST_KEY_FOCUS_ON_BREAK);
  }
  persist_write_int(PERSIST_KEY_FOCUS_DONE_COUNT, s_focus_completed_today);
  persist_write_int(PERSIST_KEY_FOCUS_DONE_DAY, s_focus_done_day);
}

static void load_focus(void) {
  if (persist_exists(PERSIST_KEY_FOCUS_END)) {
    s_focus_end_epoch = (time_t)persist_read_int(PERSIST_KEY_FOCUS_END);
    s_focus_on_break = persist_exists(PERSIST_KEY_FOCUS_ON_BREAK) &&
                       persist_read_int(PERSIST_KEY_FOCUS_ON_BREAK) != 0;
  }
  s_focus_completed_today = persist_read_int(PERSIST_KEY_FOCUS_DONE_COUNT);
  s_focus_done_day = persist_read_int(PERSIST_KEY_FOCUS_DONE_DAY);
  focus_roll_day();
}

// Records one fully-completed focus session for today's Stats count.
static void focus_bump_completed(void) {
  focus_roll_day();
  s_focus_completed_today++;
  save_focus();
}
#endif

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

#ifdef BREAK_REMINDER
static const uint32_t PERSIST_KEY_BREAK_ACCUM_S = 112;
static const uint32_t PERSIST_KEY_BREAK_LAST_STOP = 113;
static const uint32_t PERSIST_KEY_BREAK_COUNT_TODAY = 117;
static const uint32_t PERSIST_KEY_BREAK_TOTAL_S_TODAY = 118;
static const uint32_t PERSIST_KEY_BREAK_DAY = 119;

// Zeroes the Stats break tally when the local day has rolled over.
static void break_roll_day(void) {
  int d = local_day_id();
  if (d != s_break_day) {
    s_break_day = d;
    s_break_count_today = 0;
    s_break_total_s_today = 0;
  }
}

// Persists the break-reminder tally so it survives the app closing between a
// stop and the next start. persist_read_int returns 0 for a missing key, which
// is the right default for all of them.
static void save_break_state(void) {
  persist_write_int(PERSIST_KEY_BREAK_ACCUM_S, s_break_accum_s);
  persist_write_int(PERSIST_KEY_BREAK_LAST_STOP, (int)s_break_last_stop_epoch);
  persist_write_int(PERSIST_KEY_BREAK_COUNT_TODAY, s_break_count_today);
  persist_write_int(PERSIST_KEY_BREAK_TOTAL_S_TODAY, s_break_total_s_today);
  persist_write_int(PERSIST_KEY_BREAK_DAY, s_break_day);
}

static void load_break_state(void) {
  s_break_accum_s = persist_read_int(PERSIST_KEY_BREAK_ACCUM_S);
  s_break_last_stop_epoch = (time_t)persist_read_int(PERSIST_KEY_BREAK_LAST_STOP);
  s_break_count_today = persist_read_int(PERSIST_KEY_BREAK_COUNT_TODAY);
  s_break_total_s_today = persist_read_int(PERSIST_KEY_BREAK_TOTAL_S_TODAY);
  s_break_day = persist_read_int(PERSIST_KEY_BREAK_DAY);
  break_roll_day();
}
#endif

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
static void update_empty_layer(void);
static void push_habits_window(void);
static void update_habits_empty_layer(void);
static Task *find_task_by_id(const char *id);
#ifndef PBL_PLATFORM_APLITE
static void push_stats_window(void);
static void stats_render(void);
static void push_schedule_window(void);
static void schedule_refresh_if_open(void);
static void push_page_window(PageMode mode);
static void upcoming_render(void);
static void handle_repeat_data(DictionaryIterator *it);
#endif
#if PROJECTS_BROWSER
static void push_browse_window(const char *jump_to_project);
static void browse_update_empty(void);
static void request_project_list(void);
static void request_project_tasks(const char *project_id);
static Task *resolve_browse_task_at(MenuIndex index);
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
static void focus_end(bool notify);
static const char *presence_state_phrase(void);
static void send_presence_stop(void);
#endif
#ifndef PBL_PLATFORM_APLITE
static void backlight_touch(void);
#else
#define backlight_touch() ((void)0)
#endif

// The shared head of the sub-window loads: root layer, backlight, a status bar
// (into *status), and the content rect below it (the return value). *root is
// the window's root layer, for the caller's own layer_add_child calls.
static GRect window_chrome(Window *window, StatusBarLayer **status, Layer **root) {
  *root = window_get_root_layer(window);
  GRect b = layer_get_bounds(*root);
  backlight_touch();
  *status = add_status_bar(*root);
  return GRect(b.origin.x, b.origin.y + STATUS_BAR_LAYER_HEIGHT,
               b.size.w, b.size.h - STATUS_BAR_LAYER_HEIGHT);
}
#ifndef PBL_PLATFORM_APLITE
static void show_notes_overlay(Task *task);
static void show_project_notes_overlay(TaskGroup *group);
static void hide_notes_overlay(void);
static void push_notes_window(void);
static bool try_open_checklist(void);
static void push_reflect_window(void);
typedef enum { PICK_ESTIMATE, PICK_DEADLINE, PICK_HABIT, PICK_TIME } PickKind;
// task_id is a habit id for PICK_HABIT. current: ms (estimate) / days-from-today
// or DEADLINE_NONE (deadline) / the counter's value (habit) / hour 0-23 (time).
static void push_value_picker(PickKind kind, const char *task_id, int current);
static void send_task_set_due_time(const char *task_id, int hour);
typedef enum { ACTX_TODAY, ACTX_PROJECT, ACTX_TAG } ActionCtx;
static void push_action_menu(const char *task_id, ActionCtx ctx, bool in_backlog);
static void pending_toggle_timer_callback(void *data);
static void pending_reschedule_timer_callback(void *data);
static void cancel_pending_reschedule(void);
static void begin_pending_reschedule(RescheduleKind kind);
#ifdef PBL_PLATFORM_EMERY
static void pending_done_commit_callback(void *data);
static void begin_pending_done(const char *task_id);
static void cancel_pending_done(void);
#endif
static void send_task_reschedule(const char *task_id, RescheduleKind kind, const char *project_id);
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
  SECTION0_ROW_TAGS,     // tags page, right after Projects, opt-in / default off (non-aplite)
  SECTION0_ROW_STATS,    // stats page, between Projects and Add Task (non-aplite)
  SECTION0_ROW_SCHEDULE, // schedule page, between Stats and Add Task (non-aplite)
  SECTION0_ROW_UPCOMING, // upcoming page, between Schedule and Add Task (non-aplite)
  SECTION0_ROW_NOTESPAGE, // notes page, right after Upcoming, opt-in / default off (non-aplite)
  SECTION0_ROW_ADD_TASK,
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
// browser isn't built (aplite - see PROJECTS_BROWSER).
#if PROJECTS_BROWSER
#define PROJECTS_ROW_ACTIVE() (s_projects_enabled)
#else
#define PROJECTS_ROW_ACTIVE() false
#endif

// Whether the "Tags" row sits in section 0 - opt-in (default off) and only
// where the browser it reuses is built.
#if PROJECTS_BROWSER
#define TAGS_ROW_ACTIVE() (s_tags_enabled)
#else
#define TAGS_ROW_ACTIVE() false
#endif

// Whether the "Stats" row sits in section 0. Compile-time false on aplite.
#ifdef PBL_PLATFORM_APLITE
#define STATS_ROW_ACTIVE() false
#else
#define STATS_ROW_ACTIVE() (s_stats_enabled)
#endif

// Whether the "Schedule" row sits in section 0. Compile-time false on aplite.
#ifdef PBL_PLATFORM_APLITE
#define SCHEDULE_ROW_ACTIVE() false
#else
#define SCHEDULE_ROW_ACTIVE() (s_schedule_enabled)
#endif

// Whether the "Upcoming" row sits in section 0. Compile-time false on aplite.
#ifdef PBL_PLATFORM_APLITE
#define UPCOMING_ROW_ACTIVE() false
#define NOTESPAGE_ROW_ACTIVE() false
#else
#define UPCOMING_ROW_ACTIVE() (s_upcoming_enabled)
#define NOTESPAGE_ROW_ACTIVE() (s_notespage_enabled)
#endif

// A remote presence session shows in the pinned "TRACKING" section
// (remote_in_pinned_section) whenever nothing is tracked locally. There used
// to be a separate dark-blue "LIVE" row at the top of section 0 as well; it
// only duplicated the same task and was removed. In the rare overlap - this
// watch tracking its own task while another device also tracks - the pinned
// slot shows the local task and the remote session is not surfaced
// separately.

static int section0_row_count(void) {
  int count = 1; // Resync always present.
  if (s_habits_enabled) {
    count++;
  }
  if (PROJECTS_ROW_ACTIVE()) {
    count++;
  }
  if (TAGS_ROW_ACTIVE()) {
    count++;
  }
  if (STATS_ROW_ACTIVE()) {
    count++;
  }
  if (SCHEDULE_ROW_ACTIVE()) {
    count++;
  }
  if (UPCOMING_ROW_ACTIVE()) {
    count++;
  }
  if (NOTESPAGE_ROW_ACTIVE()) {
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
  if (TAGS_ROW_ACTIVE()) {
    if (row == next) {
      return SECTION0_ROW_TAGS;
    }
    next++;
  }
  if (STATS_ROW_ACTIVE()) {
    if (row == next) {
      return SECTION0_ROW_STATS;
    }
    next++;
  }
  if (SCHEDULE_ROW_ACTIVE()) {
    if (row == next) {
      return SECTION0_ROW_SCHEDULE;
    }
    next++;
  }
  if (UPCOMING_ROW_ACTIVE()) {
    if (row == next) {
      return SECTION0_ROW_UPCOMING;
    }
    next++;
  }
  if (NOTESPAGE_ROW_ACTIVE()) {
    if (row == next) {
      return SECTION0_ROW_NOTESPAGE;
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
// GColor8 byte, centred vertically at x. Returns the x for the following text -
// unchanged (nothing drawn) when the project has no colour. Shared by the
// grouped today view's project rows and the Projects browser's list.
static int16_t draw_project_swatch(GContext *ctx, int16_t x, int16_t cell_h, uint8_t color) {
  if (color == 0) {
    return x;
  }
  fill_bg(ctx, GRect(x, (cell_h - 16) / 2, 16, 16), (GColor8){ .argb = color });
  return x + 22;
}

// Super Productivity's built-in default project - a fixed id, the same on every
// instance (see index.js's Inbox fallback for Add Task).
#define INBOX_PROJECT_ID "INBOX_PROJECT"

// The left-of-title marker on a project row: the Inbox glyph for the default
// project (which typically has no theme colour to show a swatch for), else the
// project's colour swatch. Returns the x the title text should start at.
static int16_t draw_project_marker(GContext *ctx, int16_t x, int16_t cell_h,
                                    const char *project_id, uint8_t color, bool is_selected) {
  if (project_id && strcmp(project_id, INBOX_PROJECT_ID) == 0) {
    graphics_context_set_compositing_mode(ctx, GCompOpSet);
    graphics_draw_bitmap_in_rect(ctx, is_selected ? s_inbox_white_bitmap : s_inbox_bitmap,
                                  GRect(x, (cell_h - 16) / 2, 16, 16));
    return x + 22;
  }
  return draw_project_swatch(ctx, x, cell_h, color);
}
#endif

#ifndef PBL_PLATFORM_APLITE
// Defined further down with the marquee helpers; used by the pinned tracking
// header's sweeping-word draw below.
static int16_t title_natural_width_font(const char *title, GFont font);
static int16_t pingpong_offset(int16_t travel);
#endif

static void menu_draw_header(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *context) {
  if (s_task_count == 0) {
    if (section_index == 0 && ACTIONABLE_EMPTY_ACTIVE()) {
      // "No tasks for today." above the still-reachable section-0 rows.
      GRect bounds = layer_get_bounds(cell_layer);
      fill_bg(ctx, bounds, GColorWhite);
      graphics_context_set_text_color(ctx, GColorBlack);
      draw_text(ctx, "No tasks for today.", EMPTY_MSG_FONT_KEY, bounds, GTextOverflowModeWordWrap, GTextAlignmentCenter);
    }
    return;
  }
  if (section_index == 0) {
    return;
  }
#ifndef PBL_PLATFORM_APLITE
  // The pinned tracking header - a thin green strip. Bold (CHROME_FONT_BOLD_KEY,
  // same point size as CHROME_FONT_KEY) so it reads at a glance. While THIS
  // watch is the one tracking, the word sweeps gently left<->right across the
  // strip to catch the eye; a remote presence session leaves it static,
  // left-aligned.
  if (has_pinned_row() && section_index == 1) {
    GRect hb = layer_get_bounds(cell_layer);
    fill_bg(ctx, hb, GColorGreen);
    graphics_context_set_text_color(ctx, GColorBlack);
    const char *label = !focus_active() ? "TRACKING" : s_focus_on_break ? "BREAK" : "FOCUSING";
    GFont label_font = fonts_get_system_font(CHROME_FONT_BOLD_KEY);
    int16_t avail = hb.size.w - TITLE_BOX_X * 2;
    int16_t word_w = title_natural_width_font(label, label_font);
    int16_t x = TITLE_BOX_X;
    if (s_tracking_task_id[0] != '\0' && avail - word_w > 0) {
      x += pingpong_offset(avail - word_w);
    }
    graphics_draw_text(ctx, label, label_font, GRect(x, 0, word_w + 4, hb.size.h),
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
  fill_bg(ctx, bounds, GColorGreen);

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

#ifndef PBL_PLATFORM_APLITE
static int16_t title_natural_width_font(const char *title, GFont font) {
  GSize size = graphics_text_layout_get_content_size(
      title, font, GRect(0, 0, 2000, 100),
      GTextOverflowModeFill, GTextAlignmentLeft);
  return size.w;
}

// Draws `text` in `box`: left-aligned with a trailing ellipsis normally, or -
// when `marquee` is set - as a looping two-copy horizontal scroll offset by
// s_scroll_offset_px (whose timer refresh_scroll_state owns). Used by the
// live-tracking presence rows; the task rows have their own inline copy.
static void draw_marquee_title(GContext *ctx, GRect box, const char *text, GFont font, bool marquee) {
  if (marquee) {
    int16_t natural_width = title_natural_width_font(text, font);
    int16_t period = natural_width + SCROLL_GAP_PX;
    int16_t x = -(s_scroll_offset_px % period);
    graphics_draw_text(ctx, text, font,
                        GRect(box.origin.x + x, box.origin.y, natural_width, box.size.h),
                        GTextOverflowModeFill, GTextAlignmentLeft, NULL);
    graphics_draw_text(ctx, text, font,
                        GRect(box.origin.x + x + period, box.origin.y, natural_width, box.size.h),
                        GTextOverflowModeFill, GTextAlignmentLeft, NULL);
  } else {
    graphics_draw_text(ctx, text, font, box, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  }
}

// Triangle-wave position 0 -> travel -> 0, driven by the shared
// s_scroll_offset_px counter, with a brief hold at each end. Drives the pinned
// "TRACKING" header word's gentle left<->right sweep while a local session runs.
static int16_t pingpong_offset(int16_t travel) {
  if (travel <= 0) {
    return 0;
  }
  const int32_t hold = 20;
  int32_t leg = travel + hold;
  int32_t phase = (int32_t)s_scroll_offset_px % (leg * 2);
  int32_t pos = phase < leg ? phase : (leg * 2 - phase);
  return (int16_t)(pos > travel ? travel : pos);
}
#endif

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
  // State 5: the phone's presence socket dropped mid-session (index.js's
  // pushPresenceOffline). Not "<verb> on <device>" - the device is fine, our
  // link to it isn't.
  if (s_presence_state == 5) {
    return "Offline";
  }
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
#ifndef PBL_PLATFORM_APLITE
  if (s_pending_reschedule_kind != RESCHEDULE_NONE) {
    s_pending_reschedule_tick++;
  }
#endif
#ifdef PBL_PLATFORM_EMERY
  if (s_pending_done_task_id[0] != '\0') {
    s_pending_done_tick++;
  }
  if (s_sync_check_active) {
    s_sync_check_tick++;
    if (s_sync_check_tick * SCROLL_INTERVAL_MS >= SYNC_CHECK_MS) {
      s_sync_check_active = false;
    }
  }
  if (s_addtask_flash_active) {
    s_addtask_flash_tick++;
    if (s_addtask_flash_tick * SCROLL_INTERVAL_MS >= SYNC_CHECK_MS) {
      s_addtask_flash_active = false;
    }
  }
#endif
  layer_mark_dirty(menu_layer_get_layer(s_menu_layer));
#if PROJECTS_BROWSER
  // A pending reschedule started from the browser task view animates its bar
  // there, not on the today list.
  if (s_browse_menu && s_pending_reschedule_kind != RESCHEDULE_NONE) {
    layer_mark_dirty(menu_layer_get_layer(s_browse_menu));
  }
#endif
  s_scroll_timer = app_timer_register(SCROLL_INTERVAL_MS, scroll_timer_callback, NULL);
}

#ifndef PBL_PLATFORM_APLITE
// Whether the currently selected row is the remote row in the pinned
// "TRACKING" section - it draws s_presence_task and so can also want a
// marquee when the name overflows.
static bool selected_row_is_presence_title(void) {
  if (s_presence_task[0] == '\0') {
    return false;
  }
  MenuIndex sel = menu_layer_get_selected_index(s_menu_layer);
  return remote_in_pinned_section() && sel.section == 1 && sel.row == 0;
}
#endif

// Starts/stops the marquee timer to match whether the selected row needs it,
// optionally resetting the scroll position. Called on selection or list change.
// Assembles a task row's subtitle: issue key, then the deadline marker, the
// due time, and spent/estimate - each appended with a separator only when
// something's already there. Shared by draw_task_row and refresh_scroll_state
// (which needs the text to decide whether to run the marquee timer). Does NOT
// cover the "Done" / pending-reschedule cases - draw_task_row handles those
// before calling here, and they never need to scroll.
static void build_task_subtitle(Task *task, char *out, size_t cap) {
  out[0] = '\0';
  bool is_tracking_this = s_tracking_task_id[0] != '\0' &&
                           strncmp(s_tracking_task_id, task->id, MAX_ID_LEN) == 0;
  int effective_ms = task->time_spent_ms;
  if (is_tracking_this) {
    time_t elapsed_s = time(NULL) - s_tracking_start_epoch;
    if (elapsed_s > 0) {
      effective_ms += (int)elapsed_s * 1000;
    }
  }
#ifndef PBL_PLATFORM_APLITE
  if (task->issue_key[0] != '\0') {
    str_copy(out, task->issue_key, cap);
  }
#endif
  if (task->deadline_days != DEADLINE_NONE) {
    char dl_text[16];
    if (task->deadline_days < 0) {
      str_copy(dl_text, "! overdue", sizeof(dl_text));
    } else if (task->deadline_days == 0) {
      str_copy(dl_text, "! today", sizeof(dl_text));
    } else {
      snprintf(dl_text, sizeof(dl_text), "! %dd", task->deadline_days);
    }
    size_t n = strlen(out);
    if (n > 0) {
      snprintf(out + n, cap - n, "  %s", dl_text);
    } else {
      str_copy(out, dl_text, cap);
    }
  }
  if (task->due_min >= 0) {
    char due_text[16];
    format_due_time(task->due_min, due_text, sizeof(due_text));
    size_t n = strlen(out);
    if (n > 0) {
      snprintf(out + n, cap - n, "  %s", due_text);
    } else {
      str_copy(out, due_text, cap);
    }
  }
  if (effective_ms > 0 || is_tracking_this) {
    char time_text[20];
    format_duration_ms(effective_ms, is_tracking_this, time_text, sizeof(time_text));
    if (task->time_estimate_ms > 0) {
      char estimate_text[20];
      format_duration_ms(task->time_estimate_ms, false, estimate_text, sizeof(estimate_text));
      char combined[48];
      snprintf(combined, sizeof(combined), "%s / %s", time_text, estimate_text);
      str_copy(time_text, combined, sizeof(time_text));
    }
    size_t n = strlen(out);
    if (n > 0) {
      snprintf(out + n, cap - n, "%s%s", is_tracking_this ? "  " : " - ", time_text);
    } else {
      str_copy(out, time_text, cap);
    }
  }
}

static void refresh_scroll_state(bool reset_offset) {
  if (reset_offset) {
    s_scroll_offset_px = 0;
  }
  Task *selected = resolve_selected_task();
  GRect menu_bounds = layer_get_bounds(menu_layer_get_layer(s_menu_layer));
  int16_t available = menu_bounds.size.w - TITLE_BOX_X * 2;
  bool needs_scroll = selected && title_natural_width(selected->title) > available;
#ifndef PBL_PLATFORM_APLITE
  // Keep the repaint timer alive while a transient row animation is running.
  if (s_pending_reschedule_kind != RESCHEDULE_NONE
#ifdef PBL_PLATFORM_EMERY
      || s_pending_done_task_id[0] != '\0' || s_sync_check_active || s_addtask_flash_active
#endif
     ) {
    needs_scroll = true;
  }
  if (!needs_scroll && selected && !selected->done) {
    char sub[56];
    build_task_subtitle(selected, sub, sizeof(sub));
    if (sub[0] != '\0' &&
        title_natural_width_font(sub, fonts_get_system_font(SUBTITLE_FONT_KEY)) > available) {
      needs_scroll = true;
    }
  }
#endif
#ifndef PBL_PLATFORM_APLITE
  if (!needs_scroll && selected_row_is_presence_title()) {
    needs_scroll = title_natural_width(s_presence_task) > available;
  }
  // Keep the repaint timer running the whole time this watch is tracking: the
  // pinned tracking header's word sweeps left<->right for the entire session,
  // regardless of the selected row.
  if (has_pinned_row() && s_tracking_task_id[0] != '\0') {
    needs_scroll = true;
  }
#endif
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
  fill_bg(ctx, bounds, bg);
  graphics_context_set_text_color(ctx, fg);

  GFont title_font = fonts_get_system_font(TITLE_FONT_KEY);
  GSize one_line_size = graphics_text_layout_get_content_size(
      "Ag", title_font, GRect(0, 0, 200, 100), GTextOverflowModeFill, GTextAlignmentLeft);
  int16_t title_box_h = one_line_size.h > 0 ? one_line_size.h : (bounds.size.h - TITLE_BOX_Y);
  GRect title_box = GRect(TITLE_BOX_X, ROW_TITLE_TOP_Y(bounds.size.h, title_box_h, SUBTITLE_STRIP_H),
                           bounds.size.w - TITLE_BOX_X * 2, title_box_h);

  // Recurring-task glyph on the right of the title line: an open circle-arrow.
  // The title box shrinks to leave room so its ellipsis clears the glyph.
  if (task->recurs && !needs_marquee) {
    int16_t gx = bounds.size.w - TITLE_BOX_X - 12;
    int16_t gy = title_box.origin.y + title_box.size.h / 2;
    graphics_context_set_stroke_color(ctx, fg);
    graphics_draw_arc(ctx, GRect(gx, gy - 5, 11, 11), GOvalScaleModeFitCircle,
                      DEG_TO_TRIGANGLE(35), DEG_TO_TRIGANGLE(330));
    graphics_draw_line(ctx, GPoint(gx + 9, gy - 5), GPoint(gx + 12, gy - 2));
    graphics_draw_line(ctx, GPoint(gx + 9, gy - 5), GPoint(gx + 6, gy - 3));
    title_box.size.w -= 16;
  }

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
  // A pending reschedule (and, on emery, a pending done-toggle) takes over the
  // whole subtitle line with a centred message; emery also draws a centre-
  // anchored bar that shrinks as the cancel window runs out. A Select commits
  // early / cancels. See begin_pending_reschedule / begin_pending_done.
  {
    const char *pending_msg = NULL;
    int pend_total_ms = 0, pend_tick = 0;
    if (s_pending_reschedule_kind != RESCHEDULE_NONE &&
        strncmp(s_pending_reschedule_task_id, task->id, MAX_ID_LEN) == 0) {
#ifdef PBL_PLATFORM_EMERY
      static char at_msg[28];
      if (s_pending_reschedule_kind == RESCHEDULE_AT) {
        int h = s_pending_reschedule_at_hour;
        if (clock_is_24h_style()) {
          snprintf(at_msg, sizeof(at_msg), "Scheduling %d:00...", h);
        } else {
          int h12 = h % 12;
          if (h12 == 0) {
            h12 = 12;
          }
          snprintf(at_msg, sizeof(at_msg), "Scheduling %d %s...", h12, h < 12 ? "AM" : "PM");
        }
        pending_msg = at_msg;
      } else
#endif
      {
        pending_msg = s_pending_reschedule_kind == RESCHEDULE_TOMORROW ? "Moving to tomorrow..."
                      : s_pending_reschedule_kind == RESCHEDULE_TODAY ? "Scheduling for today..."
                      : s_pending_reschedule_kind == RESCHEDULE_TO_BACKLOG ? "Moving to backlog..."
                      : s_pending_reschedule_kind == RESCHEDULE_FROM_BACKLOG ? "Moving to list..."
                      : "Un-Scheduling...";
      }
      pend_total_ms = RESCHEDULE_WINDOW_MS;
      pend_tick = s_pending_reschedule_tick;
    }
#ifdef PBL_PLATFORM_EMERY
    else if (s_pending_done_task_id[0] != '\0' &&
             strncmp(s_pending_done_task_id, task->id, MAX_ID_LEN) == 0) {
      pending_msg = "Marking done...";
      pend_total_ms = DONE_WINDOW_MS;
      pend_tick = s_pending_done_tick;
    }
#endif
    if (pending_msg) {
      GColor pend_crisp = is_selected ? GColorWhite : GColorBlack;
      graphics_context_set_text_color(ctx, pend_crisp);
      draw_text(ctx, pending_msg, SUBTITLE_FONT_KEY, subtitle_box, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter);
#ifdef PBL_PLATFORM_EMERY
      int rem_ms = pend_total_ms - pend_tick * SCROLL_INTERVAL_MS;
      if (rem_ms < 0) {
        rem_ms = 0;
      }
      int full_w = subtitle_box.size.w;
      int bar_w = full_w * rem_ms / pend_total_ms;
      graphics_context_set_fill_color(ctx, pend_crisp);
      graphics_fill_rect(ctx,
                         GRect(subtitle_box.origin.x + (full_w - bar_w) / 2,
                               subtitle_box.origin.y + subtitle_box.size.h - 3, bar_w, 2),
                         0, GCornerNone);
#else
      (void)pend_total_ms;
      (void)pend_tick;
#endif
      return;
    }
  }
#endif

  if (task->done) {
    draw_text(ctx, "Done", SUBTITLE_FONT_KEY, subtitle_box, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter);
    return;
  }

  char subtitle[56];
  build_task_subtitle(task, subtitle, sizeof(subtitle));

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
#ifndef PBL_PLATFORM_APLITE
    GFont sf = fonts_get_system_font(SUBTITLE_FONT_KEY);
    // Marquee the subtitle on the selected row when it overflows - same
    // two-copy scroll the title uses, sharing s_scroll_offset_px. Not on the
    // pinned row (its right-aligned project name already claims half the line).
    bool sub_marquee = is_selected && !show_project &&
                        title_natural_width_font(subtitle, sf) > left_box.size.w;
    draw_marquee_title(ctx, left_box, subtitle, sf, sub_marquee);
#else
    draw_text(ctx, subtitle, SUBTITLE_FONT_KEY, left_box, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
#endif
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

    if (kind == SECTION0_ROW_HABITS) {
      // Navigates to the habits (SimpleCounter) page. Icon matches the real
      // app's "heart_check" icon; GCompOpSet lets its transparent background
      // through.
#ifdef PBL_PLATFORM_APLITE
      // aplite has no RAM headroom for the progress layout below - plain row.
      GRect ic = GRect(bounds.size.w - ROW_ICON_SIZE - 10, (bounds.size.h - ROW_ICON_SIZE) / 2,
                       ROW_ICON_SIZE, ROW_ICON_SIZE);
      draw_nav_row(ctx, bounds, is_selected, GColorVividCerulean, "Habits",
                   s_heart_bitmap, s_heart_white_bitmap, ic);
#else
      // Title + a "N left today" / "All done today" progress subtitle, laid out
      // like the Resync row below. The all-done state gets a small check drawn
      // ahead of its subtitle.
      int habits_left = 0;
      for (int i = 0; i < s_habit_count; i++) {
        if (!s_habits[i].done) {
          habits_left++;
        }
      }
      bool all_done = s_habit_count > 0 && habits_left == 0;
      GColor fg = is_selected ? GColorWhite : GColorBlack;
      fill_bg(ctx, bounds, GColorVividCerulean);
      graphics_context_set_text_color(ctx, fg);
      draw_text(ctx, "Habits", HEADING_FONT_KEY,
                GRect(TITLE_BOX_X, ROW_TITLE_TOP_Y(bounds.size.h, HEADING_TITLE_H, CHROME_STRIP_H),
                      bounds.size.w - TITLE_BOX_X * 2 - ROW_ICON_SIZE - 8, HEADING_TITLE_H),
                GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
      if (s_habit_count > 0) {
        int16_t sub_x = TITLE_BOX_X;
        int16_t sub_y = ROW_SUBTITLE_TOP_Y(bounds.size.h, HEADING_TITLE_H, CHROME_STRIP_H);
        char hsub[22];
        if (all_done) {
          graphics_context_set_stroke_color(ctx, fg);
          graphics_draw_line(ctx, GPoint(sub_x, sub_y + 8), GPoint(sub_x + 3, sub_y + 11));
          graphics_draw_line(ctx, GPoint(sub_x + 3, sub_y + 11), GPoint(sub_x + 9, sub_y + 4));
          sub_x += 13;
          str_copy(hsub, "All done today", sizeof(hsub));
        } else {
          snprintf(hsub, sizeof(hsub), "%d left today", habits_left);
        }
        draw_text(ctx, hsub, CHROME_FONT_KEY,
                  GRect(sub_x, sub_y, bounds.size.w - sub_x - TITLE_BOX_X, CHROME_STRIP_H),
                  GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
      }
      GRect ic = GRect(bounds.size.w - ROW_ICON_SIZE - 10,
                       s_habit_count > 0
                         ? ROW_TITLE_TOP_Y(bounds.size.h, HEADING_TITLE_H, CHROME_STRIP_H) + 2
                         : (bounds.size.h - ROW_ICON_SIZE) / 2,
                       ROW_ICON_SIZE, ROW_ICON_SIZE);
      graphics_context_set_compositing_mode(ctx, GCompOpSet);
      graphics_draw_bitmap_in_rect(ctx, is_selected ? s_heart_white_bitmap : s_heart_bitmap, ic);
#endif
      return;
    }

#if PROJECTS_BROWSER
    if (kind == SECTION0_ROW_PROJECTS) {
      // Navigates to the projects browser. Purple - its own colour among the
      // section-0 nav rows. White folder icon on the right.
      GRect ic = GRect(bounds.size.w - ROW_ICON_SIZE - 8, (bounds.size.h - 20) / 2, 20, 20);
      draw_nav_row(ctx, bounds, is_selected, GColorPurple, "Projects",
                   s_project_bitmap, s_project_white_bitmap, ic);
      return;
    }

    if (kind == SECTION0_ROW_TAGS) {
      // Opens the Tags page (Projects browser reused in BROWSE_TAGS mode). Mint
      // green - distinct from every other section-0 row (the old Limerick read
      // as the same yellow as the Schedule row). Darkens to JaegerGreen with
      // white text on select. A "#" hash glyph on the right, from primitives.
      GColor icon = is_selected ? GColorWhite : GColorBlack;
      fill_bg(ctx, bounds, is_selected ? GColorJaegerGreen : GColorMintGreen);
      graphics_context_set_text_color(ctx, icon);
      GRect tags_title_box = GRect(TITLE_BOX_X, HEADING_TITLE_Y(bounds.size.h),
                                    bounds.size.w - TITLE_BOX_X * 2 - ROW_ICON_SIZE - 8, HEADING_TITLE_H);
      draw_text(ctx, "Tags", HEADING_FONT_KEY, tags_title_box, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
      int16_t hx = bounds.size.w - ROW_ICON_SIZE - 4;
      int16_t hy = bounds.size.h / 2;
      graphics_context_set_stroke_color(ctx, icon);
      graphics_draw_line(ctx, GPoint(hx + 2, hy - 6), GPoint(hx, hy + 6));       // "#" left bar
      graphics_draw_line(ctx, GPoint(hx + 8, hy - 6), GPoint(hx + 6, hy + 6));   // "#" right bar
      graphics_draw_line(ctx, GPoint(hx - 2, hy - 2), GPoint(hx + 10, hy - 2));  // "#" top bar
      graphics_draw_line(ctx, GPoint(hx - 3, hy + 3), GPoint(hx + 9, hy + 3));   // "#" bottom bar
      return;
    }
#endif

#ifndef PBL_PLATFORM_APLITE
    if (kind == SECTION0_ROW_STATS) {
      // Opens the read-only Stats page. Orange, bar-chart icon.
      GRect ic = GRect(bounds.size.w - ROW_ICON_SIZE - 8, (bounds.size.h - 20) / 2, 20, 20);
      draw_nav_row(ctx, bounds, is_selected, GColorOrange, "Stats",
                   s_stats_bitmap, s_stats_white_bitmap, ic);
      return;
    }
#endif

#ifndef PBL_PLATFORM_APLITE
    if (kind == SECTION0_ROW_SCHEDULE) {
      // Opens the Schedule page - today's timed tasks in time order. Yellow: the
      // old teal read as the same blue as the Habits row (cerulean) on-watch, so
      // it's now a hue nothing else in section 0 uses (Resync red, Habits
      // cerulean, Projects purple, Stats orange, Upcoming indigo, Add Task
      // green). A clock glyph on the right, drawn from primitives (no bitmap
      // asset), black normally / white when selected like the others.
      GColor icon = is_selected ? GColorWhite : GColorBlack;
      fill_bg(ctx, bounds, GColorYellow);
      graphics_context_set_text_color(ctx, icon);
      GRect sched_title_box = GRect(TITLE_BOX_X, HEADING_TITLE_Y(bounds.size.h),
                                     bounds.size.w - TITLE_BOX_X * 2 - ROW_ICON_SIZE - 8, HEADING_TITLE_H);
      draw_text(ctx, "Schedule", HEADING_FONT_KEY, sched_title_box, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
      GPoint clock_c = GPoint(bounds.size.w - ROW_ICON_SIZE - 8 + 10, bounds.size.h / 2);
      graphics_context_set_stroke_color(ctx, icon);
      graphics_draw_circle(ctx, clock_c, 8);
      graphics_draw_line(ctx, clock_c, GPoint(clock_c.x, clock_c.y - 5));     // minute hand
      graphics_draw_line(ctx, clock_c, GPoint(clock_c.x + 4, clock_c.y + 2)); // hour hand
      return;
    }
#endif

#ifndef PBL_PLATFORM_APLITE
    if (kind == SECTION0_ROW_UPCOMING) {
      // Opens the Upcoming page - future-dated tasks by day. Indigo, its own
      // colour among the nav rows. A ">>" glyph on the right, from primitives.
      GColor icon = is_selected ? GColorWhite : GColorBlack;
      fill_bg(ctx, bounds, GColorIndigo);
      graphics_context_set_text_color(ctx, icon);
      GRect up_title_box = GRect(TITLE_BOX_X, HEADING_TITLE_Y(bounds.size.h),
                                  bounds.size.w - TITLE_BOX_X * 2 - ROW_ICON_SIZE - 8, HEADING_TITLE_H);
      draw_text(ctx, "Upcoming", HEADING_FONT_KEY, up_title_box, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
      int16_t cx = bounds.size.w - ROW_ICON_SIZE - 4;
      int16_t cy = bounds.size.h / 2;
      graphics_context_set_stroke_color(ctx, icon);
      for (int k = 0; k < 2; k++) {
        int16_t x = cx + k * 6;
        graphics_draw_line(ctx, GPoint(x, cy - 5), GPoint(x + 5, cy));
        graphics_draw_line(ctx, GPoint(x + 5, cy), GPoint(x, cy + 5));
      }
      return;
    }
#endif

#ifndef PBL_PLATFORM_APLITE
    if (kind == SECTION0_ROW_NOTESPAGE) {
      // Opens the Notes page - today-pinned standalone notes. Light grey,
      // inverting to dark grey + white on select like the other nav rows. A
      // lined-note glyph on the right.
      GColor fg = is_selected ? GColorWhite : GColorBlack;
      fill_bg(ctx, bounds, is_selected ? GColorDarkGray : GColorLightGray);
      graphics_context_set_text_color(ctx, fg);
      GRect np_title_box = GRect(TITLE_BOX_X, HEADING_TITLE_Y(bounds.size.h),
                                  bounds.size.w - TITLE_BOX_X * 2 - ROW_ICON_SIZE - 8, HEADING_TITLE_H);
      draw_text(ctx, "Notes", HEADING_FONT_KEY, np_title_box, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
      int16_t gx = bounds.size.w - ROW_ICON_SIZE - 4;
      int16_t gy = bounds.size.h / 2;
      graphics_context_set_stroke_color(ctx, fg);
      graphics_draw_rect(ctx, GRect(gx, gy - 7, 13, 15));
      for (int k = 0; k < 3; k++) {
        graphics_draw_line(ctx, GPoint(gx + 3, gy - 3 + k * 3), GPoint(gx + 10, gy - 3 + k * 3));
      }
      return;
    }
#endif

#ifndef PBL_PLATFORM_APLITE
    if (kind == SECTION0_ROW_ADD_TASK) {
      // Mic platforms with the feature enabled only. Starts dictation via
      // menu_select_click.
      GRect ic = GRect(bounds.size.w - ROW_ICON_SIZE - 10, (bounds.size.h - ROW_ICON_SIZE) / 2,
                       ROW_ICON_SIZE, ROW_ICON_SIZE);
#ifdef PBL_PLATFORM_EMERY
      if (s_addtask_flash_active) {
        draw_nav_row(ctx, bounds, is_selected, GColorGreen, "Added",
                     s_mic_bitmap, s_mic_white_bitmap, ic);
        return;
      }
#endif
      draw_nav_row(ctx, bounds, is_selected, GColorJaegerGreen, "Add Task",
                   s_mic_bitmap, s_mic_white_bitmap, ic);
      return;
    }
#endif

    // Reflects live sync status so a resync failure is visible while the
    // cached list still shows, instead of being silently swallowed.
    static char s_resync_subtitle[MAX_STATUS_MSG_LEN + 16];
    const char *subtitle = "Synced";
    switch (s_status_code) {
      case STATUS_SYNCING:
        // s_status_msg carries the phone's "Decrypting N%" during the slow
        // E2EE replay (index.js's sendStatus); empty once past it.
        subtitle = s_status_msg[0] != '\0' ? s_status_msg : "Syncing...";
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
    // On emery it flashes green for SYNC_CHECK_MS right after a clean sync.
    GColor resync_bg = GColorRed;
#ifdef PBL_PLATFORM_EMERY
    if (s_sync_check_active) {
      resync_bg = GColorIslamicGreen;
    }
#endif
    fill_bg(ctx, bounds, resync_bg);
    graphics_context_set_text_color(ctx, is_selected ? GColorWhite : GColorBlack);
    GRect title_box = GRect(TITLE_BOX_X, ROW_TITLE_TOP_Y(bounds.size.h, HEADING_TITLE_H, CHROME_STRIP_H),
                             bounds.size.w - TITLE_BOX_X * 2 - ROW_ICON_SIZE - 8, HEADING_TITLE_H);
    draw_text(ctx, "Resync", HEADING_FONT_KEY, title_box, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
    // Full row width - not truncating a "Failed: ..." status matters more than
    // dodging the icon, which sits up in the title band.
    GRect subtitle_box = GRect(TITLE_BOX_X, ROW_SUBTITLE_TOP_Y(bounds.size.h, HEADING_TITLE_H, CHROME_STRIP_H),
                                bounds.size.w - TITLE_BOX_X * 2, CHROME_STRIP_H);
    draw_text(ctx, subtitle, CHROME_FONT_KEY, subtitle_box, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
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
    fill_bg(ctx, bounds, bg);
    graphics_context_set_text_color(ctx, fg);
    GRect title_box = GRect(TITLE_BOX_X, ROW_TITLE_TOP_Y(bounds.size.h, HEADING_TITLE_H, CHROME_STRIP_H),
                             bounds.size.w - TITLE_BOX_X * 2, HEADING_TITLE_H);
    draw_text(ctx, "Finish Day", HEADING_FONT_KEY, title_box, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter);
    // Version text as this row's subtitle - the same footer slot it's lived in
    // since v0.6.5, just no longer alone.
    GRect subtitle_box = GRect(TITLE_BOX_X, ROW_SUBTITLE_TOP_Y(bounds.size.h, HEADING_TITLE_H, CHROME_STRIP_H),
                                bounds.size.w - TITLE_BOX_X * 2, CHROME_STRIP_H);
    draw_text(ctx, "v" APP_VERSION, CHROME_FONT_KEY, subtitle_box, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter);
#else
    // Plain version-only footer - no tap/long-select action on aplite.
    GRect bounds = layer_get_bounds(cell_layer);
    fill_bg(ctx, bounds, GColorWhite);
    graphics_context_set_text_color(ctx, GColorBlack);
    draw_text(ctx, "v" APP_VERSION, FONT_KEY_GOTHIC_14, bounds, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter);
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

    fill_bg(ctx, bounds, GColorGreen);
#if TODAY_PROJECT_SWATCH
    int16_t text_x = draw_project_marker(ctx, TITLE_BOX_X, bounds.size.h,
                                          project_row->project_id,
                                          s_tasks[project_row->start].project_color, is_selected);
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
    fill_bg(ctx, bounds, bg);
    graphics_context_set_text_color(ctx, fg);
    GRect title_box = GRect(TITLE_BOX_X, ROW_TITLE_TOP_Y(bounds.size.h, HEADING_TITLE_H, CHROME_STRIP_H),
                             bounds.size.w - TITLE_BOX_X * 2, HEADING_TITLE_H);
    GFont pinned_font = fonts_get_system_font(TITLE_FONT_KEY);
    const char *pinned_title = s_presence_task[0] != '\0' ? s_presence_task : "Live tracking";
    draw_marquee_title(ctx, title_box, pinned_title, pinned_font,
                        is_selected && s_presence_task[0] != '\0' &&
                        title_natural_width_font(pinned_title, pinned_font) > title_box.size.w);
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
    draw_text(ctx, sub, SUBTITLE_FONT_KEY, subtitle_box, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
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
    case MSG_TASK_PLAN_TODAY:
    case MSG_TASK_UNSCHEDULE:
      dict_write_cstring(iter, KEY_TASK_ID, s_retry_str);
      if (s_retry_str2[0] != '\0') {
        dict_write_cstring(iter, KEY_PROJECT_ID, s_retry_str2);
      }
      break;
    case MSG_TASK_SET_BACKLOG:
      dict_write_cstring(iter, KEY_TASK_ID, s_retry_str);
      if (s_retry_str2[0] != '\0') {
        dict_write_cstring(iter, KEY_PROJECT_ID, s_retry_str2);
      }
      dict_write_int32(iter, KEY_PROJECT_TASK_BACKLOG, s_retry_int);
      break;
    case MSG_TASK_SET_ESTIMATE:
      dict_write_cstring(iter, KEY_TASK_ID, s_retry_str);
      dict_write_int32(iter, KEY_TASK_TIME_ESTIMATE_MS, s_retry_int);
      break;
    case MSG_TASK_SET_DEADLINE:
      dict_write_cstring(iter, KEY_TASK_ID, s_retry_str);
      dict_write_int32(iter, KEY_TASK_DEADLINE_DAYS, s_retry_int);
      break;
    case MSG_TASK_SET_DUE_TIME:
      dict_write_cstring(iter, KEY_TASK_ID, s_retry_str);
      dict_write_int32(iter, KEY_TASK_DUE_MIN, s_retry_int);
      break;
    case MSG_TASK_MOVE_PROJECT:
      dict_write_cstring(iter, KEY_TASK_ID, s_retry_str);
      dict_write_cstring(iter, KEY_PROJECT_ID, s_retry_str2); // target project
      break;
    case MSG_TASK_TOGGLE_CHECK:
      dict_write_cstring(iter, KEY_TASK_ID, s_retry_str);
      dict_write_int32(iter, KEY_CHECK_INDEX, s_retry_int >> 1); // index<<1 | checked
      dict_write_int32(iter, KEY_CHECK_VALUE, s_retry_int & 1);
      break;
    case MSG_METRIC_ENERGY:
      dict_write_int32(iter, KEY_METRIC_ENERGY, s_retry_int);
      break;
    case MSG_TASK_REPEAT_REQUEST:
      dict_write_cstring(iter, KEY_TASK_ID, s_retry_str);
      break;
    case MSG_TASK_REPEAT_PAUSE:
      dict_write_cstring(iter, KEY_TASK_ID, s_retry_str);
      dict_write_int32(iter, KEY_TASK_REPEAT_PAUSED, s_retry_int);
      break;
    case MSG_METRIC_RATING:
      dict_write_int32(iter, KEY_METRIC_RATING, s_retry_int);
      break;
    case MSG_METRIC_REFLECT:
      dict_write_cstring(iter, KEY_METRIC_REFLECT_TEXT, s_retry_str);
      break;
    case MSG_TASK_TOGGLE_TAG:
      dict_write_cstring(iter, KEY_TASK_ID, s_retry_str);
      dict_write_cstring(iter, KEY_PROJECT_ID, s_retry_str2); // the tag id
      dict_write_int32(iter, KEY_PROJECT_TASK_BACKLOG, s_retry_int); // 1 = assign
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
      dict_write_int32(iter, KEY_IS_TAGS, s_retry_int);
      break;
    case MSG_PROJECT_LIST_REQUEST:
      dict_write_int32(iter, KEY_IS_TAGS, s_retry_int);
      if (s_retry_str[0] != '\0') {
        dict_write_cstring(iter, KEY_TASK_ID, s_retry_str); // BROWSE_TAG_EDIT
      }
      break;
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
    str_copy(s_retry_str, str_val, sizeof(s_retry_str));
  } else {
    s_retry_str[0] = '\0';
  }
  if (str_val2) {
    str_copy(s_retry_str2, str_val2, sizeof(s_retry_str2));
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
// Schedule the task for today / tomorrow, or clear its scheduling. The phone
// turns this into an updateTask op and pushes a fresh list back - the task may
// leave or join a Today-only view. aplite-excluded with the gesture.
static void send_task_reschedule(const char *task_id, RescheduleKind kind, const char *project_id) {
  const char *pid = (project_id && project_id[0] != '\0') ? project_id : NULL;
  if (kind == RESCHEDULE_TO_BACKLOG || kind == RESCHEDULE_FROM_BACKLOG) {
    // int_val carries the direction (1 = into the backlog); see send_pending_retry.
    begin_send(MSG_TASK_SET_BACKLOG, task_id, pid, kind == RESCHEDULE_TO_BACKLOG ? 1 : 0);
    return;
  }
  int msg_type = kind == RESCHEDULE_TODAY ? MSG_TASK_PLAN_TODAY
                 : kind == RESCHEDULE_TOMORROW ? MSG_TASK_PLAN_TOMORROW
                 : MSG_TASK_UNSCHEDULE;
  begin_send(msg_type, task_id, pid, 0);
}

// Set (or clear, ms == 0) a task's time estimate. The phone turns it into a
// plain updateTask op and pushes a fresh list back. int_val carries the ms.
static void send_task_set_estimate(const char *task_id, int32_t ms) {
  begin_send(MSG_TASK_SET_ESTIMATE, task_id, NULL, ms);
}

// Set a task's deadline to `days` from today, or clear it (days < 0).
static void send_task_set_deadline(const char *task_id, int32_t days) {
  begin_send(MSG_TASK_SET_DEADLINE, task_id, NULL, days);
}

// Add (assign != 0) or remove a tag from a task.
static void send_toggle_tag(const char *task_id, const char *tag_id, int32_t assign) {
  begin_send(MSG_TASK_TOGGLE_TAG, task_id, tag_id, assign);
}

// Schedule a task at `hour`:00 (the phone picks the day).
static void send_task_set_due_time(const char *task_id, int hour) {
  begin_send(MSG_TASK_SET_DUE_TIME, task_id, NULL, hour * 60);
}

static void send_move_to_project(const char *task_id, const char *project_id) {
  begin_send(MSG_TASK_MOVE_PROJECT, task_id, project_id, 0);
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
    if (s_dictation_target == DICT_NOTE_APPEND) {
      send_note_append(s_notes_overlay_subject_id, transcription, s_notes_overlay_is_project);
    } else if (s_dictation_target == DICT_REFLECT) {
      begin_send(MSG_METRIC_REFLECT, transcription, NULL, 0);
      s_reflect_note_set = true;
      if (s_reflect_menu) {
        menu_layer_reload_data(s_reflect_menu);
      }
    } else {
      send_task_add(transcription);
#ifdef PBL_PLATFORM_EMERY
      s_addtask_flash_active = true;
      s_addtask_flash_tick = 0;
      if (s_menu_layer) {
        menu_layer_reload_data(s_menu_layer);
        refresh_scroll_state(false);
      }
#endif
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
  s_dictation_target = DICT_ADD_TASK;
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
  s_dictation_target = DICT_NOTE_APPEND;
  s_dictation_pending = true;
  dictation_session_start(s_dictation_session);
}

// Reflect window "improvement" row - dictate one thing to improve; the callback
// routes it to MSG_METRIC_REFLECT (today's metric.reflections[0]).
static void start_reflect_dictation(void) {
  if (s_dictation_pending || !s_dictation_session) {
    return;
  }
  s_dictation_target = DICT_REFLECT;
  s_dictation_pending = true;
  dictation_session_start(s_dictation_session);
}
#endif

#ifndef PBL_PLATFORM_APLITE
#ifdef PBL_PLATFORM_EMERY
// Slide-out finished (or was interrupted) - actually hide the strip and park its
// frame back at rest so the next show_top_banner starts clean.
static void banner_anim_stopped(Animation *anim, bool finished, void *ctx) {
  if (s_overtime_banner_layer) {
    Layer *bl = text_layer_get_layer(s_overtime_banner_layer);
    layer_set_hidden(bl, true);
    layer_set_frame(bl, s_banner_frame);
  }
}

// Animates the top banner strip down into view (in) or back up out of view.
// The strip briefly sweeps past the status bar - same as a system notification.
// emery only: the 144px platforms have no code-space headroom for it and fall
// back to an instant show / hide.
static void banner_slide(bool in) {
  if (!s_overtime_banner_layer) {
    return;
  }
  Layer *bl = text_layer_get_layer(s_overtime_banner_layer);
  animation_unschedule_all(); // a re-fire mid-slide must not leave a stale handler
  GRect rest = s_banner_frame;
  GRect off = rest;
  off.origin.y -= rest.size.h; // just clear of the content top
  GRect from = in ? off : rest;
  GRect to = in ? rest : off;
  layer_set_frame(bl, from);
  PropertyAnimation *pa = property_animation_create_layer_frame(bl, &from, &to);
  if (!pa) {
    layer_set_frame(bl, to);
    if (!in) {
      banner_anim_stopped(NULL, true, NULL);
    }
    return;
  }
  Animation *a = property_animation_get_animation(pa);
  animation_set_duration(a, 200);
  animation_set_curve(a, in ? AnimationCurveEaseOut : AnimationCurveEaseIn);
  if (!in) {
    animation_set_handlers(a, (AnimationHandlers) { .stopped = banner_anim_stopped }, NULL);
  }
  animation_schedule(a);
}
#else
static void banner_slide(bool in) {
  if (s_overtime_banner_layer) {
    layer_set_hidden(text_layer_get_layer(s_overtime_banner_layer), !in);
  }
}
#endif

static void overtime_banner_timeout_callback(void *data) {
  s_overtime_banner_timer = NULL;
  banner_slide(false);
}

// Hides the over-estimate banner and cancels its auto-dismiss timer - safe to
// call whether or not it's showing.
static void hide_overtime_banner(void) {
  if (s_overtime_banner_timer) {
    app_timer_cancel(s_overtime_banner_timer);
    s_overtime_banner_timer = NULL;
  }
  if (s_overtime_banner_layer) {
    Layer *bl = text_layer_get_layer(s_overtime_banner_layer);
#ifdef PBL_PLATFORM_EMERY
    animation_unschedule_all(); // drop any in-flight slide before we hide / destroy
    layer_set_frame(bl, s_banner_frame);
#endif
    layer_set_hidden(bl, true);
  }
}

// Short rising two-note chime played with any banner when s_audible_notify is
// on. Only compiled on speaker hardware (Pebble Time 2); a no-op call otherwise.
// Skipped when the user muted the watch system-wide.
static void banner_ping(void) {
#ifdef PBL_SPEAKER
  if (speaker_is_muted()) {
    return;
  }
  static const SpeakerNote notes[] = {
    { .midi_note = 84, .waveform = SpeakerWaveformSine, .duration_ms = 90, .velocity = 0 },
    { .midi_note = 88, .waveform = SpeakerWaveformSine, .duration_ms = 110, .velocity = 0 },
  };
  speaker_play_notes(notes, ARRAY_LENGTH(notes),
                     s_audible_volume < 0 ? 0 : (s_audible_volume > 100 ? 100 : s_audible_volume));
#endif
}

// Shows `text` in the top banner strip with a double vibe (and a ping when
// audible notifications are on), and (re)arms the auto-dismiss timer. `text`
// must stay valid until the banner hides - s_overtime_banner_text or a string
// literal. Shared by the over-estimate / break / idle / task-due banners; only
// one shows at a time (last writer wins).
static void show_top_banner(const char *text) {
  if (!s_overtime_banner_layer) {
    return;
  }
  text_layer_set_text(s_overtime_banner_layer, text);
  layer_set_hidden(text_layer_get_layer(s_overtime_banner_layer), false);
  banner_slide(true); // slides down from the top, then rests
  vibes_double_pulse();
  if (s_audible_notify) {
    banner_ping();
  }
  if (s_overtime_banner_timer) {
    app_timer_cancel(s_overtime_banner_timer);
  }
  s_overtime_banner_timer = app_timer_register(OVERTIME_BANNER_MS, overtime_banner_timeout_callback, NULL);
}

static void show_overtime_banner(const char *task_title) {
  snprintf(s_overtime_banner_text, sizeof(s_overtime_banner_text),
            "Over estimate\n%s", task_title);
  show_top_banner(s_overtime_banner_text);
}

// Called once per tracking tick: fires the over-estimate banner the first time
// effective time (synced spent + the running session's elapsed) reaches the
// estimate. Covers both a task tracked ON this watch and one tracked on another
// device (remote presence, state 1) when the phone has sent that task's synced
// spent + estimate. Latched via s_overtime_notified (re-armed if effective time
// drops back under). With the "repeat every 5 minutes" sub-option, re-fires
// every OVERTIME_REPEAT_INTERVAL_S while the task stays over.
static void maybe_notify_overtime(void) {
  if (!s_overtime_notify_enabled) {
    return;
  }

  const char *over_title;
  int effective_ms;
  int estimate_ms;
  int elapsed_s;

  if (s_tracking_task_id[0] != '\0') {
    Task *task = find_task_by_id(s_tracking_task_id);
    if (!task || task->time_estimate_ms <= 0) {
      return;
    }
    effective_ms = task->time_spent_ms;
    elapsed_s = (int)(time(NULL) - s_tracking_start_epoch);
    estimate_ms = task->time_estimate_ms;
    over_title = task->title;
  } else if (s_presence_state == 1 && s_presence_estimate_ms > 0) {
    // Another device is tracking; s_presence_spent_ms is that task's synced
    // time-spent, s_presence_elapsed_base the running session's start.
    effective_ms = s_presence_spent_ms;
    elapsed_s = (int)(time(NULL) - s_presence_elapsed_base);
    estimate_ms = s_presence_estimate_ms;
    over_title = s_presence_task[0] != '\0' ? s_presence_task : "Live tracking";
  } else {
    return;
  }
  if (elapsed_s > 0) {
    effective_ms += elapsed_s * 1000;
  }

  if (effective_ms < estimate_ms) {
    s_overtime_notified = false; // re-arm for a later crossing
    return;
  }
  if (s_error_overlay_active) {
    return;
  }
  if (!s_overtime_notified) {
    s_overtime_notified = true;
    s_overtime_last_notify_epoch = time(NULL);
    show_overtime_banner(over_title);
    return;
  }
  // Already notified this crossing - "repeat every 5 minutes" re-fires it.
  if (s_overtime_repeat_enabled &&
      time(NULL) - s_overtime_last_notify_epoch >= OVERTIME_REPEAT_INTERVAL_S) {
    s_overtime_last_notify_epoch = time(NULL);
    show_overtime_banner(over_title);
  }
}

#ifdef BREAK_REMINDER
// Called once per tracking tick, next to maybe_notify_overtime: fires the
// "time for a break" banner the first time this watch's banked tracked time
// since the last break (s_break_accum_s + the running session's elapsed)
// reaches s_break_reminder_min. Latched via s_break_notified until start_tracking
// sees a real gap. Local task tracking only.
static void maybe_notify_break(void) {
  if (s_tracking_task_id[0] == '\0' || s_break_reminder_min <= 0 || s_break_notified) {
    return;
  }
  int elapsed_s = (int)(time(NULL) - s_tracking_start_epoch);
  if (elapsed_s < 0) {
    elapsed_s = 0;
  }
  if (s_break_accum_s + elapsed_s < s_break_reminder_min * 60) {
    return;
  }
  if (s_error_overlay_active) {
    return;
  }
  s_break_notified = true;
  show_top_banner("Time for a break");
}
#endif

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
#ifdef BREAK_REMINDER
  maybe_notify_break();
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
  str_copy(s_tracking_task_id, task->id, MAX_ID_LEN);
  s_tracking_start_epoch = time(NULL);
  save_tracking();
#ifndef PBL_PLATFORM_APLITE
  // Fresh session - re-arm the over-estimate banner and clear any stale one.
  s_overtime_notified = false;
  s_overtime_last_notify_epoch = 0;
  hide_overtime_banner();
#ifdef BREAK_REMINDER
  // Break reminder: a long enough pause since the last session counts as a real
  // break - zero the running tally and re-arm the banner. A shorter gap just
  // carries the tally forward into this session.
  if (s_break_last_stop_epoch != 0 &&
      time(NULL) - s_break_last_stop_epoch >= BREAK_RESET_GAP_S) {
    s_break_accum_s = 0;
    s_break_notified = false;
    // This gap was a real break - record it for the Stats page.
    break_roll_day();
    int gap = (int)(time(NULL) - s_break_last_stop_epoch);
    s_break_count_today++;
    s_break_total_s_today += gap > BREAK_MAX_S ? BREAK_MAX_S : gap;
  }
  save_break_state();
  // Fresh "not tracking" window for the next gap.
  s_untracked_notify_count = 0;
#endif
  // Pin this task to the top (if enabled); cancel any grace timer from a
  // just-stopped task and re-lay-out the list.
  cancel_unpin_timer();
  str_copy(s_pinned_task_id, task->id, MAX_ID_LEN);
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
// session up to `end_epoch` for upload (handleTrackTimeStop in index.js).
// stop_tracking_and_report() passes now; the midnight auto-stop passes the
// day boundary so the overnight run isn't logged.
static void stop_tracking_at(time_t end_epoch) {
  if (s_tracking_task_id[0] == '\0') {
    return;
  }
  time_t elapsed_s = end_epoch - s_tracking_start_epoch;
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
  // A focus session only wraps a local track - stopping the timer ends it too
  // (drops the keepalive timer, releases the backlight). Silent: the stop
  // itself is the user's cue.
  if (s_focus_end_epoch != 0) {
    focus_end(false);
  }
  s_overtime_notified = false;
  s_overtime_last_notify_epoch = 0;
  hide_overtime_banner();
#ifdef BREAK_REMINDER
  // Bank this session's tracked time and stamp the stop, so the next start can
  // tell a real break from a brief pause. s_break_notified is deliberately kept
  // - a quick stop/restart shouldn't clear an already-shown break banner.
  s_break_accum_s += elapsed_s > 0 ? (int)elapsed_s : 0;
  s_break_last_stop_epoch = time(NULL);
  save_break_state();
#endif
  // Keep the just-stopped task pinned for a short grace period so it slides
  // back into its group smoothly. A new start_tracking() cancels this.
  if (s_pinned_task_id[0] != '\0') {
    cancel_unpin_timer();
    s_unpin_timer = app_timer_register(UNPIN_GRACE_MS, unpin_timer_callback, NULL);
  }
  live_window_refresh(); // pop the detail screen if it was open on this session
#endif
  stop_tracking_tick();
}

static void stop_tracking_and_report(void) {
  stop_tracking_at(time(NULL));
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
  // A pending move-to-tomorrow / unschedule / "Marking done..." is cancelled by
  // a physical Select during its window - checked before the normal row handling
  // so the press only cancels (no toggle, no notes).
  if (s_pending_reschedule_kind != RESCHEDULE_NONE) {
    cancel_pending_reschedule();
    return;
  }
#ifdef PBL_PLATFORM_EMERY
  if (s_pending_done_task_id[0] != '\0') {
    Task *pd_sel = resolve_selected_task();
    if (pd_sel && strncmp(pd_sel->id, s_pending_done_task_id, MAX_ID_LEN) == 0) {
      cancel_pending_done(); // Select on the same row within the window = undo
      return;
    }
    // Selection has moved on - commit the pending one now and let this press
    // fall through to start the newly-selected row's own toggle.
    if (s_pending_done_timer) {
      app_timer_cancel(s_pending_done_timer);
      pending_done_commit_callback(NULL);
    }
  }
#endif
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
      s_browse_mode = BROWSE_PROJECTS;
      push_browse_window(NULL);
    } else if (kind == SECTION0_ROW_TAGS) {
      s_browse_mode = BROWSE_TAGS;
      push_browse_window(NULL);
#endif
#ifndef PBL_PLATFORM_APLITE
    } else if (kind == SECTION0_ROW_STATS) {
      push_stats_window();
    } else if (kind == SECTION0_ROW_SCHEDULE) {
      push_schedule_window();
    } else if (kind == SECTION0_ROW_UPCOMING) {
      push_page_window(PAGE_UPCOMING);
    } else if (kind == SECTION0_ROW_NOTESPAGE) {
      push_page_window(PAGE_NOTES);
    } else if (kind == SECTION0_ROW_ADD_TASK) {
      start_add_task_dictation();
#endif
    } else {
      request_sync(); // the "Resync" row
    }
    return;
  }
#ifndef PBL_PLATFORM_APLITE
  // The pinned "TRACKING" row (section 1) - Select opens the full-screen
  // tracking detail: the local session, or a remote device's (same screen the
  // dark-blue LIVE row opens). Select there stops the timer.
  if (cell_index->section == 1 && (s_tracking_task_id[0] != '\0' || remote_in_pinned_section())) {
    push_live_window();
    return;
  }
  // The project row - Select opens that project's tasks in the browser,
  // long-Select opens its notes (menu_select_long_click). Same split the
  // browser's own project list uses.
  TaskGroup *project_row = resolve_project_row_at(*cell_index);
  if (project_row) {
#if PROJECTS_BROWSER
    s_browse_mode = BROWSE_PROJECTS;
    push_browse_window(project_row->project_id);
#endif
    return;
  }
  // The Finish Day row: Select opens the Reflect energy check-in when it's
  // enabled (long-Select still archives - menu_select_long_click). The row
  // resolves to no task, so this must come before resolve_task_at's NULL.
  if (s_reflect_enabled &&
      (int)cell_index->section - GROUP_SECTION_BASE == s_group_count) {
    push_reflect_window();
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
  str_copy(s_pending_toggle_task_id, task->id, MAX_ID_LEN);
  s_pending_toggle_timer = app_timer_register(DOUBLE_CLICK_WINDOW_MS, pending_toggle_timer_callback, NULL);
#else
  task->done = !task->done;
  save_tasks();
  menu_layer_reload_data(s_menu_layer);
  send_task_toggle(task);
#endif
}

// Long-select on a task row opens its action menu (non-aplite) - track, notes,
// tags, estimate, deadline. aplite has no menu window budget, so there it's the
// plain track toggle it always was.
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
  if (!task) {
    return;
  }
#ifndef PBL_PLATFORM_APLITE
  push_action_menu(task->id, ACTX_TODAY, false);
#else
  if (task->done) {
    return; // tracking a completed task isn't a real scenario
  }
  bool already_tracking_this = s_tracking_task_id[0] != '\0' &&
                                strncmp(s_tracking_task_id, task->id, MAX_ID_LEN) == 0;
  stop_tracking_and_report();
  if (!already_tracking_this) {
    start_tracking(task);
  }
  menu_layer_reload_data(s_menu_layer);
#endif
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
// empty-state layers under it.
//
// With a cached list to fall back on, a sync failure doesn't blank the
// screen: the tasks stay visible and the red Resync row ("Sync failed" /
// "Failed: ...", Select on it to retry) carries the error - see
// update_empty_layer's own "resync with a populated list" path. The
// fullscreen overlay is only for when there's nothing else to show.
static void show_error_overlay(void) {
  if (s_task_count > 0) {
    s_error_overlay_active = false;
    layer_set_hidden(text_layer_get_layer(s_error_layer), true);
    menu_layer_reload_data(s_menu_layer); // refresh the Resync row subtitle
    update_empty_layer();
    return;
  }
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
  str_copy(s_notes_overlay_subject_id, id, MAX_ID_LEN);
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
  str_copy(s_notes_tags_line, task->tags, sizeof(s_notes_tags_line));
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
#ifdef PBL_PLATFORM_EMERY
  // Marking a task done opens its own 5s cancel window rather than committing
  // now; un-completing one is immediate. emery only.
  if (!task->done) {
    begin_pending_done(task->id);
    return;
  }
#endif
  task->done = !task->done;
  save_tasks();
  menu_layer_reload_data(s_menu_layer);
  send_task_toggle(task);
  refresh_scroll_state(false);
}

#ifdef PBL_PLATFORM_EMERY
// Opens (or restarts) the "Marking done..." cancel window for a task by id. The
// task is not marked done until pending_done_commit_callback fires; a Select in
// the meantime calls cancel_pending_done.
static void begin_pending_done(const char *task_id) {
  if (s_pending_done_timer) {
    app_timer_cancel(s_pending_done_timer);
  }
  str_copy(s_pending_done_task_id, task_id, MAX_ID_LEN);
  s_pending_done_tick = 0;
  s_pending_done_timer = app_timer_register(DONE_WINDOW_MS, pending_done_commit_callback, NULL);
  vibes_short_pulse();
  if (s_menu_layer) {
    menu_layer_reload_data(s_menu_layer);
  }
  refresh_scroll_state(false);
}

// Cancel window elapsed: mark the task done for real and sync it.
static void pending_done_commit_callback(void *data) {
  s_pending_done_timer = NULL;
  Task *task = find_task_by_id(s_pending_done_task_id);
  s_pending_done_task_id[0] = '\0';
  s_pending_done_tick = 0;
  if (task && !task->done) {
    task->done = true;
    save_tasks();
    send_task_toggle(task);
  }
  if (s_menu_layer) {
    menu_layer_reload_data(s_menu_layer);
  }
  refresh_scroll_state(false);
}

// Select pressed inside the window - drop it, the task stays not-done.
static void cancel_pending_done(void) {
  if (s_pending_done_timer) {
    app_timer_cancel(s_pending_done_timer);
    s_pending_done_timer = NULL;
  }
  s_pending_done_task_id[0] = '\0';
  s_pending_done_tick = 0;
  if (s_menu_layer) {
    menu_layer_reload_data(s_menu_layer);
  }
  refresh_scroll_state(false);
}
#endif


// The pending-reschedule subtitle rides draw_task_row, which both the today
// list and the Projects browser task view use - redraw whichever menus exist.
static void reschedule_menus_reload(void) {
  if (s_menu_layer) {
    menu_layer_reload_data(s_menu_layer);
  }
#if PROJECTS_BROWSER
  if (s_browse_menu) {
    menu_layer_reload_data(s_browse_menu);
  }
#endif
}

// Clears a pending schedule-today / tomorrow / unschedule (Select pressed
// within the window, or the window's own task vanished) and redraws so the
// subtitle reverts.
static void cancel_pending_reschedule(void) {
  if (s_pending_reschedule_timer) {
    app_timer_cancel(s_pending_reschedule_timer);
    s_pending_reschedule_timer = NULL;
  }
  s_pending_reschedule_kind = RESCHEDULE_NONE;
  s_pending_reschedule_tick = 0;
  s_pending_reschedule_task_id[0] = '\0';
  s_pending_reschedule_project_id[0] = '\0';
  reschedule_menus_reload();
  refresh_scroll_state(false);
}

// Commits the pending reschedule once its cancel window passes. Sends by the
// stashed id (the phone ignores a since-deleted task) - the task need not be
// in s_tasks, since a browser task usually isn't.
static void pending_reschedule_timer_callback(void *data) {
  s_pending_reschedule_timer = NULL;
  RescheduleKind kind = s_pending_reschedule_kind;
  s_pending_reschedule_kind = RESCHEDULE_NONE;
#ifdef PBL_PLATFORM_EMERY
  if (kind == RESCHEDULE_AT && s_pending_reschedule_task_id[0] != '\0') {
    send_task_set_due_time(s_pending_reschedule_task_id, s_pending_reschedule_at_hour);
  } else
#endif
  if (kind != RESCHEDULE_NONE && s_pending_reschedule_task_id[0] != '\0') {
    send_task_reschedule(s_pending_reschedule_task_id, kind, s_pending_reschedule_project_id);
  }
  s_pending_reschedule_task_id[0] = '\0';
  s_pending_reschedule_project_id[0] = '\0';
  s_pending_reschedule_tick = 0;
  // Drop the pending subtitle now; the phone's list push handles the rest.
  reschedule_menus_reload();
  refresh_scroll_state(false);
}

// Starts (or replaces) the pending reschedule for the currently-selected task -
// on the today list or the Projects browser's task view, whichever is on top.
// Long-press Up = schedule today (browser) / unschedule (today list), long-
// press Down = tomorrow on both.
static void begin_pending_reschedule(RescheduleKind kind) {
  if (s_error_overlay_active || kind == RESCHEDULE_NONE) {
    return;
  }
  const char *task_id = NULL;
  s_pending_reschedule_project_id[0] = '\0';
#if PROJECTS_BROWSER
  if (window_stack_get_top_window() == s_browse_window && s_browse_level == 1 && s_browse_menu) {
    Task *bt = resolve_browse_task_at(menu_layer_get_selected_index(s_browse_menu));
    if (bt) {
      task_id = bt->id;
      // Only a real project id round-trips (the phone re-pushes that project's
      // list). In tags mode s_browse_project_id is a tag - leave it empty so the
      // phone re-pushes the today list instead.
      if (!browse_wants_tags()) {
        str_copy(s_pending_reschedule_project_id, s_browse_project_id, MAX_PROJECT_ID_LEN);
      }
    }
  } else
#endif
  {
    if (s_task_count == 0) {
      return;
    }
    Task *task = resolve_selected_task();
    if (task) {
      task_id = task->id; // NULL on a pinned/project/action row
    }
  }
  if (!task_id) {
    return;
  }
  // A pending done-toggle on the same tap sequence would otherwise commit
  // mid-window - drop it in favour of this.
  if (s_pending_toggle_timer) {
    app_timer_cancel(s_pending_toggle_timer);
    s_pending_toggle_timer = NULL;
    s_pending_toggle_task_id[0] = '\0';
  }
#ifdef PBL_PLATFORM_EMERY
  if (s_pending_done_timer) {
    app_timer_cancel(s_pending_done_timer);
    s_pending_done_timer = NULL;
    s_pending_done_task_id[0] = '\0';
    s_pending_done_tick = 0;
  }
#endif
  if (s_pending_reschedule_timer) {
    app_timer_cancel(s_pending_reschedule_timer);
  }
  str_copy(s_pending_reschedule_task_id, task_id, MAX_ID_LEN);
  s_pending_reschedule_kind = kind;
  s_pending_reschedule_tick = 0;
  s_pending_reschedule_timer =
      app_timer_register(RESCHEDULE_WINDOW_MS, pending_reschedule_timer_callback, NULL);
  vibes_short_pulse();
  reschedule_menus_reload();
  refresh_scroll_state(false);
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
// int_val carries IS_TAGS (1 for any tag-flavoured mode) - see send_pending_retry
// and the phone's handleProjectListRequest / handleProjectTasksRequest. In
// BROWSE_TAG_EDIT the str_val is the task id whose tags are being edited (the
// phone then marks each tag assigned/not in PROJECT_TASK_COUNT).
static void request_project_list(void) {
  const char *edit_id = (s_browse_mode == BROWSE_TAG_EDIT && s_browse_edit_task_id[0] != '\0')
                            ? s_browse_edit_task_id : NULL;
  begin_send(MSG_PROJECT_LIST_REQUEST, edit_id, NULL, browse_wants_tags() ? 1 : 0);
}

static void request_project_tasks(const char *project_id) {
  begin_send(MSG_PROJECT_TASKS_REQUEST, project_id, NULL, browse_wants_tags() ? 1 : 0);
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
#ifdef PBL_PLATFORM_EMERY
  // A sync just finished cleanly - flash the Resync row green.
  if (s_status_code == STATUS_SYNCING && new_status_code == STATUS_OK) {
    s_sync_check_active = true;
    s_sync_check_tick = 0;
    if (s_menu_layer) {
      menu_layer_reload_data(s_menu_layer);
      refresh_scroll_state(false);
    }
  }
#endif
  s_status_code = new_status_code;
}

// The TASK_* fields shared by MSG_TASK_ITEM and MSG_PROJECT_TASKS_ITEM. The
// caller has already validated id_tuple / title_tuple non-NULL and bounds-
// checked idx; MSG_TASK_ITEM fills in project/tags/colour separately.
static void parse_common_task_fields(DictionaryIterator *it, Task *dst,
                                     Tuple *id_tuple, Tuple *title_tuple) {
  str_copy(dst->id, id_tuple->value->cstring, MAX_ID_LEN);
  str_copy(dst->title, title_tuple->value->cstring, MAX_TITLE_LEN);
  dst->done = tuple_int(it, KEY_TASK_DONE, 0) != 0;
  dst->due_min = tuple_int(it, KEY_TASK_DUE_MIN, -1);
  dst->time_spent_ms = tuple_int(it, KEY_TASK_TIME_SPENT_MS, 0);
  dst->time_estimate_ms = tuple_int(it, KEY_TASK_TIME_ESTIMATE_MS, 0);
  dst->deadline_days = tuple_int(it, KEY_TASK_DEADLINE_DAYS, DEADLINE_NONE);
  dst->recurs = tuple_int(it, KEY_TASK_RECURS, 0) != 0;
#ifndef PBL_PLATFORM_APLITE
  dst->remind_min = tuple_int(it, KEY_TASK_REMIND_MIN, -1);
  dst->remind_fired = false;
  str_copy(dst->issue_key, tuple_str(it, KEY_TASK_ISSUE_KEY, ""), sizeof(dst->issue_key));
#endif
}

static void inbox_received_handler(DictionaryIterator *iterator, void *context) {
  Tuple *type_tuple = dict_find(iterator, KEY_MSG_TYPE);
  if (!type_tuple) {
    return;
  }

  switch (type_tuple->value->int32) {
    case MSG_TASK_SYNC_START: {
      s_incoming_total = tuple_int(iterator, KEY_TASK_TOTAL, 0);
      if (s_incoming_total > MAX_TASKS) {
        s_incoming_total = MAX_TASKS;
      }
#if INCOMING_MALLOCED
      // A previous batch that never reached SYNC_END would have leaked it.
      if (s_incoming && s_incoming != s_tasks) {
        free(s_incoming);
      }
      s_incoming = malloc(sizeof(Task) * MAX_TASKS);
      if (!s_incoming) {
        s_incoming = s_tasks; // OOM: parse in place, accept a brief torn list
      }
#endif
      set_status_code(STATUS_SYNCING);
      break;
    }
    case MSG_TASK_ITEM: {
      Tuple *idx_tuple = dict_find(iterator, KEY_TASK_INDEX);
      Tuple *id_tuple = dict_find(iterator, KEY_TASK_ID);
      Tuple *title_tuple = dict_find(iterator, KEY_TASK_TITLE);
      if (!idx_tuple || !id_tuple || !title_tuple) {
        break;
      }
#if INCOMING_MALLOCED
      if (!s_incoming) {
        break; // an ITEM with no preceding SYNC_START (stale batch)
      }
#endif
      int idx = idx_tuple->value->int32;
      if (idx < 0 || idx >= MAX_TASKS) {
        break;
      }
      // Key absent (not 0, a valid 12:00am) means "no dueWithTime" - the phone
      // only sends a field when the task has it. parse_common_task_fields
      // handles id/title/done/due/spent/estimate; project/tags/colour here.
      parse_common_task_fields(iterator, &s_incoming[idx], id_tuple, title_tuple);
      str_copy(s_incoming[idx].project, tuple_str(iterator, KEY_TASK_PROJECT, ""), MAX_PROJECT_LEN);
#ifndef PBL_PLATFORM_APLITE
      str_copy(s_incoming[idx].project_id, tuple_str(iterator, KEY_TASK_PROJECT_ID, ""), MAX_PROJECT_ID_LEN);
      str_copy(s_incoming[idx].tags, tuple_str(iterator, KEY_TASK_TAGS, ""), MAX_TASK_TAGS_LEN);
#if TODAY_PROJECT_SWATCH
      s_incoming[idx].project_color = (uint8_t)tuple_int(iterator, KEY_TASK_PROJECT_COLOR, 0);
#endif
#endif
      break;
    }
    case MSG_TASK_SYNC_END: {
      int count = s_incoming_total < MAX_TASKS ? s_incoming_total : MAX_TASKS;
#if INCOMING_MALLOCED
      bool committed = (s_incoming == s_tasks); // OOM path parsed in place
      if (s_incoming && s_incoming != s_tasks) {
        memcpy(s_tasks, s_incoming, sizeof(Task) * (size_t)count);
        free(s_incoming);
        committed = true;
      }
      s_incoming = NULL;
      if (committed) {
        s_task_count = count; // a stale END with no active batch leaves the list alone
      }
#else
      memcpy(s_tasks, s_incoming, sizeof(Task) * (size_t)count);
      s_task_count = count;
#endif
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
#ifndef PBL_PLATFORM_APLITE
      schedule_refresh_if_open(); // rebuild the Schedule sub-page off the new list
#endif
      break;
    }
    case MSG_HABIT_SYNC_START: {
      s_habit_incoming_total = tuple_int(iterator, KEY_HABIT_TOTAL, 0);
      if (s_habit_incoming_total > MAX_HABITS) {
        s_habit_incoming_total = MAX_HABITS;
      }
      break;
    }
    case MSG_HABIT_ITEM: {
      Tuple *idx_tuple = dict_find(iterator, KEY_HABIT_INDEX);
      Tuple *id_tuple = dict_find(iterator, KEY_HABIT_ID);
      Tuple *title_tuple = dict_find(iterator, KEY_HABIT_TITLE);
      if (!idx_tuple || !id_tuple || !title_tuple) {
        break;
      }
      int idx = idx_tuple->value->int32;
      if (idx < 0 || idx >= MAX_HABITS) {
        break;
      }
      // Written directly into s_habits (no separate incoming buffer - safe, see
      // the Habit struct comment).
      str_copy(s_habits[idx].id, id_tuple->value->cstring, MAX_HABIT_ID_LEN);
      str_copy(s_habits[idx].title, title_tuple->value->cstring, MAX_TITLE_LEN);
      s_habits[idx].done = tuple_int(iterator, KEY_HABIT_DONE, 0) != 0;
      s_habits[idx].value = tuple_int(iterator, KEY_HABIT_VALUE, 0);
      s_habits[idx].goal = tuple_int(iterator, KEY_HABIT_GOAL, 0);
      // habit type: 0 = ClickCounter, 1 = StopWatch, 2 = RepeatedCountdownReminder.
      int habit_type = tuple_int(iterator, KEY_HABIT_TYPE, 0);
#ifdef PBL_PLATFORM_APLITE
      // aplite has no code budget for the countdown path, and both timer types
      // are excluded from its visible list identically, so collapse both into
      // is_stopwatch (is_countdown is never read there).
      s_habits[idx].is_stopwatch = habit_type != 0;
#else
      s_habits[idx].is_stopwatch = habit_type == 1;
      s_habits[idx].is_countdown = habit_type == 2;
      s_habits[idx].countdown_ms = tuple_int(iterator, KEY_HABIT_COUNTDOWN_MS, 0);
      s_habits[idx].streak = tuple_int(iterator, KEY_HABIT_STREAK, 0);
      s_habits[idx].best_streak = tuple_int(iterator, KEY_HABIT_BEST_STREAK, 0);
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
      // Drop a reply for the other mode (the user switched Projects<->Tags
      // while this list was in flight).
      if ((tuple_int(iterator, KEY_IS_TAGS, 0) != 0) != browse_wants_tags()) {
        break;
      }
      s_browse_project_incoming = tuple_int(iterator, KEY_PROJECT_TOTAL, 0);
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
      str_copy(s_browse_projects[idx].id, id_tuple->value->cstring, MAX_PROJECT_ID_LEN);
      str_copy(s_browse_projects[idx].title, title_tuple->value->cstring, MAX_TITLE_LEN);
      s_browse_projects[idx].color = tuple_int(iterator, KEY_PROJECT_COLOR, 0);
      s_browse_projects[idx].task_count = tuple_int(iterator, KEY_PROJECT_TASK_COUNT, 0);
      break;
    }
    case MSG_PROJECT_LIST_END: {
      if (!s_browse_projects) {
        break;
      }
      s_browse_project_count = s_browse_project_incoming;
      s_browse_projects_loading = false;
#if PROJECTS_CACHE
      if (s_browse_mode == BROWSE_PROJECTS) {
        save_browse_projects(); // the tag list isn't cached - always fetched
      }
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
      s_browse_task_incoming = tuple_int(iterator, KEY_TASK_TOTAL, 0);
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
      parse_common_task_fields(iterator, bt, id_tuple, title_tuple);
      // Project display name - shown right-aligned in the Tags view (a tag's
      // tasks span projects); redundant but harmless in the Projects view.
      str_copy(bt->project, tuple_str(iterator, KEY_TASK_PROJECT, ""), MAX_PROJECT_LEN);
      if (tuple_int(iterator, KEY_PROJECT_TASK_BACKLOG, 0) != 0 && idx < s_browse_backlog_start) {
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
#ifndef PBL_PLATFORM_APLITE
    case MSG_STATS_DATA: {
      s_stats_est_remaining_ms = tuple_int(iterator, KEY_STATS_EST_REMAINING_MS, 0);
      s_stats_worked_today_ms = tuple_int(iterator, KEY_STATS_WORKED_TODAY_MS, 0);
      s_stats_done_today = tuple_int(iterator, KEY_STATS_DONE_TODAY, 0);
      s_stats_worked_yesterday_ms = tuple_int(iterator, KEY_STATS_WORKED_YESTERDAY_MS, 0);
      s_stats_done_yesterday = tuple_int(iterator, KEY_STATS_DONE_YESTERDAY, 0);
      if (s_stats_projects) { // NULL = window closed; it'll re-request on open
        str_copy(s_stats_projects, tuple_str(iterator, KEY_STATS_TEXT, ""), STATS_TEXT_CAP);
        s_stats_have_data = true;
        stats_render();
      }
      break;
    }
    case MSG_UPCOMING_DATA: {
      if (s_upcoming_text && s_page_mode == PAGE_UPCOMING) {
        str_copy(s_upcoming_text, tuple_str(iterator, KEY_UPCOMING_TEXT, ""), PAGE_TEXT_CAP);
        s_upcoming_have_data = true;
        upcoming_render(); // no-op if the window was closed before the reply landed
      }
      break;
    }
    case MSG_NOTESPAGE_DATA: {
      if (s_upcoming_text && s_page_mode == PAGE_NOTES) {
        str_copy(s_upcoming_text, tuple_str(iterator, KEY_NOTESPAGE_TEXT, ""), PAGE_TEXT_CAP);
        s_upcoming_have_data = true;
        upcoming_render();
      }
      break;
    }
    case MSG_TASK_REPEAT_DATA: {
      handle_repeat_data(iterator);
      break;
    }
    case MSG_POMODORO_CFG: {
      int w = tuple_int(iterator, KEY_POMODORO_WORK_MIN, 0);
      int b = tuple_int(iterator, KEY_POMODORO_BREAK_MIN, 0);
      if (w > 0 && w < 600) {
        s_pomodoro_work_min = w;
      }
      if (b > 0 && b < 600) {
        s_pomodoro_break_min = b;
      }
      break;
    }
#endif
    case MSG_SYNC_STATUS: {
      Tuple *status_tuple = dict_find(iterator, KEY_STATUS_CODE);
      if (status_tuple) {
        set_status_code(status_tuple->value->int32);
      }
      str_copy(s_status_msg, tuple_str(iterator, KEY_STATUS_MSG, ""), MAX_STATUS_MSG_LEN);
      // Feature toggles from the phone's pairing settings - optional fields,
      // absent-means-unchanged (pass the current value as the fallback) so a
      // version mismatch can't reset a flag. Read before reload_data so a
      // change shows in the same redraw.
      s_habits_enabled = tuple_int(iterator, KEY_HABITS_ENABLED, s_habits_enabled) != 0;
      s_add_task_enabled = tuple_int(iterator, KEY_ADD_TASK_ENABLED, s_add_task_enabled) != 0;
      s_projects_enabled = tuple_int(iterator, KEY_PROJECTS_ENABLED, s_projects_enabled) != 0;
#ifndef PBL_PLATFORM_APLITE
      s_stats_enabled = tuple_int(iterator, KEY_STATS_ENABLED, s_stats_enabled) != 0;
      s_schedule_enabled = tuple_int(iterator, KEY_SCHEDULE_ENABLED, s_schedule_enabled) != 0;
      s_upcoming_enabled = tuple_int(iterator, KEY_UPCOMING_ENABLED, s_upcoming_enabled) != 0;
      s_notespage_enabled = tuple_int(iterator, KEY_NOTESPAGE_ENABLED, s_notespage_enabled) != 0;
      s_tags_enabled = tuple_int(iterator, KEY_TAGS_ENABLED, s_tags_enabled) != 0;
      s_yesterday_stats_enabled = tuple_int(iterator, KEY_YESTERDAY_STATS_ENABLED, s_yesterday_stats_enabled) != 0;
#endif
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
      s_audible_notify = tuple_int(iterator, KEY_AUDIBLE_NOTIFICATIONS, s_audible_notify) != 0;
      s_audible_volume = tuple_int(iterator, KEY_AUDIBLE_VOLUME, s_audible_volume);
#ifdef BREAK_REMINDER
      // "Remind me to take a break" interval in minutes (0 = off). Re-sent every
      // sync like the flags above; the tally itself lives in persist.
      s_break_reminder_min = tuple_int(iterator, KEY_BREAK_REMINDER_MIN, s_break_reminder_min);
      s_idle_reminder_min = tuple_int(iterator, KEY_IDLE_REMINDER_MIN, s_idle_reminder_min);
#endif
      // "Notify before a task is due" - minutes ahead, 0 = off.
      s_due_reminder_min = tuple_int(iterator, KEY_DUE_REMINDER_MIN, s_due_reminder_min);
      // Focus-mode session length in minutes. Only the NEXT session picks up a
      // change; a running one keeps its already-computed end time.
      s_focus_len_min = tuple_int(iterator, KEY_FOCUS_LEN_MIN, s_focus_len_min);
      s_use_pomodoro_cfg = tuple_int(iterator, KEY_USE_POMODORO_CFG, s_use_pomodoro_cfg) != 0;
      // "Stop tracking at midnight" - absent-means-unchanged.
      s_stop_at_midnight = tuple_int(iterator, KEY_STOP_AT_MIDNIGHT, s_stop_at_midnight) != 0;
      // "Nudge me about unfinished streaks" - absent-means-unchanged.
      s_habit_streak_nudge = tuple_int(iterator, KEY_HABIT_STREAK_NUDGE, s_habit_streak_nudge) != 0;
      // "Log energy on Finish Day" - absent-means-unchanged.
      s_reflect_enabled = tuple_int(iterator, KEY_REFLECT_ENABLED, s_reflect_enabled) != 0;
#endif
      // reload_data refreshes the Resync row's status subtitle;
      // update_empty_layer() handles the empty screen. Both no-op while the
      // error overlay is up.
      menu_layer_reload_data(s_menu_layer);
      update_empty_layer();
      if (s_status_code == STATUS_ERROR) {
        show_error_overlay();
      }
      break;
    }
    case MSG_WIPE_CACHE: {
      // Pairing page "Wipe watch cache": forget the persisted lists, show the
      // now-empty list, and pull a fresh copy from the phone.
      clear_persisted_caches();
      if (s_menu_layer) {
        menu_layer_reload_data(s_menu_layer);
      }
      update_empty_layer();
      request_sync();
      break;
    }
#ifndef PBL_PLATFORM_APLITE
    case MSG_PRESENCE_UPDATE: {
      s_presence_state = tuple_int(iterator, KEY_PRESENCE_STATE, 0);
      if (s_presence_state == 0) {
        s_presence_task[0] = '\0';
        s_presence_device[0] = '\0';
        s_presence_can_stop = false;
        s_presence_stopping = false;
      } else {
        str_copy(s_presence_task, tuple_str(iterator, KEY_PRESENCE_TASK_TITLE, ""), sizeof(s_presence_task));
        str_copy(s_presence_device, tuple_str(iterator, KEY_PRESENCE_DEVICE, ""), sizeof(s_presence_device));
        s_presence_elapsed_base = time(NULL) - (time_t)tuple_int(iterator, KEY_PRESENCE_ELAPSED_S, 0);
        s_presence_can_stop = tuple_int(iterator, KEY_PRESENCE_CAN_STOP, 0) != 0;
        s_presence_spent_ms = tuple_int(iterator, KEY_PRESENCE_SPENT_MS, 0);
        s_presence_estimate_ms = tuple_int(iterator, KEY_PRESENCE_ESTIMATE_MS, 0);
        // The phone confirms a stop by clearing (state 0), never by another
        // still-live update - so any fresh state drops the "Stopping..." latch.
        s_presence_stopping = false;
      }
      // A remote session that just started or switched task begins its own
      // over-estimate crossing - re-arm the latch and drop any banner it left
      // up. The phone flags this (PRESENCE_NEW_SESSION, a new sessionId); a
      // same-session heartbeat clears it so the repeat interval isn't reset
      // every 60s. Skipped while tracking locally - that session owns the latch.
      if (s_tracking_task_id[0] == '\0' && tuple_int(iterator, KEY_PRESENCE_NEW_SESSION, 0) != 0) {
        s_overtime_notified = false;
        s_overtime_last_notify_epoch = 0;
        hide_overtime_banner();
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
  str_copy(s_status_msg, "Couldn't reach phone app", MAX_STATUS_MSG_LEN);
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

#ifdef PBL_PLATFORM_EMERY
static void habit_flash_timer_cb(void *data) {
  s_habit_flash_timer = NULL;
  s_habit_flash_tick++;
  if (s_habit_flash_tick * HABIT_FLASH_STEP_MS < HABIT_FLASH_MS) {
    s_habit_flash_timer = app_timer_register(HABIT_FLASH_STEP_MS, habit_flash_timer_cb, NULL);
  } else {
    s_habit_flash_id[0] = '\0';
  }
  if (s_habits_menu_layer) {
    layer_mark_dirty(menu_layer_get_layer(s_habits_menu_layer));
  }
}

// Pulse the given habit's row green - called when a bump just took it to goal.
static void begin_habit_flash(const char *habit_id) {
  if (s_habit_flash_timer) {
    app_timer_cancel(s_habit_flash_timer);
  }
  str_copy(s_habit_flash_id, habit_id, MAX_HABIT_ID_LEN);
  s_habit_flash_tick = 0;
  s_habit_flash_timer = app_timer_register(HABIT_FLASH_STEP_MS, habit_flash_timer_cb, NULL);
}

static void stop_habit_flash(void) {
  if (s_habit_flash_timer) {
    app_timer_cancel(s_habit_flash_timer);
    s_habit_flash_timer = NULL;
  }
  s_habit_flash_id[0] = '\0';
}
#endif

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
  str_copy(s_tracking_habit_id, habit->id, MAX_HABIT_ID_LEN);
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
#ifdef PBL_PLATFORM_EMERY
  // Just hit its goal - pulse the row green a couple of times over HABIT_FLASH_MS.
  if (s_habit_flash_id[0] != '\0' &&
      strncmp(s_habit_flash_id, habit->id, MAX_HABIT_ID_LEN) == 0 &&
      (s_habit_flash_tick / 2) % 2 == 0) {
    bg = GColorMintGreen;
  }
#endif
  // A done habit's title stays full-strength (unlike a done task's, which dims):
  // the "- Done" subtitle carries the signal, and a habit gets incremented past
  // goal / decremented below it on the same day, so dimming would flicker.
  // Text stays black even when selected - the cerulean is light enough.
  GColor fg = GColorBlack;
  fill_bg(ctx, bounds, bg);
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
  draw_text(ctx, habit->title, TITLE_FONT_KEY, title_box, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
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
  GRect left_sub = subtitle_box;
#ifndef PBL_PLATFORM_APLITE
  // Right-aligned streak label on the subtitle line. Current streak from 2 up
  // ("1 streak" reads oddly); at 7+ it's a milestone - bold white-on-black
  // rounded badge (black-on-white contrasts on the cerulean selected row too).
  // Milestone tiers 7 / 30 / 100 add 1 / 2 / 3 white pips inside the badge.
  // Once the current streak lapses below 2, show "best N" instead as the
  // record to chase, when there is one.
  char st[16];
  bool milestone = false;
  if (habit->streak >= 2) {
    snprintf(st, sizeof(st), "%d streak", habit->streak);
    milestone = habit->streak >= 7;
  } else if (habit->best_streak >= 2) {
    snprintf(st, sizeof(st), "best %d", habit->best_streak);
  } else {
    st[0] = '\0';
  }
  if (st[0]) {
    GFont sf = fonts_get_system_font(milestone ? FONT_KEY_GOTHIC_14_BOLD : FONT_KEY_GOTHIC_14);
    GSize ss = graphics_text_layout_get_content_size(
        st, sf, subtitle_box, GTextOverflowModeTrailingEllipsis, GTextAlignmentRight);
    int16_t sw = ss.w;
    if (sw > subtitle_box.size.w / 2) {
      sw = subtitle_box.size.w / 2;
    }
    int tier = habit->streak >= 100 ? 3 : habit->streak >= 30 ? 2 : milestone ? 1 : 0;
    int16_t pips_w = tier ? tier * 4 + 1 : 0; // tier 3px pips, 1px apart, +2px lead
    int16_t pad = milestone ? 3 : 0;
    int16_t badge_w = sw + pad * 2 + pips_w;
    int16_t badge_x = subtitle_box.origin.x + subtitle_box.size.w - badge_w;
    GRect text_box = GRect(badge_x + pad + pips_w, subtitle_box.origin.y + 2,
                           sw, subtitle_box.size.h - 2);
    if (milestone) {
      graphics_context_set_fill_color(ctx, GColorBlack);
      graphics_fill_rect(ctx, GRect(badge_x, subtitle_box.origin.y, badge_w, subtitle_box.size.h),
                         3, GCornersAll);
      graphics_context_set_fill_color(ctx, GColorWhite);
      for (int p = 0; p < tier; p++) {
        graphics_fill_rect(ctx, GRect(badge_x + 3 + p * 4,
                                      subtitle_box.origin.y + subtitle_box.size.h / 2 - 1, 3, 3),
                           0, GCornerNone);
      }
      graphics_context_set_text_color(ctx, GColorWhite);
    }
    graphics_draw_text(ctx, st, sf, text_box,
        GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
    if (milestone) {
      graphics_context_set_text_color(ctx, fg);
    }
    left_sub.size.w -= (badge_w + 4);
  }
#endif
  draw_text(ctx, subtitle, SUBTITLE_FONT_KEY, left_sub, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
}

// Select +1, long-select -1 (never below 0). The phone applies the delta to its
// cached value and uploads a plain replace, so this local bump is a guess
// corrected by the next full sync.
static void adjust_habit(MenuIndex index, int32_t delta) {
  Habit *habit = resolve_habit_at(index);
  if (!habit || habit->value + delta < 0) {
    return; // already at 0, trying to go lower - silent no-op
  }
  bool was_done = habit->done;
  habit->value += delta;
  habit->done = habit->value >= habit->goal;
#ifdef PBL_PLATFORM_EMERY
  if (habit->done && !was_done) {
    begin_habit_flash(habit->id);
  }
#else
  (void)was_done;
#endif
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

#ifndef PBL_PLATFORM_APLITE
// Long-Up on a plain ClickCounter habit opens the value picker (Select +1 and
// long-Select -1 are unchanged; a timer habit has no set-value action). Wraps
// the MenuLayer's own provider the same way the main / browse windows do.
static ClickConfigProvider s_habits_menu_ccp = NULL;

static void habits_value_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (!s_habits_menu_layer) {
    return;
  }
  Habit *h = resolve_habit_at(menu_layer_get_selected_index(s_habits_menu_layer));
  if (!h || h->is_stopwatch || h->is_countdown) {
    return;
  }
  backlight_touch();
  push_value_picker(PICK_HABIT, h->id, h->value);
}

static void habits_menu_click_config_provider(void *context) {
  if (s_habits_menu_ccp) {
    s_habits_menu_ccp(context);
  }
  window_long_click_subscribe(BUTTON_ID_UP, 0, habits_value_long_click_handler, NULL);
}
#endif

static void habits_window_load(Window *window) {
  Layer *window_layer;
  GRect content_bounds = window_chrome(window, &s_habits_status_bar, &window_layer);

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
#ifndef PBL_PLATFORM_APLITE
  s_habits_menu_ccp = window_get_click_config_provider(window);
  window_set_click_config_provider_with_context(window, habits_menu_click_config_provider,
                                                window_get_click_config_context(window));
#endif
  layer_add_child(window_layer, menu_layer_get_layer(s_habits_menu_layer));

  s_habits_empty_layer = make_text_layer(window_layer, content_bounds,
                                         EMPTY_MSG_FONT_KEY, GTextAlignmentCenter);
  text_layer_set_text(s_habits_empty_layer, "No habits synced.");

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
#ifdef PBL_PLATFORM_EMERY
  stop_habit_flash();
#endif
  menu_layer_destroy(s_habits_menu_layer);
#ifndef PBL_PLATFORM_APLITE
  s_habits_menu_ccp = NULL;
#endif
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
// Persist the project list (chunked, like save_tasks - MAX_BROWSE_PROJECTS *
// sizeof(BrowseProject) overflows a single 256 B persist key) so it renders
// immediately on the next open and stays viewable with the phone away.
static void save_browse_projects(void) {
  if (!s_browse_projects) {
    return;
  }
  persist_delete(PERSIST_KEY_BROWSE_PROJECTS); // legacy single-blob key (pre-chunk builds)
  save_blob_cache(PERSIST_KEY_BROWSE_PROJECTS + 1, 280, (uint8_t *)s_browse_projects,
                  s_browse_project_count, sizeof(BrowseProject),
                  (size_t)MAX_BROWSE_PROJECTS * sizeof(BrowseProject));
}

static void load_browse_projects(void) {
  if (!s_browse_projects) {
    return;
  }
  s_browse_project_count = load_blob_cache(PERSIST_KEY_BROWSE_PROJECTS + 1, 280,
                                           (uint8_t *)s_browse_projects,
                                           MAX_BROWSE_PROJECTS, sizeof(BrowseProject));
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
    // project group header (menu_draw_header). The selected row darkens with
    // white text so it stands out (a bare text-colour flip on the bright fill
    // barely read). Any tag-flavoured mode uses the mint green of its nav row.
#ifndef PBL_PLATFORM_APLITE
    if (browse_wants_tags()) {
      fill_bg(ctx, bounds, is_selected ? GColorJaegerGreen : GColorMintGreen);
    } else
#endif
    {
      fill_bg(ctx, bounds, is_selected ? GColorIslamicGreen : GColorGreen);
    }
    int16_t title_y = HEADING_TITLE_Y(bounds.size.h);
    graphics_context_set_text_color(ctx, is_selected ? GColorWhite : GColorBlack);
#ifndef PBL_PLATFORM_APLITE
    if (s_browse_mode == BROWSE_TAG_EDIT) {
      // A checkbox (p->task_count 1 = tag assigned to s_browse_edit_task_id),
      // then the tag name. Select toggles it.
      GColor mk = is_selected ? GColorWhite : GColorBlack;
      GRect box = GRect(TITLE_BOX_X, (bounds.size.h - 16) / 2, 16, 16);
      graphics_context_set_stroke_color(ctx, mk);
      graphics_draw_rect(ctx, box);
      if (p->task_count) {
        graphics_context_set_fill_color(ctx, mk);
        graphics_fill_rect(ctx, GRect(box.origin.x + 4, box.origin.y + 4, 8, 8), 0, GCornerNone);
      }
      int16_t tx = TITLE_BOX_X + 22;
      draw_text(ctx, p->title, FONT_KEY_GOTHIC_24_BOLD,
                GRect(tx, title_y, bounds.size.w - tx - TITLE_BOX_X, HEADING_TITLE_H),
                GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
      return;
    }
#endif
    // The Inbox glyph for the default project, else the project's theme colour
    // as a swatch (draw_project_marker, shared with the today view). The phone
    // already packed the colour to a GColor8.
    int16_t text_x = draw_project_marker(ctx, TITLE_BOX_X, bounds.size.h, p->id,
                                          (uint8_t)p->color, is_selected);
    // Right-aligned count of the project's regular-list tasks (backlog and
    // done excluded, computed phone-side). Fixed-width strip so the title
    // ellipsis lands before it; 3 digits of GOTHIC_24_BOLD fit in 40 px.
    char count_buf[8];
    snprintf(count_buf, sizeof(count_buf), "%d", p->task_count);
    const int16_t count_w = 40;
    draw_text(ctx, count_buf, FONT_KEY_GOTHIC_24_BOLD,
              GRect(bounds.size.w - TITLE_BOX_X - count_w, title_y, count_w, HEADING_TITLE_H),
              GTextOverflowModeTrailingEllipsis, GTextAlignmentRight);
    draw_text(ctx, p->title, FONT_KEY_GOTHIC_24_BOLD,
              GRect(text_x, title_y, bounds.size.w - text_x - TITLE_BOX_X - count_w, HEADING_TITLE_H),
              GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
    return;
  }
  Task *bt = resolve_browse_task_at(*cell_index);
  if (!bt) {
    return;
  }
  // draw_task_row reads the same fields the today list draws; a browsed task
  // isn't in s_tasks but the struct is identical, so this reuses it wholesale
  // (including the live-ticking "spent / estimate" when it's the tracked one).
  // Tags mode passes show_project so each row names its project (a tag's tasks
  // span projects).
  bool show_project = false;
#ifndef PBL_PLATFORM_APLITE
  show_project = (s_browse_mode == BROWSE_TAGS); // TAG_EDIT is level-0 only
#endif
  draw_task_row(ctx, bounds, bt, is_selected, show_project);
}

// Switch to level 1 for `project_id` and fetch its tasks. Shared by a
// project-row Select in the browser, by push_browse_window's jump-straight-to
// path, and by the today view's project row (via push_browse_window).
static void browse_descend(const char *project_id) {
  str_copy(s_browse_project_id, project_id, MAX_PROJECT_ID_LEN);
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
// done - same send the today list uses, mirrored onto the today list too. A
// Select within a pending-reschedule window just cancels it (no toggle).
static void browse_menu_select_click(MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  backlight_touch();
  if (s_pending_reschedule_kind != RESCHEDULE_NONE) {
    cancel_pending_reschedule();
    return;
  }
  if (s_browse_level == 0) {
    BrowseProject *p = resolve_browse_project_at(*cell_index);
    if (!p) {
      return;
    }
#ifndef PBL_PLATFORM_APLITE
    if (s_browse_mode == BROWSE_TAG_EDIT) {
      p->task_count = p->task_count ? 0 : 1; // flip the checkbox
      menu_layer_reload_data(s_browse_menu);
      send_toggle_tag(s_browse_edit_task_id, p->id, p->task_count);
      return;
    }
    if (s_browse_mode == BROWSE_MOVE) {
      send_move_to_project(s_browse_edit_task_id, p->id);
      window_stack_pop(true); // back to whatever opened the picker
      return;
    }
#endif
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
// tags; a tag row is a no-op). Level 1: long-Select opens the per-task action
// menu, same as the today list - with "Move to backlog/list" as an extra row
// for a project task.
static void browse_menu_select_long_click(MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  backlight_touch();
  if (s_browse_level == 0) {
#ifndef PBL_PLATFORM_APLITE
    if (browse_wants_tags() || s_browse_mode == BROWSE_MOVE) {
      return; // a tag has no notes; the MOVE picker has no long-press
    }
#endif
    BrowseProject *p = resolve_browse_project_at(*cell_index);
    if (!p) {
      return;
    }
    s_notes_tags_line[0] = '\0';
    show_notes_overlay_for(p->id, true);
    return;
  }
#ifndef PBL_PLATFORM_APLITE
  Task *bt = resolve_browse_task_at(*cell_index);
  if (!bt) {
    return;
  }
  bool no_project = (s_browse_project_id[0] == '\0' ||
                     strncmp(s_browse_project_id, NO_PROJECT_ID_STR, MAX_PROJECT_ID_LEN) == 0);
  // ctx only decides whether the "Move to backlog" row shows - tags and the
  // "No Project" bucket have no backlog. Scheduling itself routes through
  // begin_pending_reschedule's own window/mode check, not ctx.
  ActionCtx ctx = (browse_wants_tags() || no_project) ? ACTX_TAG : ACTX_PROJECT;
  push_action_menu(bt->id, ctx, pt_section_is_backlog((int)cell_index->section));
#endif
}

static void browse_update_empty(void) {
  bool empty;
  const char *msg;
  bool tags = false;
#ifndef PBL_PLATFORM_APLITE
  tags = browse_wants_tags();
#endif
  if (s_browse_level == 0) {
    empty = s_browse_project_count == 0;
    msg = s_browse_projects_loading ? (tags ? "Loading tags..." : "Loading projects...")
                                    : (tags ? "No tags." : "No projects.");
  } else {
    empty = s_browse_task_count == 0;
    msg = s_browse_tasks_loading ? "Loading..."
                                 : (tags ? "No tasks for this tag." : "No tasks in this project.");
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

// Wraps MenuLayer's provider only to add BACK (level 1 -> level 0). Scheduling
// and backlog moves are in the per-task action menu now (long-Select a task).
static void browse_menu_click_config_provider(void *context) {
  if (s_browse_menu_ccp) {
    s_browse_menu_ccp(context);
  }
  window_single_click_subscribe(BUTTON_ID_BACK, browse_back_click_handler);
}

static void browse_window_load(Window *window) {
  Layer *window_layer;
  GRect content = window_chrome(window, &s_browse_status_bar, &window_layer);

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

  s_browse_empty = make_text_layer(window_layer, content, EMPTY_MSG_FONT_KEY, GTextAlignmentCenter);

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
  if (!browse_wants_tags()) {
    // PROJECTS and the MOVE picker both show the project list - render the cache
    // instantly, the fetch below refreshes it. Tags aren't cached.
    load_browse_projects();
  }
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
  // A task note with "- [ ]" lines: Select opens the interactive checklist.
  // Otherwise it dismisses (Back always dismisses).
  if (try_open_checklist()) {
    return;
  }
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
// Estimate / deadline / tags live in the task action menu (long-Select a task
// row), not on hidden gestures here.
static void notes_window_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, notes_window_select_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 0, notes_window_select_long_click_handler, NULL);
}

// Fills the layer with NOTES_TAGS_BG_COLOR and draws the bold "Tags:" label
// then the tag names below it. Two graphics_draw_text calls since one can't mix
// font weights. A no-op when the frame is zero-height (a project subject).
static void notes_tags_layer_draw(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  fill_bg(ctx, bounds, NOTES_TAGS_BG_COLOR);
  graphics_context_set_text_color(ctx, GColorBlack);

  int16_t content_w = bounds.size.w - NOTES_TAGS_PADDING_X * 2;
  int16_t label_h, names_h;
  measure_notes_tags_parts(content_w, &label_h, &names_h);

  GRect label_rect = GRect(NOTES_TAGS_PADDING_X, NOTES_TAGS_PADDING_Y, content_w, label_h);
  draw_text(ctx, NOTES_TAGS_LABEL, NOTES_LABEL_FONT_KEY, label_rect, GTextOverflowModeWordWrap, GTextAlignmentLeft);

  GRect names_rect = GRect(NOTES_TAGS_PADDING_X, NOTES_TAGS_PADDING_Y + label_h, content_w, names_h);
  draw_text(ctx, notes_tags_display_line(), NOTES_BODY_FONT_KEY, names_rect, GTextOverflowModeWordWrap, GTextAlignmentLeft);
}

static void notes_window_load(Window *window) {
  Layer *window_layer;
  GRect content_bounds = window_chrome(window, &s_notes_status_bar, &window_layer);
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

// ---------- notes checklist ----------
// The "- [ ]" / "- [x]" lines in a task's notes markdown, shown as a tickable
// list. Reached with Select from the notes window (which sits below this in the
// stack, keeping s_notes_full_text alive) when the fetched notes have any.
// A toggle flips the row locally and sends MSG_TASK_TOGGLE_CHECK (index = the
// row's position among checklist lines); the phone rewrites that one line and
// pushes a plain updateTask. On close, a re-fetch refreshes the notes text.
#define MAX_CHECKLIST 32
#define CHECKLIST_LABELS_CAP 1024
typedef struct { int label_off; int label_len; bool checked; } ChecklistItem;
static ChecklistItem s_checklist[MAX_CHECKLIST];
static int s_checklist_count = 0;
// malloc'd only while the checklist window is open - basalt heap has no room to
// spare it permanently.
static char *s_checklist_labels = NULL;
static int s_checklist_labels_len = 0;
static bool s_checklist_dirty = false;
static Window *s_checklist_window = NULL;
static MenuLayer *s_checklist_menu = NULL;
static StatusBarLayer *s_checklist_status_bar = NULL;

// Rebuild s_checklist from s_notes_full_text. A checklist line is optional
// indent, "-" or "*", " [", one of " xX", "]", then the label. Labels are
// snapshotted into s_checklist_labels so a later notes re-fetch can't dangle
// them.
static void parse_checklist(void) {
  s_checklist_count = 0;
  s_checklist_labels_len = 0;
  const char *buf = s_notes_full_text;
  if (!buf || !s_checklist_labels) {
    return;
  }
  const char *p = buf;
  while (*p && s_checklist_count < MAX_CHECKLIST) {
    const char *q = p;
    while (*q == ' ' || *q == '\t') {
      q++;
    }
    if ((q[0] == '-' || q[0] == '*') && q[1] == ' ' && q[2] == '[' && q[3] &&
        (q[3] == ' ' || q[3] == 'x' || q[3] == 'X') && q[4] == ']') {
      const char *lbl = q + 5;
      if (*lbl == ' ') {
        lbl++;
      }
      const char *le = lbl;
      while (*le && *le != '\n') {
        le++;
      }
      int len = (int)(le - lbl);
      if (len > CHECKLIST_LABELS_CAP - 1 - s_checklist_labels_len) {
        len = CHECKLIST_LABELS_CAP - 1 - s_checklist_labels_len;
      }
      if (len < 0) {
        len = 0;
      }
      ChecklistItem *it = &s_checklist[s_checklist_count++];
      it->label_off = s_checklist_labels_len;
      it->label_len = len;
      it->checked = (q[3] == 'x' || q[3] == 'X');
      memcpy(s_checklist_labels + s_checklist_labels_len, lbl, (size_t)len);
      s_checklist_labels_len += len;
      s_checklist_labels[s_checklist_labels_len++] = '\0';
    }
    while (*p && *p != '\n') {
      p++;
    }
    if (*p == '\n') {
      p++;
    }
  }
}

static uint16_t checklist_num_rows(MenuLayer *ml, uint16_t section, void *ctx) {
  return (uint16_t)s_checklist_count;
}

static void checklist_draw_row(GContext *ctx, const Layer *cell, MenuIndex *idx, void *c) {
  if ((int)idx->row >= s_checklist_count || !s_checklist_labels) {
    return;
  }
  ChecklistItem *it = &s_checklist[idx->row];
  GRect b = layer_get_bounds(cell);
  bool sel = menu_layer_get_selected_index(s_checklist_menu).row == idx->row;
  GColor fg = sel ? GColorWhite : GColorBlack;
  GRect box = GRect(b.origin.x + 6, b.origin.y + (b.size.h - 16) / 2, 16, 16);
  graphics_context_set_stroke_color(ctx, fg);
  graphics_draw_rect(ctx, box);
  if (it->checked) {
    graphics_context_set_fill_color(ctx, fg);
    graphics_fill_rect(ctx, GRect(box.origin.x + 4, box.origin.y + 4, 8, 8), 0, GCornerNone);
  }
  graphics_context_set_text_color(ctx, fg);
  GRect tb = GRect(box.origin.x + 26, b.origin.y, b.size.w - (box.origin.x + 26) - 4, b.size.h);
  draw_text(ctx, s_checklist_labels + it->label_off, FONT_KEY_GOTHIC_18,
            tb, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
}

static void checklist_select(MenuLayer *ml, MenuIndex *idx, void *c) {
  if ((int)idx->row >= s_checklist_count) {
    return;
  }
  backlight_touch();
  ChecklistItem *it = &s_checklist[idx->row];
  it->checked = !it->checked;
  s_checklist_dirty = true;
  menu_layer_reload_data(s_checklist_menu);
  // s_notes_overlay_subject_id is the task id - try_open_checklist bars the
  // project-notes case.
  begin_send(MSG_TASK_TOGGLE_CHECK, s_notes_overlay_subject_id, NULL,
             ((int32_t)idx->row << 1) | (it->checked ? 1 : 0));
}

static void checklist_window_load(Window *window) {
  Layer *wl;
  GRect cb = window_chrome(window, &s_checklist_status_bar, &wl);
  s_checklist_menu = menu_layer_create(cb);
  menu_layer_set_callbacks(s_checklist_menu, NULL, (MenuLayerCallbacks) {
    .get_num_rows = checklist_num_rows,
    .draw_row = checklist_draw_row,
    .select_click = checklist_select,
  });
  menu_layer_set_normal_colors(s_checklist_menu, GColorWhite, GColorBlack);
  menu_layer_set_highlight_colors(s_checklist_menu, GColorVividCerulean, GColorWhite);
  menu_layer_set_click_config_onto_window(s_checklist_menu, window);
  layer_add_child(wl, menu_layer_get_layer(s_checklist_menu));
}

static void checklist_window_unload(Window *window) {
  menu_layer_destroy(s_checklist_menu);
  s_checklist_menu = NULL;
  status_bar_layer_destroy(s_checklist_status_bar);
  free(s_checklist_labels);
  s_checklist_labels = NULL;
  s_checklist_count = 0;
  // Re-pull the notes so the text under us reflects the toggles (the phone
  // rewrote the markdown). Only if something changed.
  if (s_checklist_dirty && s_notes_overlay_active && !s_notes_overlay_is_project) {
    s_checklist_dirty = false;
    reset_notes_full_buffer();
    s_notes_display_text = NOTES_LOADING_TEXT;
    s_notes_is_loading = true;
    request_notes_full(s_notes_overlay_subject_id, false);
    render_notes_overlay_content();
  }
}

static bool try_open_checklist(void) {
  if (s_notes_overlay_is_project || s_notes_is_loading || s_checklist_labels) {
    return false;
  }
  s_checklist_labels = malloc(CHECKLIST_LABELS_CAP);
  if (!s_checklist_labels) {
    return false;
  }
  parse_checklist();
  if (s_checklist_count == 0) {
    free(s_checklist_labels);
    s_checklist_labels = NULL;
    return false;
  }
  s_checklist_dirty = false;
  if (!s_checklist_window) {
    s_checklist_window = window_create();
    window_set_window_handlers(s_checklist_window, (WindowHandlers) {
      .load = checklist_window_load,
      .unload = checklist_window_unload,
    });
  }
  window_stack_push(s_checklist_window, true);
  return true;
}

// ---------- value picker (estimate / deadline) ----------
// One tiny full-screen picker window, reached from a task's notes overlay:
// long-Up sets the time estimate, long-Down sets the deadline. Up/Down step a
// fixed ladder, Select sends it (a plain updateTask, applied optimistically to
// the row), Back cancels. PickKind is declared up with the forward decls.
// Estimate ladder is minutes (0 = clear). Deadline ladder is days from today,
// with -1 as the "None" (clear) rung.
static const int s_est_ladder_min[] = { 0, 15, 30, 45, 60, 90, 120, 180, 240, 300, 360, 480 };
static const int s_dl_ladder_days[] = { -1, 0, 1, 2, 3, 7, 14, 30 };
// Habit counter value - a spread that covers small daily counters and the
// odd big one. Sent as a delta off the current value (MSG_HABIT_ADJUST).
static const int s_habit_ladder[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 15, 20, 25, 30, 40, 50, 75, 100 };
// Hour of the day for "Schedule at..." - the phone picks today or tomorrow.
static const int s_time_ladder_hr[] = { 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22 };
#define ARRLEN(a) (int)(sizeof(a) / sizeof((a)[0]))
static Window *s_pick_window = NULL;
static Layer *s_pick_layer = NULL;
static StatusBarLayer *s_pick_status_bar = NULL;
static char s_pick_task_id[MAX_ID_LEN] = "";
static PickKind s_pick_kind = PICK_ESTIMATE;
static int s_pick_idx = 0;

static int pick_ladder_len(void) {
  return s_pick_kind == PICK_ESTIMATE ? ARRLEN(s_est_ladder_min)
       : s_pick_kind == PICK_DEADLINE ? ARRLEN(s_dl_ladder_days)
       : s_pick_kind == PICK_TIME ? ARRLEN(s_time_ladder_hr)
       : ARRLEN(s_habit_ladder);
}
static int pick_ladder_val(int i) {
  return s_pick_kind == PICK_ESTIMATE ? s_est_ladder_min[i]
       : s_pick_kind == PICK_DEADLINE ? s_dl_ladder_days[i]
       : s_pick_kind == PICK_TIME ? s_time_ladder_hr[i]
       : s_habit_ladder[i];
}

// The value string for the current rung.
static void pick_format(char *out, size_t len) {
  int v = pick_ladder_val(s_pick_idx);
  if (s_pick_kind == PICK_HABIT) {
    snprintf(out, len, "%d", v);
    return;
  }
  if (s_pick_kind == PICK_TIME) {
    if (clock_is_24h_style()) {
      snprintf(out, len, "%d:00", v);
    } else {
      int h12 = v % 12;
      if (h12 == 0) {
        h12 = 12;
      }
      snprintf(out, len, "%d:00 %s", h12, v < 12 ? "AM" : "PM");
    }
    return;
  }
  if (s_pick_kind == PICK_ESTIMATE) {
    if (v <= 0) {
      str_copy(out, "None", len);
      return;
    }
    int h = v / 60, m = v % 60;
    if (h && m) {
      snprintf(out, len, "%dh %dm", h, m);
    } else if (h) {
      snprintf(out, len, "%dh", h);
    } else {
      snprintf(out, len, "%dm", m);
    }
    return;
  }
  if (v < 0) {
    str_copy(out, "None", len);
  } else if (v == 0) {
    str_copy(out, "Today", len);
  } else if (v == 1) {
    str_copy(out, "Tomorrow", len);
  } else if (v == 7) {
    str_copy(out, "1 week", len);
  } else if (v == 14) {
    str_copy(out, "2 weeks", len);
  } else if (v == 30) {
    str_copy(out, "1 month", len);
  } else {
    snprintf(out, len, "%d days", v);
  }
}

// Ladder index whose value is closest to `target` (minutes for estimate, days
// for deadline; DEADLINE_NONE-equivalent handled by the caller).
static int pick_nearest_idx(int target) {
  int best = 0, best_d = 1 << 30;
  for (int i = 0; i < pick_ladder_len(); i++) {
    int d = pick_ladder_val(i) - target;
    if (d < 0) {
      d = -d;
    }
    if (d < best_d) {
      best_d = d;
      best = i;
    }
  }
  return best;
}

static void pick_layer_update_proc(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  fill_bg(ctx, b, GColorWhite);
  graphics_context_set_text_color(ctx, GColorBlack);
  int16_t cy = b.size.h / 2;
  draw_text(ctx, s_pick_kind == PICK_ESTIMATE ? "Estimate"
                 : s_pick_kind == PICK_DEADLINE ? "Deadline"
                 : s_pick_kind == PICK_TIME ? "Schedule at" : "Count",
            CHROME_FONT_KEY,
            GRect(0, cy - 44, b.size.w, 20), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter);
  char val[16];
  pick_format(val, sizeof(val));
  draw_text(ctx, val, FONT_KEY_GOTHIC_28_BOLD,
            GRect(0, cy - 22, b.size.w, 34), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter);
  draw_text(ctx, "Up/Down pick\nSelect to set", CHROME_FONT_KEY,
            GRect(0, cy + 16, b.size.w, 40), GTextOverflowModeWordWrap, GTextAlignmentCenter);
}

static void pick_step(int delta) {
  int n = s_pick_idx + delta;
  if (n < 0) {
    n = 0;
  }
  if (n >= pick_ladder_len()) {
    n = pick_ladder_len() - 1;
  }
  if (n != s_pick_idx) {
    s_pick_idx = n;
    layer_mark_dirty(s_pick_layer);
    vibes_short_pulse();
  }
}

static void pick_up_click(ClickRecognizerRef r, void *c) { backlight_touch(); pick_step(1); }
static void pick_down_click(ClickRecognizerRef r, void *c) { backlight_touch(); pick_step(-1); }

static void pick_select_click(ClickRecognizerRef r, void *c) {
  backlight_touch();
  int v = pick_ladder_val(s_pick_idx);
  if (s_pick_kind == PICK_HABIT) {
    Habit *h = find_habit_by_id(s_pick_task_id);
    if (h) {
      int32_t delta = v - h->value;
      if (delta != 0) {
        bool h_was_done = h->done;
        h->value = v;
        h->done = h->value >= h->goal;
        save_habits();
        send_habit_adjust(h, delta); // phone replaces countOnDay[today] with the sum
#ifdef PBL_PLATFORM_EMERY
        if (h->done && !h_was_done) {
          begin_habit_flash(h->id);
        }
#else
        (void)h_was_done;
#endif
        if (s_habits_menu_layer) {
          menu_layer_reload_data(s_habits_menu_layer);
        }
      }
    }
    window_stack_pop(true);
    return;
  }
  Task *t = find_task_by_id(s_pick_task_id);
  if (s_pick_kind == PICK_TIME) {
#ifdef PBL_PLATFORM_EMERY
    // Hand off to the shared cancel window - "Scheduling 3 PM..." + bar on the
    // task row below, commit (send_task_set_due_time) after RESCHEDULE_WINDOW_MS.
    (void)t;
    s_pending_reschedule_at_hour = v;
    window_stack_pop(true);
    begin_pending_reschedule(RESCHEDULE_AT);
    return;
#else
    if (t) {
      t->due_min = v * 60; // the phone decides today vs tomorrow
    }
    send_task_set_due_time(s_pick_task_id, v);
#endif
  } else if (s_pick_kind == PICK_ESTIMATE) {
    int32_t ms = (int32_t)v * 60000;
    if (t) {
      t->time_estimate_ms = (int)ms;
    }
    send_task_set_estimate(s_pick_task_id, ms);
  } else {
    if (t) {
      t->deadline_days = (v < 0) ? DEADLINE_NONE : v;
    }
    send_task_set_deadline(s_pick_task_id, v);
  }
  if (t) {
    save_tasks();
    if (s_menu_layer) {
      menu_layer_reload_data(s_menu_layer);
    }
  }
  window_stack_pop(true);
}

static void pick_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, pick_up_click);
  window_single_click_subscribe(BUTTON_ID_DOWN, pick_down_click);
  window_single_click_subscribe(BUTTON_ID_SELECT, pick_select_click);
}

static void pick_window_load(Window *window) {
  Layer *window_layer;
  GRect content = window_chrome(window, &s_pick_status_bar, &window_layer);
  s_pick_layer = layer_create(content);
  layer_set_update_proc(s_pick_layer, pick_layer_update_proc);
  layer_add_child(window_layer, s_pick_layer);
  window_set_click_config_provider(window, pick_click_config_provider);
}

static void pick_window_unload(Window *window) {
  layer_destroy(s_pick_layer);
  s_pick_layer = NULL;
  status_bar_layer_destroy(s_pick_status_bar);
  s_pick_status_bar = NULL;
}

// current: ms for PICK_ESTIMATE; days-from-today for PICK_DEADLINE, or
// DEADLINE_NONE for no deadline (opens on the "None" rung).
static void push_value_picker(PickKind kind, const char *task_id, int current) {
  if (!task_id || task_id[0] == '\0') {
    return;
  }
  str_copy(s_pick_task_id, task_id, sizeof(s_pick_task_id));
  s_pick_kind = kind;
  if (kind == PICK_ESTIMATE) {
    s_pick_idx = pick_nearest_idx(current / 60000);
  } else if (kind == PICK_DEADLINE) {
    s_pick_idx = (current == DEADLINE_NONE) ? 0 : pick_nearest_idx(current);
  } else {
    s_pick_idx = pick_nearest_idx(current); // habit value, or hour 0-23 for PICK_TIME
  }
  if (!s_pick_window) {
    s_pick_window = window_create();
    window_set_window_handlers(s_pick_window, (WindowHandlers) {
      .load = pick_window_load,
      .unload = pick_window_unload,
    });
  }
  window_stack_push(s_pick_window, true);
}

// ---------- reflect (before-Finish-Day retro) ----------
// A 3-row menu reached with Select on the Finish Day row (config.enableReflect;
// long-Select there still archives). Each row logs one field of today's metric
// straight away (optimistic, phone does [Metric] Upsert Metric):
//   Energy      - Select cycles Low/OK/Good        -> MSG_METRIC_ENERGY (1-3)
//   Day rating  - Select cycles 1..4 (impactOfWork) -> MSG_METRIC_RATING (1-4)
//   Improvement - Select dictates one thing to improve -> MSG_METRIC_REFLECT
// Values start unset ("-") - the watch never fetches the existing metric.
// State statics are declared up near s_reflect_enabled.

#define REFLECT_HAS_MIC PBL_IF_MICROPHONE_ELSE(true, false)

static uint16_t reflect_num_rows(MenuLayer *ml, uint16_t section, void *ctx) {
  return REFLECT_HAS_MIC ? 3 : 2;
}

static void reflect_draw_row(GContext *ctx, const Layer *cell, MenuIndex *idx, void *c) {
  char sub[20];
  const char *title;
  if (idx->row == 0) {
    title = "Energy";
    str_copy(sub, s_reflect_energy == 0 ? "-"
                  : s_reflect_energy == 1 ? "Low"
                  : s_reflect_energy == 2 ? "OK" : "Good", sizeof(sub));
  } else if (idx->row == 1) {
    title = "Day rating";
    if (s_reflect_rating == 0) {
      str_copy(sub, "-", sizeof(sub));
    } else {
      snprintf(sub, sizeof(sub), "%d of 4", s_reflect_rating);
    }
  } else {
    title = "Improvement";
    str_copy(sub, s_reflect_note_set ? "Logged" : "Say one thing", sizeof(sub));
  }
  menu_cell_basic_draw(ctx, cell, title, sub, NULL);
}

static void reflect_select(MenuLayer *ml, MenuIndex *idx, void *c) {
  backlight_touch();
  if (idx->row == 0) {
    s_reflect_energy = s_reflect_energy >= 3 ? 1 : s_reflect_energy + 1;
    begin_send(MSG_METRIC_ENERGY, "", NULL, s_reflect_energy);
  } else if (idx->row == 1) {
    s_reflect_rating = s_reflect_rating >= 4 ? 1 : s_reflect_rating + 1;
    begin_send(MSG_METRIC_RATING, "", NULL, s_reflect_rating);
  } else {
#ifndef PBL_PLATFORM_APLITE
    start_reflect_dictation(); // reflect_note_set is flipped on the callback
#endif
    return;
  }
  menu_layer_reload_data(s_reflect_menu);
}

static void reflect_window_load(Window *window) {
  Layer *wl;
  GRect cb = window_chrome(window, &s_reflect_status_bar, &wl);
  s_reflect_menu = menu_layer_create(cb);
  menu_layer_set_callbacks(s_reflect_menu, NULL, (MenuLayerCallbacks) {
    .get_num_rows = reflect_num_rows,
    .draw_row = reflect_draw_row,
    .select_click = reflect_select,
  });
  menu_layer_set_normal_colors(s_reflect_menu, GColorWhite, GColorBlack);
  menu_layer_set_highlight_colors(s_reflect_menu, GColorVividCerulean, GColorWhite);
  menu_layer_set_click_config_onto_window(s_reflect_menu, window);
  layer_add_child(wl, menu_layer_get_layer(s_reflect_menu));
}

static void reflect_window_unload(Window *window) {
  menu_layer_destroy(s_reflect_menu);
  s_reflect_menu = NULL;
  status_bar_layer_destroy(s_reflect_status_bar);
  s_reflect_status_bar = NULL;
}

static void push_reflect_window(void) {
  s_reflect_energy = 0;
  s_reflect_rating = 0;
  s_reflect_note_set = false;
  if (!s_reflect_window) {
    s_reflect_window = window_create();
    window_set_window_handlers(s_reflect_window, (WindowHandlers) {
      .load = reflect_window_load,
      .unload = reflect_window_unload,
    });
  }
  window_stack_push(s_reflect_window, true);
}

// ---------- per-task action menu ----------
// Long-Select on a task row - today list OR a Projects/Tags browser task view -
// opens this. The one home for per-task actions. Each row pops this menu, then
// launches its target, so Back from the target returns to the list.
enum {
  ACT_TRACK, ACT_TODAY, ACT_TOMORROW, ACT_AT, ACT_UNSCHEDULE,
  ACT_NOTES, ACT_TAGS, ACT_MOVE, ACT_ESTIMATE, ACT_DEADLINE, ACT_BACKLOG,
  ACT_REPEAT, // appended by act_rows() only for a recurring task
};
// The visible rows, in order, per context. Tags / Move open the browse window
// as a picker, so they're only offered from the today list (from the browser
// that window is already on the stack). Backlog is a project-task concept.
static const int s_act_rows_today[] = {
  ACT_TRACK, ACT_TODAY, ACT_TOMORROW, ACT_AT, ACT_UNSCHEDULE,
  ACT_NOTES, ACT_TAGS, ACT_MOVE, ACT_ESTIMATE, ACT_DEADLINE };
static const int s_act_rows_project[] = {
  ACT_TRACK, ACT_TODAY, ACT_TOMORROW, ACT_AT, ACT_UNSCHEDULE,
  ACT_NOTES, ACT_ESTIMATE, ACT_DEADLINE, ACT_BACKLOG };
static const int s_act_rows_tag[] = {
  ACT_TRACK, ACT_TODAY, ACT_TOMORROW, ACT_AT, ACT_UNSCHEDULE,
  ACT_NOTES, ACT_ESTIMATE, ACT_DEADLINE };

// ActionCtx declared with the forward decls.
static Window *s_action_window = NULL;
static MenuLayer *s_action_menu = NULL;
static StatusBarLayer *s_action_status_bar = NULL;
static char s_action_task_id[MAX_ID_LEN] = "";
static ActionCtx s_action_ctx = ACTX_TODAY;
static bool s_action_in_backlog = false;
// "Repeat" row: pattern text (MSG_TASK_REPEAT_DATA) + pause state, per open.
static char s_action_repeat_text[24] = "";
static bool s_action_repeat_paused = false;
static int s_act_rows_buf[12];

static Task *resolve_action_task(void);

static const int *act_rows(int *count) {
  const int *base;
  int n;
  switch (s_action_ctx) {
    case ACTX_PROJECT: base = s_act_rows_project; n = ARRLEN(s_act_rows_project); break;
    case ACTX_TAG:     base = s_act_rows_tag;     n = ARRLEN(s_act_rows_tag);     break;
    default:           base = s_act_rows_today;   n = ARRLEN(s_act_rows_today);   break;
  }
  memcpy(s_act_rows_buf, base, (size_t)n * sizeof(int));
  Task *t = resolve_action_task();
  if (t && t->recurs && n < (int)ARRLEN(s_act_rows_buf)) {
    s_act_rows_buf[n++] = ACT_REPEAT;
  }
  *count = n;
  return s_act_rows_buf;
}

static bool action_task_is_tracked(void) {
  return s_tracking_task_id[0] != '\0' &&
         strncmp(s_tracking_task_id, s_action_task_id, MAX_ID_LEN) == 0;
}

// MSG_TASK_REPEAT_DATA: the "Repeat" row's pattern + pause state, for the task
// whose action menu is open. A stale reply for a different task is ignored.
static void handle_repeat_data(DictionaryIterator *it) {
  if (strncmp(tuple_str(it, KEY_TASK_ID, ""), s_action_task_id, MAX_ID_LEN) != 0) {
    return;
  }
  str_copy(s_action_repeat_text, tuple_str(it, KEY_TASK_REPEAT_TEXT, ""), sizeof(s_action_repeat_text));
  s_action_repeat_paused = tuple_int(it, KEY_TASK_REPEAT_PAUSED, 0) != 0;
  if (s_action_menu) {
    menu_layer_reload_data(s_action_menu);
  }
}

// The Task* for s_action_task_id: the today list, else a browsed task.
static Task *resolve_action_task(void) {
  Task *t = find_task_by_id(s_action_task_id);
#if PROJECTS_BROWSER
  if (!t && s_browse_tasks) {
    for (int i = 0; i < s_browse_task_count; i++) {
      if (strncmp(s_browse_tasks[i].id, s_action_task_id, MAX_ID_LEN) == 0) {
        return &s_browse_tasks[i];
      }
    }
  }
#endif
  return t;
}

static uint16_t action_get_num_rows(MenuLayer *ml, uint16_t section, void *ctx) {
  int n;
  act_rows(&n);
  return (uint16_t)n;
}

static void action_draw_row(GContext *ctx, const Layer *cell, MenuIndex *idx, void *c) {
  int n;
  const int *rows = act_rows(&n);
  if ((int)idx->row >= n) {
    return;
  }
  const char *label = "";
  const char *sub = NULL;
  switch (rows[idx->row]) {
    case ACT_TRACK:      label = action_task_is_tracked() ? "Stop tracking" : "Start tracking"; break;
    case ACT_TODAY:      label = "Schedule today"; break;
    case ACT_TOMORROW:   label = "Schedule tomorrow"; break;
    case ACT_AT:         label = "Schedule at..."; break;
    case ACT_UNSCHEDULE: label = "Unschedule"; break;
    case ACT_NOTES:      label = "Notes"; break;
    case ACT_TAGS:       label = "Edit tags"; break;
    case ACT_MOVE:       label = "Move to project"; break;
    case ACT_ESTIMATE:   label = "Set estimate"; break;
    case ACT_DEADLINE:   label = "Set deadline"; break;
    case ACT_BACKLOG:    label = s_action_in_backlog ? "Move to list" : "Move to backlog"; break;
    case ACT_REPEAT:
      label = s_action_repeat_paused ? "Resume repeat" : "Pause repeat";
      sub = s_action_repeat_text[0] ? s_action_repeat_text : "Loading...";
      break;
  }
  menu_cell_basic_draw(ctx, cell, label, sub, NULL);
}

static void action_select(MenuLayer *ml, MenuIndex *idx, void *c) {
  backlight_touch();
  int n;
  const int *rows = act_rows(&n);
  if ((int)idx->row >= n) {
    return;
  }
  int row = rows[idx->row];
  Task *t = resolve_action_task();
  if (row == ACT_REPEAT) {
    // Toggle pause in place - keep the menu open so the label flips.
    s_action_repeat_paused = !s_action_repeat_paused;
    begin_send(MSG_TASK_REPEAT_PAUSE, s_action_task_id, NULL, s_action_repeat_paused ? 1 : 0);
    menu_layer_reload_data(s_action_menu);
    return;
  }
  window_stack_pop(true); // close the menu; targets push onto the list below it
  switch (row) {
    case ACT_TRACK:
      if (!t) {
        break;
      }
      {
        bool was = action_task_is_tracked();
        stop_tracking_and_report();
        if (!was) {
          start_tracking(t);
        }
        menu_layer_reload_data(s_menu_layer);
      }
      break;
    case ACT_TODAY:
      begin_pending_reschedule(RESCHEDULE_TODAY);
      break;
    case ACT_TOMORROW:
      begin_pending_reschedule(RESCHEDULE_TOMORROW);
      break;
    case ACT_AT: {
      int hour = 9;
      if (t && t->due_min >= 0) {
        hour = t->due_min / 60;
      } else {
        time_t now = time(NULL);
        struct tm *lt = localtime(&now);
        hour = lt->tm_hour;
      }
      push_value_picker(PICK_TIME, s_action_task_id, hour);
      break;
    }
    case ACT_UNSCHEDULE:
      begin_pending_reschedule(RESCHEDULE_UNSCHEDULE);
      break;
    case ACT_BACKLOG:
      begin_pending_reschedule(s_action_in_backlog ? RESCHEDULE_FROM_BACKLOG : RESCHEDULE_TO_BACKLOG);
      break;
    case ACT_NOTES:
      if (t) {
        show_notes_overlay(t);
      }
      break;
    case ACT_TAGS:
#if PROJECTS_BROWSER
      s_browse_mode = BROWSE_TAG_EDIT;
      str_copy(s_browse_edit_task_id, s_action_task_id, sizeof(s_browse_edit_task_id));
      push_browse_window(NULL);
#endif
      break;
    case ACT_MOVE:
#if PROJECTS_BROWSER
      s_browse_mode = BROWSE_MOVE;
      str_copy(s_browse_edit_task_id, s_action_task_id, sizeof(s_browse_edit_task_id));
      push_browse_window(NULL);
#endif
      break;
    case ACT_ESTIMATE:
      push_value_picker(PICK_ESTIMATE, s_action_task_id, t ? t->time_estimate_ms : 0);
      break;
    case ACT_DEADLINE:
      push_value_picker(PICK_DEADLINE, s_action_task_id, t ? t->deadline_days : DEADLINE_NONE);
      break;
  }
}

static void action_window_load(Window *window) {
  Layer *wl;
  GRect content = window_chrome(window, &s_action_status_bar, &wl);
  s_action_menu = menu_layer_create(content);
  menu_layer_set_callbacks(s_action_menu, NULL, (MenuLayerCallbacks) {
    .get_num_rows = action_get_num_rows,
    .draw_row = action_draw_row,
    .select_click = action_select,
  });
  menu_layer_set_click_config_onto_window(s_action_menu, window);
  layer_add_child(wl, menu_layer_get_layer(s_action_menu));
}

static void action_window_unload(Window *window) {
  menu_layer_destroy(s_action_menu);
  s_action_menu = NULL;
  status_bar_layer_destroy(s_action_status_bar);
  s_action_status_bar = NULL;
}

static void push_action_menu(const char *task_id, ActionCtx ctx, bool in_backlog) {
  if (!task_id || task_id[0] == '\0') {
    return;
  }
  str_copy(s_action_task_id, task_id, sizeof(s_action_task_id));
  s_action_ctx = ctx;
  s_action_in_backlog = in_backlog;
  // "Repeat" row only exists for a recurring task; its pattern text is fetched.
  s_action_repeat_text[0] = '\0';
  s_action_repeat_paused = false;
  {
    Task *rt = resolve_action_task();
    if (rt && rt->recurs) {
      begin_send(MSG_TASK_REPEAT_REQUEST, s_action_task_id, NULL, 0);
    }
  }
  if (!s_action_window) {
    s_action_window = window_create();
    window_set_window_handlers(s_action_window, (WindowHandlers) {
      .load = action_window_load,
      .unload = action_window_unload,
    });
  }
  window_stack_push(s_action_window, true);
}
#endif

// ---------- stats page ----------
#ifndef PBL_PLATFORM_APLITE
// A pinned section-0 row ("Stats", between Projects and Add Task) opens this
// read-only scrollable summary: estimate remaining today, time worked today,
// time tracked without a break, the current tracking session, tasks completed
// today, then every project with its task count. The phone sends the two
// durations, the done count and a preformatted "Name - count" project block in
// one MSG_STATS_DATA. A custom layer inside a ScrollLayer draws each metric as a
// black label bar (white text) with its value below in black - so the label
// reads as a heading and the value as data. "Without a break" and "Current
// session" are recomputed on every draw from watch-local state (s_break_*,
// s_tracking_start_epoch), or - for the session - a remote device's live
// presence (s_presence_state / s_presence_elapsed_base).
#ifdef PBL_PLATFORM_EMERY
#define STATS_LABEL_FONT FONT_KEY_GOTHIC_24_BOLD
#define STATS_VALUE_FONT FONT_KEY_GOTHIC_28_BOLD
#define STATS_LINE_FONT  FONT_KEY_GOTHIC_24
#define STATS_LABEL_H 28
#define STATS_VALUE_H 34
#define STATS_LINE_H  30
#else
#define STATS_LABEL_FONT FONT_KEY_GOTHIC_18_BOLD
#define STATS_VALUE_FONT FONT_KEY_GOTHIC_24_BOLD
#define STATS_LINE_FONT  FONT_KEY_GOTHIC_18
#define STATS_LABEL_H 22
#define STATS_VALUE_H 28
#define STATS_LINE_H  24
#endif
#define STATS_GAP 6
#define STATS_PAD_X 6
static Window *s_stats_window;
static StatusBarLayer *s_stats_status_bar;
static ScrollLayer *s_stats_scroll_layer;
static Layer *s_stats_content_layer;
static GRect s_stats_content_bounds;
static char s_stats_est[24] = "";
static char s_stats_worked[24] = "";
static char s_stats_session[40] = "";
static char s_stats_nobreak[24] = "";

// Recompute the value strings (the durations from the last payload, the
// session from live tracking state).
static void stats_compute_values(void) {
  format_duration_ms(s_stats_est_remaining_ms < 0 ? 0 : s_stats_est_remaining_ms, false,
                     s_stats_est, sizeof(s_stats_est));
  format_duration_ms(s_stats_worked_today_ms < 0 ? 0 : s_stats_worked_today_ms, false,
                     s_stats_worked, sizeof(s_stats_worked));
  char dur[24];
  if (s_tracking_task_id[0] != '\0') {
    int e = (int)((time(NULL) - s_tracking_start_epoch) * 1000);
    format_duration_ms(e < 0 ? 0 : e, false, dur, sizeof(dur));
    str_copy(s_stats_session, dur, sizeof(s_stats_session));
  } else if (s_presence_state == 1) {
    int e = (int)((time(NULL) - s_presence_elapsed_base) * 1000);
    format_duration_ms(e < 0 ? 0 : e, false, dur, sizeof(dur));
    snprintf(s_stats_session, sizeof(s_stats_session), "%s (%s)", dur,
             s_presence_device[0] != '\0' ? s_presence_device : "remote");
  } else {
    str_copy(s_stats_session, "Not tracking", sizeof(s_stats_session));
  }

  // "Without a break" - the break reminder's tally: watch-tracked time banked
  // since the last pause of BREAK_RESET_GAP_S or more, plus the running session.
  int nb = s_break_accum_s;
  if (s_tracking_task_id[0] != '\0' && time(NULL) > s_tracking_start_epoch) {
    nb += (int)(time(NULL) - s_tracking_start_epoch);
  }
  format_duration_ms(nb * 1000, false, s_stats_nobreak, sizeof(s_stats_nobreak));
}

// Draws one label bar (black, white text) with its value below (black text).
static int16_t stats_draw_metric(GContext *ctx, int16_t y, int16_t w,
                                  const char *label, const char *value) {
  fill_bg(ctx, GRect(0, y, w, STATS_LABEL_H), GColorBlack);
  graphics_context_set_text_color(ctx, GColorWhite);
  draw_text(ctx, label, STATS_LABEL_FONT, GRect(STATS_PAD_X, y, w - STATS_PAD_X * 2, STATS_LABEL_H), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
  y += STATS_LABEL_H;
  graphics_context_set_text_color(ctx, GColorBlack);
  draw_text(ctx, value, STATS_VALUE_FONT, GRect(STATS_PAD_X, y, w - STATS_PAD_X * 2, STATS_VALUE_H), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
  return y + STATS_VALUE_H + STATS_GAP;
}

// Counts s_stats_projects' lines by kind: "\x02"-prefixed section headers draw
// as a black bar (STATS_LABEL_H + 2), the rest as plain lines (STATS_LINE_H).
static void stats_text_line_counts(int *label_lines, int *text_lines) {
  *label_lines = 0;
  *text_lines = 0;
  if (!s_stats_projects || s_stats_projects[0] == '\0') {
    *text_lines = 1;
    return;
  }
  bool sol = true;
  for (const char *p = s_stats_projects; *p; p++) {
    if (sol) {
      if (*p == '\x02') {
        (*label_lines)++;
      } else {
        (*text_lines)++;
      }
      sol = false;
    }
    if (*p == '\n') {
      sol = true;
    }
  }
}

static int16_t stats_content_height(void) {
  int label_lines, text_lines;
  stats_text_line_counts(&label_lines, &text_lines);
  return (s_yesterday_stats_enabled ? STATS_LABEL_H + 2 : 0)  // Today/Yesterday bar
       + (STATS_LABEL_H + STATS_VALUE_H + STATS_GAP) * 7       // the seven metrics
       + label_lines * (STATS_LABEL_H + 2)
       + text_lines * STATS_LINE_H
       + 8;
}

static void stats_content_update_proc(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  int16_t w = b.size.w;
  fill_bg(ctx, b, GColorWhite);

  if (!s_stats_have_data) {
    graphics_context_set_text_color(ctx, GColorBlack);
    draw_text(ctx, "Loading…", STATS_LINE_FONT, GRect(STATS_PAD_X, 8, w - STATS_PAD_X * 2, STATS_LINE_H), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
    return;
  }

  char done_buf[12];
  snprintf(done_buf, sizeof(done_buf), "%d", s_stats_done_today);
  focus_roll_day();
  char focus_buf[12];
  snprintf(focus_buf, sizeof(focus_buf), "%d", s_focus_completed_today);
  // "Break time" - count / total duration of today's real breaks.
  break_roll_day();
  char break_buf[24];
  if (s_break_count_today > 0) {
    char bdur[16];
    format_duration_ms(s_break_total_s_today * 1000, false, bdur, sizeof(bdur));
    snprintf(break_buf, sizeof(break_buf), "%d / %s", s_break_count_today, bdur);
  } else {
    str_copy(break_buf, "0", sizeof(break_buf));
  }
  // "Yesterday" view (toggled with long Up/Down): the two figures the phone
  // sends for that day; the live watch-local metrics show a dash.
  bool yd = s_stats_show_yesterday;
  char worked_y[24];
  if (yd) {
    format_duration_ms(s_stats_worked_yesterday_ms < 0 ? 0 : s_stats_worked_yesterday_ms,
                       false, worked_y, sizeof(worked_y));
    snprintf(done_buf, sizeof(done_buf), "%d", s_stats_done_yesterday);
  }

  int16_t y = 0;
  // When the yesterday toggle is on, a centred bar names which day is shown.
  if (s_yesterday_stats_enabled) {
    fill_bg(ctx, GRect(0, y, w, STATS_LABEL_H), GColorBlack);
    graphics_context_set_text_color(ctx, GColorWhite);
    draw_text(ctx, yd ? "Yesterday" : "Today", STATS_LABEL_FONT,
              GRect(STATS_PAD_X, y, w - STATS_PAD_X * 2, STATS_LABEL_H),
              GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter);
    y += STATS_LABEL_H + 2;
  }
  y = stats_draw_metric(ctx, y, w, "Estimate remaining", yd ? "-" : s_stats_est);
  y = stats_draw_metric(ctx, y, w, yd ? "Worked yesterday" : "Worked today", yd ? worked_y : s_stats_worked);
  y = stats_draw_metric(ctx, y, w, "Without a break", yd ? "-" : s_stats_nobreak);
  y = stats_draw_metric(ctx, y, w, "Break time", yd ? "-" : break_buf);
  y = stats_draw_metric(ctx, y, w, "Current session", yd ? "-" : s_stats_session);
  y = stats_draw_metric(ctx, y, w, yd ? "Done yesterday" : "Completed today", done_buf);
  y = stats_draw_metric(ctx, y, w, "Focus sessions", yd ? "-" : focus_buf);

  graphics_context_set_text_color(ctx, GColorBlack);
  GFont line_font = fonts_get_system_font(STATS_LINE_FONT);
  if (!s_stats_projects || s_stats_projects[0] == '\0') {
    graphics_draw_text(ctx, "None", line_font,
                        GRect(STATS_PAD_X, y, w - STATS_PAD_X * 2, STATS_LINE_H),
                        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    return;
  }
  // Verbatim lines from s_stats_projects: a "\x02" prefix marks a section
  // header (black bar), everything else is a plain line. See handleStatsRequest.
  const char *p = s_stats_projects;
  char line[80];
  while (*p) {
    const char *nl = strchr(p, '\n');
    size_t len = nl ? (size_t)(nl - p) : strlen(p);
    if (len >= sizeof(line)) {
      len = sizeof(line) - 1;
    }
    memcpy(line, p, len);
    line[len] = '\0';
    if (line[0] == '\x02') {
      fill_bg(ctx, GRect(0, y, w, STATS_LABEL_H), GColorBlack);
      graphics_context_set_text_color(ctx, GColorWhite);
      draw_text(ctx, line + 1, STATS_LABEL_FONT,
                GRect(STATS_PAD_X, y, w - STATS_PAD_X * 2, STATS_LABEL_H),
                GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
      graphics_context_set_text_color(ctx, GColorBlack);
      y += STATS_LABEL_H + 2;
    } else {
      graphics_draw_text(ctx, line, line_font,
                          GRect(STATS_PAD_X, y, w - STATS_PAD_X * 2, STATS_LINE_H),
                          GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
      y += STATS_LINE_H;
    }
    if (!nl) {
      break;
    }
    p = nl + 1;
  }
}

static void stats_render(void) {
  if (!s_stats_content_layer) {
    return;
  }
  stats_compute_values();
  int16_t h = s_stats_have_data ? stats_content_height() : s_stats_content_bounds.size.h;
  if (h < s_stats_content_bounds.size.h) {
    h = s_stats_content_bounds.size.h;
  }
  layer_set_frame(s_stats_content_layer, GRect(0, 0, s_stats_content_bounds.size.w, h));
  scroll_layer_set_content_size(s_stats_scroll_layer, GSize(s_stats_content_bounds.size.w, h));
  layer_mark_dirty(s_stats_content_layer);
}

static void request_stats(void) {
  begin_send(MSG_STATS_REQUEST, NULL, NULL, 0);
}

// Long Up/Down on the Stats page flips between today and yesterday, when the
// "Yesterday toggle" setting is on. Wraps the scroll layer's own click config.
static ClickConfigProvider s_stats_ccp = NULL;

static void stats_toggle_yesterday_handler(ClickRecognizerRef recognizer, void *context) {
  if (!s_yesterday_stats_enabled) {
    return;
  }
  s_stats_show_yesterday = !s_stats_show_yesterday;
  vibes_short_pulse();
  stats_render();
}

static void stats_window_click_config_provider(void *context) {
  if (s_stats_ccp) {
    s_stats_ccp(context);
  }
  window_long_click_subscribe(BUTTON_ID_UP, 0, stats_toggle_yesterday_handler, NULL);
  window_long_click_subscribe(BUTTON_ID_DOWN, 0, stats_toggle_yesterday_handler, NULL);
}

static void stats_window_load(Window *window) {
  Layer *window_layer;
  s_stats_content_bounds = window_chrome(window, &s_stats_status_bar, &window_layer);

  // Heap-backed text buffer, alive only for this window (see its declaration).
  free(s_stats_projects);
  s_stats_projects = malloc(STATS_TEXT_CAP);
  if (s_stats_projects) {
    s_stats_projects[0] = '\0';
  }
  s_stats_have_data = false;

  s_stats_scroll_layer = scroll_layer_create(s_stats_content_bounds);
  scroll_layer_set_content_size(s_stats_scroll_layer, s_stats_content_bounds.size);
  scroll_layer_set_click_config_onto_window(s_stats_scroll_layer, window);
  s_stats_ccp = window_get_click_config_provider(window);
  window_set_click_config_provider_with_context(window, stats_window_click_config_provider,
                                                window_get_click_config_context(window));

  s_stats_content_layer = layer_create(GRect(0, 0, s_stats_content_bounds.size.w,
                                             s_stats_content_bounds.size.h));
  layer_set_update_proc(s_stats_content_layer, stats_content_update_proc);
  scroll_layer_add_child(s_stats_scroll_layer, s_stats_content_layer);
  layer_add_child(window_layer, scroll_layer_get_layer(s_stats_scroll_layer));

  stats_render();
}

static void stats_window_unload(Window *window) {
  s_stats_show_yesterday = false;
  s_stats_ccp = NULL;
  layer_destroy(s_stats_content_layer);
  s_stats_content_layer = NULL;
  scroll_layer_destroy(s_stats_scroll_layer);
  s_stats_scroll_layer = NULL;
  status_bar_layer_destroy(s_stats_status_bar);
  s_stats_status_bar = NULL;
  free(s_stats_projects);
  s_stats_projects = NULL;
  s_stats_have_data = false;
}

// Created once and reused, like push_notes_window. Keeps whatever data the
// last visit fetched on screen and re-requests in the background.
static void push_stats_window(void) {
  if (!s_stats_window) {
    s_stats_window = window_create();
    window_set_window_handlers(s_stats_window, (WindowHandlers) {
      .load = stats_window_load,
      .unload = stats_window_unload,
    });
  }
  window_stack_push(s_stats_window, true);
  request_stats();
}

// ---------- upcoming window ----------
// Optional section-0 row ("Upcoming"). A read-only scrollable list of every
// task scheduled for a day AFTER today, grouped by day - the phone computes it
// (store.computeUpcoming) and sends the whole thing preformatted in one
// MSG_UPCOMING_DATA: a "\x02"-prefixed day header line before each day's tasks.
// The watch just parses and draws, reusing the Stats page's fonts/metrics.
static Window *s_upcoming_window;
static StatusBarLayer *s_upcoming_status_bar;
static ScrollLayer *s_upcoming_scroll_layer;
static Layer *s_upcoming_content_layer;
static GRect s_upcoming_content_bounds;

static int16_t upcoming_content_height(void) {
  int16_t h = 8;
  if (!s_upcoming_text) {
    return h;
  }
  const char *p = s_upcoming_text;
  bool at_line_start = true;
  for (; *p; p++) {
    if (at_line_start) {
      h += (*p == '\x02') ? STATS_LABEL_H : STATS_LINE_H;
    }
    at_line_start = (*p == '\n');
  }
  return h;
}

static void upcoming_content_update_proc(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  int16_t w = b.size.w;
  fill_bg(ctx, b, GColorWhite);

  if (!s_upcoming_have_data || !s_upcoming_text || s_upcoming_text[0] == '\0') {
    graphics_context_set_text_color(ctx, GColorBlack);
    const char *empty = s_page_mode == PAGE_NOTES ? "No pinned notes" : "Nothing scheduled";
    draw_text(ctx, s_upcoming_have_data ? empty : "Loading…", STATS_LINE_FONT,
              GRect(STATS_PAD_X, 8, w - STATS_PAD_X * 2, STATS_LINE_H),
              GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
    return;
  }

  const char *p = s_upcoming_text;
  char line[96];
  int16_t y = 4;
  while (*p) {
    const char *nl = strchr(p, '\n');
    size_t len = nl ? (size_t)(nl - p) : strlen(p);
    bool header = (*p == '\x02');
    const char *src = header ? p + 1 : p;
    size_t slen = header ? (len ? len - 1 : 0) : len;
    if (slen >= sizeof(line)) {
      slen = sizeof(line) - 1;
    }
    memcpy(line, src, slen);
    line[slen] = '\0';
    if (header) {
      fill_bg(ctx, GRect(0, y, w, STATS_LABEL_H), GColorBlack);
      graphics_context_set_text_color(ctx, GColorWhite);
      draw_text(ctx, line, STATS_LABEL_FONT, GRect(STATS_PAD_X, y, w - STATS_PAD_X * 2, STATS_LABEL_H), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
      y += STATS_LABEL_H;
    } else {
      graphics_context_set_text_color(ctx, GColorBlack);
      draw_text(ctx, line, STATS_LINE_FONT, GRect(STATS_PAD_X, y, w - STATS_PAD_X * 2, STATS_LINE_H), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
      y += STATS_LINE_H;
    }
    if (!nl) {
      break;
    }
    p = nl + 1;
  }
}

static void upcoming_render(void) {
  if (!s_upcoming_content_layer) {
    return;
  }
  int16_t h = s_upcoming_have_data ? upcoming_content_height() : s_upcoming_content_bounds.size.h;
  if (h < s_upcoming_content_bounds.size.h) {
    h = s_upcoming_content_bounds.size.h;
  }
  layer_set_frame(s_upcoming_content_layer, GRect(0, 0, s_upcoming_content_bounds.size.w, h));
  scroll_layer_set_content_size(s_upcoming_scroll_layer, GSize(s_upcoming_content_bounds.size.w, h));
  layer_mark_dirty(s_upcoming_content_layer);
}

static void request_upcoming(void) {
  begin_send(s_page_mode == PAGE_NOTES ? MSG_NOTESPAGE_REQUEST : MSG_UPCOMING_REQUEST, NULL, NULL, 0);
}

static void upcoming_window_load(Window *window) {
  Layer *window_layer;
  free(s_upcoming_text);
  s_upcoming_text = malloc(PAGE_TEXT_CAP);
  if (s_upcoming_text) {
    s_upcoming_text[0] = '\0';
  }
  s_upcoming_have_data = false;
  s_upcoming_content_bounds = window_chrome(window, &s_upcoming_status_bar, &window_layer);
  s_upcoming_scroll_layer = scroll_layer_create(s_upcoming_content_bounds);
  scroll_layer_set_content_size(s_upcoming_scroll_layer, s_upcoming_content_bounds.size);
  scroll_layer_set_click_config_onto_window(s_upcoming_scroll_layer, window);
  s_upcoming_content_layer = layer_create(GRect(0, 0, s_upcoming_content_bounds.size.w,
                                                s_upcoming_content_bounds.size.h));
  layer_set_update_proc(s_upcoming_content_layer, upcoming_content_update_proc);
  scroll_layer_add_child(s_upcoming_scroll_layer, s_upcoming_content_layer);
  layer_add_child(window_layer, scroll_layer_get_layer(s_upcoming_scroll_layer));
  upcoming_render();
}

static void upcoming_window_unload(Window *window) {
  layer_destroy(s_upcoming_content_layer);
  s_upcoming_content_layer = NULL;
  scroll_layer_destroy(s_upcoming_scroll_layer);
  s_upcoming_scroll_layer = NULL;
  status_bar_layer_destroy(s_upcoming_status_bar);
  s_upcoming_status_bar = NULL;
  free(s_upcoming_text);
  s_upcoming_text = NULL;
  s_upcoming_have_data = false;
}

static void push_page_window(PageMode mode) {
  s_page_mode = mode; // window_load allocs + clears s_upcoming_text
  if (!s_upcoming_window) {
    s_upcoming_window = window_create();
    window_set_window_handlers(s_upcoming_window, (WindowHandlers) {
      .load = upcoming_window_load,
      .unload = upcoming_window_unload,
    });
  }
  window_stack_push(s_upcoming_window, true);
  request_upcoming();
}
#endif

// ---------- schedule window ----------
// A time-ordered view of today's tasks: every s_tasks entry with a due_min
// (a Super Productivity dueWithTime), sorted ascending, like the desktop's
// schedule-day panel. Pure watch-side - no phone request, it just re-reads
// the already-synced list. Select toggles done, long-select toggles time
// tracking, matching the main list. aplite-excluded (RAM), same as Stats.
#ifndef PBL_PLATFORM_APLITE
static Window *s_schedule_window;
static MenuLayer *s_schedule_menu_layer;
static TextLayer *s_schedule_empty_layer;
static StatusBarLayer *s_schedule_status_bar;
// s_tasks indices with a due_min, sorted by due_min ascending. Rebuilt on
// open and whenever a sync replaces the list (schedule_refresh_if_open).
static int s_schedule_order[MAX_TASKS];
static int s_schedule_count = 0;

static void schedule_rebuild(void) {
  s_schedule_count = 0;
  for (int i = 0; i < s_task_count && s_schedule_count < MAX_TASKS; i++) {
    if (s_tasks[i].due_min >= 0) {
      s_schedule_order[s_schedule_count++] = i;
    }
  }
  // Insertion sort by due_min - stable, so same-time tasks keep the phone's
  // order. s_schedule_count is small (today's timed tasks only).
  for (int a = 1; a < s_schedule_count; a++) {
    int key = s_schedule_order[a];
    int key_min = s_tasks[key].due_min;
    int b = a - 1;
    while (b >= 0 && s_tasks[s_schedule_order[b]].due_min > key_min) {
      s_schedule_order[b + 1] = s_schedule_order[b];
      b--;
    }
    s_schedule_order[b + 1] = key;
  }
}

static Task *schedule_task_at(MenuIndex index) {
  if (index.section != 0 || (int)index.row >= s_schedule_count) {
    return NULL;
  }
  int ti = s_schedule_order[index.row];
  if (ti < 0 || ti >= s_task_count) {
    return NULL;
  }
  return &s_tasks[ti];
}

static void schedule_update_empty_layer(void) {
  bool show_empty = (s_schedule_count == 0);
  layer_set_hidden(text_layer_get_layer(s_schedule_empty_layer), !show_empty);
  layer_set_hidden(menu_layer_get_layer(s_schedule_menu_layer), show_empty);
}

// Rebuild + redraw the sub-page if it's currently loaded (menu layer alive).
static void schedule_refresh_if_open(void) {
  if (!s_schedule_menu_layer) {
    return;
  }
  schedule_rebuild();
  menu_layer_reload_data(s_schedule_menu_layer);
  schedule_update_empty_layer();
}

static uint16_t schedule_menu_get_num_sections(MenuLayer *menu_layer, void *context) {
  return 1;
}

static uint16_t schedule_menu_get_num_rows(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  return (uint16_t)s_schedule_count;
}

static void schedule_menu_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context) {
  Task *task = schedule_task_at(*cell_index);
  if (!task) {
    return;
  }
  bool is_selected = menu_layer_get_selected_index(s_schedule_menu_layer).row == cell_index->row;
  // Reuse the today-list row renderer - same look (dim-on-done, "@ 9:41 AM"
  // due subtitle, tracked/estimate time), with the project name shown
  // right-aligned since this view crosses projects.
  draw_task_row(ctx, layer_get_bounds(cell_layer), task, is_selected, true);
}

static void schedule_menu_select_click(MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  backlight_touch();
#if defined(PBL_TOUCH)
  if (consume_tap_select_guard()) {
    return;
  }
#endif
  Task *task = schedule_task_at(*cell_index);
  if (!task) {
    return;
  }
  // Immediate optimistic toggle (no double-click-for-notes window here) - the
  // next full sync corrects it, same as the aplite path in menu_select_click.
  task->done = !task->done;
  save_tasks();
  menu_layer_reload_data(s_schedule_menu_layer);
  send_task_toggle(task);
}

static void schedule_menu_select_long_click(MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  backlight_touch();
  Task *task = schedule_task_at(*cell_index);
  if (!task || task->done) {
    return;
  }
  bool already_tracking_this = s_tracking_task_id[0] != '\0' &&
                                strncmp(s_tracking_task_id, task->id, MAX_ID_LEN) == 0;
  stop_tracking_and_report();
  if (!already_tracking_this) {
    start_tracking(task);
  }
  menu_layer_reload_data(s_schedule_menu_layer);
}

static void schedule_menu_selection_changed(MenuLayer *menu_layer, MenuIndex new_index, MenuIndex old_index, void *context) {
  backlight_touch();
}

static void schedule_window_load(Window *window) {
  Layer *window_layer;
  GRect content_bounds = window_chrome(window, &s_schedule_status_bar, &window_layer);

  schedule_rebuild();

  s_schedule_menu_layer = menu_layer_create(content_bounds);
  menu_layer_set_callbacks(s_schedule_menu_layer, NULL, (MenuLayerCallbacks) {
    .get_num_sections = schedule_menu_get_num_sections,
    .get_num_rows = schedule_menu_get_num_rows,
    .draw_row = schedule_menu_draw_row,
    .select_click = schedule_menu_select_click,
    .select_long_click = schedule_menu_select_long_click,
    .selection_changed = schedule_menu_selection_changed,
  });
  menu_layer_set_click_config_onto_window(s_schedule_menu_layer, window);
  layer_add_child(window_layer, menu_layer_get_layer(s_schedule_menu_layer));

  s_schedule_empty_layer = make_text_layer(window_layer, content_bounds,
                                           EMPTY_MSG_FONT_KEY, GTextAlignmentCenter);
  text_layer_set_text(s_schedule_empty_layer, "Nothing scheduled for today.");

  schedule_update_empty_layer();
}

static void schedule_window_unload(Window *window) {
  menu_layer_destroy(s_schedule_menu_layer);
  s_schedule_menu_layer = NULL;
  text_layer_destroy(s_schedule_empty_layer);
  s_schedule_empty_layer = NULL;
  status_bar_layer_destroy(s_schedule_status_bar);
  s_schedule_status_bar = NULL;
}

// Created once and reused, like push_habits_window / push_stats_window.
static void push_schedule_window(void) {
  if (!s_schedule_window) {
    s_schedule_window = window_create();
    window_set_window_handlers(s_schedule_window, (WindowHandlers) {
      .load = schedule_window_load,
      .unload = schedule_window_unload,
    });
  }
  window_stack_push(s_schedule_window, true);
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

// ---- focus mode ----
// There is NO API to disable PebbleOS's inactivity auto-close. The lever we
// have: the tracking tick (live_tick_callback) calls light_enable_interaction()
// every FOCUS_LIGHT_POKE_S while a focus session's screen is on top - a brief
// backlight pulse that fades on its own, same as a button press, not a latch.
// It may still time out on real hardware; when it does, the persisted
// s_focus_end_epoch means init() drops straight back onto the focus screen.

// Ends the running focus session: clears state, buzzes, repaints the header
// strip. `notify` = ran to completion (long buzz + banner); else a plain stop.
// Nothing to undo for the backlight - the pulses fade themselves.
static void focus_end(bool ran_out) {
  // A break that ran out: fully done.
  if (ran_out && s_focus_on_break) {
    s_focus_on_break = false;
    s_focus_end_epoch = 0;
    save_focus();
    vibes_double_pulse();
    show_top_banner("Break over");
    menu_layer_reload_data(s_menu_layer);
    return;
  }
  // A work session that ran out, with Pomodoro timing on: chain into a break.
  if (ran_out && s_use_pomodoro_cfg && s_pomodoro_break_min > 0) {
    focus_bump_completed(); // the work session counts for the Stats page
    s_focus_on_break = true;
    s_focus_end_epoch = time(NULL) + (time_t)s_pomodoro_break_min * 60;
    save_focus();
    vibes_long_pulse();
    snprintf(s_overtime_banner_text, sizeof(s_overtime_banner_text),
             "Break: %d min", s_pomodoro_break_min);
    show_top_banner(s_overtime_banner_text);
    menu_layer_reload_data(s_menu_layer);
    return;
  }
  // Plain end - a manual stop, or completion with no break to chain.
  s_focus_on_break = false;
  s_focus_end_epoch = 0;
  save_focus();
  if (ran_out) {
    vibes_long_pulse();
    show_top_banner("Focus done");
    focus_bump_completed();
  } else {
    vibes_short_pulse();
  }
  menu_layer_reload_data(s_menu_layer);
}

// Toggles focus on the current LOCAL tracking session. A no-op when nothing
// is tracked locally. Bound to long-press UP and DOWN on the tracking window.
static void focus_toggle(void) {
  if (s_focus_end_epoch != 0) {
    focus_end(false);
  } else {
    if (s_tracking_task_id[0] == '\0') {
      return;
    }
    s_focus_on_break = false;
    int len = s_use_pomodoro_cfg ? s_pomodoro_work_min : s_focus_len_min;
    s_focus_end_epoch = time(NULL) + (time_t)len * 60;
    save_focus();
    vibes_short_pulse();
    light_enable_interaction();
    s_focus_last_poke_epoch = time(NULL); // next tick pulse is FOCUS_LIGHT_POKE_S out
    menu_layer_reload_data(s_menu_layer);
  }
  live_window_refresh();
}

// Fills the elapsed layer: "spent / estimate" (est_ms > 0) or a running
// H:MM:SS clock, with the font sized to match. Shared by the local and remote
// branches of live_window_refresh. `buf` must outlive the call (a static).
static void live_set_elapsed(char *buf, size_t bufsize, int spent_ms, int est_ms, int session_s) {
  if (session_s < 0) {
    session_s = 0;
  }
  if (est_ms > 0) {
    // A step down in font size - "spent / estimate" is too wide for GOTHIC_28
    // on a 144px watch.
    char spent_text[20], estimate_text[16];
    format_duration_ms(spent_ms + session_s * 1000, false, spent_text, sizeof(spent_text));
    format_duration_ms(est_ms, false, estimate_text, sizeof(estimate_text));
    snprintf(buf, bufsize, "%s / %s", spent_text, estimate_text);
    text_layer_set_font(s_live_elapsed_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  } else {
    int h = session_s / 3600, m = (session_s % 3600) / 60, s = session_s % 60;
    if (h > 0) {
      snprintf(buf, bufsize, "%d:%02d:%02d", h, m, s);
    } else {
      snprintf(buf, bufsize, "%d:%02d", m, s);
    }
    text_layer_set_font(s_live_elapsed_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  }
  text_layer_set_text(s_live_elapsed_layer, buf);
  layer_set_hidden(text_layer_get_layer(s_live_elapsed_layer), false);
}

// Rewrites the tracking-detail window's text. Two sources: this watch's own
// timer (s_tracking_*) when it's tracking, otherwise a remote device's presence
// session (s_presence_*). (Re)arms the 1s elapsed tick while tracking. A no-op
// unless the window is on top; pops when nothing is being tracked anywhere.
static void live_window_refresh(void) {
  if (!s_live_window || window_stack_get_top_window() != s_live_window) {
    return;
  }
#ifdef PBL_PLATFORM_EMERY
  if (s_live_arc_layer) {
    layer_mark_dirty(s_live_arc_layer); // depletes with the focus countdown
  }
#endif
  static char elapsed_buf[32];

  if (s_tracking_task_id[0] != '\0') {
    Task *t = find_task_by_id(s_tracking_task_id);
    if (!t) {
      stop_live_tick();
      window_stack_pop(true);
      return;
    }
    // A focus session that has run out ends here (the 1s tick keeps calling
    // us) - falls through to the plain "Tracking" display below.
    if (s_focus_end_epoch != 0 && time(NULL) >= s_focus_end_epoch) {
      focus_end(true);
    }
    if (focus_active()) {
      int left_s = (int)(s_focus_end_epoch - time(NULL));
      if (left_s < 0) {
        left_s = 0;
      }
      snprintf(elapsed_buf, sizeof(elapsed_buf), "%d:%02d", left_s / 60, left_s % 60);
      text_layer_set_text(s_live_state_layer, s_focus_on_break ? "Break" : "Focusing");
      text_layer_set_text(s_live_task_layer, t->title);
      text_layer_set_font(s_live_elapsed_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
      text_layer_set_text(s_live_elapsed_layer, elapsed_buf);
      layer_set_hidden(text_layer_get_layer(s_live_elapsed_layer), false);
      text_layer_set_text(s_live_hint_layer, "Hold to end");
    } else {
      text_layer_set_text(s_live_state_layer, "Tracking");
      text_layer_set_text(s_live_task_layer, t->title);
      live_set_elapsed(elapsed_buf, sizeof(elapsed_buf), t->time_spent_ms, t->time_estimate_ms,
                       (int)(time(NULL) - s_tracking_start_epoch));
      text_layer_set_text(s_live_hint_layer, "Select=stop  hold=focus");
    }
    if (!s_live_tick_timer) {
      s_live_tick_timer = app_timer_register(TRACKING_TICK_INTERVAL_MS, live_tick_callback, NULL);
    }
    return;
  }

  if (s_presence_state == 0) {
    stop_live_tick();
    window_stack_pop(true);
    return;
  }

  text_layer_set_text(s_live_state_layer, presence_state_phrase());
  text_layer_set_text(s_live_task_layer, s_presence_task);

  if (s_presence_state == 1) {
    live_set_elapsed(elapsed_buf, sizeof(elapsed_buf), s_presence_spent_ms, s_presence_estimate_ms,
                     (int)(time(NULL) - s_presence_elapsed_base));
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
  // Pulse the backlight every FOCUS_LIGHT_POKE_S during a focus session - the
  // only lever against the inactivity auto-close (see the focus-mode comment).
  // The 1s tick still runs for the countdown; the pulse is throttled and fades
  // on its own like a button press.
  if (focus_active()) {
    time_t now = time(NULL);
    if (now - s_focus_last_poke_epoch >= FOCUS_LIGHT_POKE_S) {
      s_focus_last_poke_epoch = now;
      light_enable_interaction();
    }
  }
  if (s_tracking_task_id[0] != '\0' || s_presence_state == 1) {
    live_window_refresh(); // re-arms the timer, or stops if the window closed
  }
}

// stop_tracking_and_report() ends with live_window_refresh(), which pops this
// screen once nothing is tracking. Also ends any focus session.
static void live_stop_local_tracking(void) {
  stop_tracking_and_report();
  menu_layer_reload_data(s_menu_layer);
  refresh_scroll_state(true);
}

static void live_window_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  // In focus mode a short Select is a no-op - only a long hold ends the
  // session, so a stray press can't drop you out mid-focus.
  if (focus_active()) {
    return;
  }
  if (s_tracking_task_id[0] != '\0') {
    live_stop_local_tracking();
    return;
  }
  if (s_presence_can_stop && !s_presence_stopping) {
    s_presence_stopping = true;
    send_presence_stop();
    live_window_refresh(); // show "Stopping..."; the phone clears us on its ack
  }
}

// Long-Select stops the local timer (which also ends a focus session). Long
// Up/Down end focus but leave the timer running.
static void live_window_select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  backlight_touch();
  if (s_tracking_task_id[0] != '\0') {
    live_stop_local_tracking();
  }
}

static void focus_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  backlight_touch();
  focus_toggle();
}

// While a focus session runs, Back is trapped on this screen - the point of
// focus mode is that a stray press doesn't drop you to the watchface. A long
// hold of Up / Down / Select ends it. Without a session Back just pops.
static void live_window_back_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (focus_active()) {
    vibes_short_pulse();
    return;
  }
  window_stack_pop(true);
}

static void live_window_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, live_window_select_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 0, live_window_select_long_click_handler, NULL);
  window_single_click_subscribe(BUTTON_ID_BACK, live_window_back_click_handler);
  window_long_click_subscribe(BUTTON_ID_UP, 0, focus_long_click_handler, NULL);
  window_long_click_subscribe(BUTTON_ID_DOWN, 0, focus_long_click_handler, NULL);
}

#ifdef PBL_PLATFORM_EMERY
// Frames the live window with a ring that depletes clockwise from 12 o'clock as
// the focus session's time runs out. Draws nothing outside a focus session, so
// plain tracking / remote presence keep the clean text-only look.
static void live_arc_update_proc(Layer *layer, GContext *ctx) {
  if (!focus_active()) {
    return;
  }
  int total_min = s_focus_on_break ? s_pomodoro_break_min
                  : (s_use_pomodoro_cfg ? s_pomodoro_work_min : s_focus_len_min);
  int total_s = total_min * 60;
  if (total_s <= 0) {
    return;
  }
  int left_s = (int)(s_focus_end_epoch - time(NULL));
  if (left_s < 0) {
    left_s = 0;
  }
  if (left_s > total_s) {
    left_s = total_s;
  }
  GRect b = layer_get_bounds(layer);
  int r = (b.size.w < b.size.h ? b.size.w : b.size.h) / 2 - 6;
  GPoint c = grect_center_point(&b);
  GRect ring = GRect(c.x - r, c.y - r, 2 * r, 2 * r);
  // via degrees to keep the arithmetic in int32 (no 64-bit divmod helper).
  int32_t sweep = DEG_TO_TRIGANGLE(left_s * 360 / total_s);
#ifdef PBL_COLOR
  graphics_context_set_stroke_color(ctx, GColorLightGray);
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_arc(ctx, ring, GOvalScaleModeFitCircle, 0, TRIG_MAX_ANGLE);
#endif
  graphics_context_set_stroke_color(ctx,
      PBL_IF_COLOR_ELSE(s_focus_on_break ? GColorVividCerulean : GColorJaegerGreen, GColorBlack));
  graphics_context_set_stroke_width(ctx, 4);
  graphics_draw_arc(ctx, ring, GOvalScaleModeFitCircle, 0, sweep);
  graphics_context_set_stroke_width(ctx, 1);
}
#endif // PBL_PLATFORM_EMERY

static void live_window_load(Window *window) {
  Layer *window_layer;
  GRect content = window_chrome(window, &s_live_status_bar, &window_layer);

  int16_t x = content.origin.x + 6;
  int16_t w = content.size.w - 12;
  int16_t y = content.origin.y + 6;
  int16_t bottom = content.origin.y + content.size.h;

#ifdef PBL_PLATFORM_EMERY
  // The focus ring sits behind everything (added first).
  s_live_arc_layer = layer_create(content);
  layer_set_update_proc(s_live_arc_layer, live_arc_update_proc);
  layer_add_child(window_layer, s_live_arc_layer);
#endif

  // Task name first (what the user asked to see), wrapping to two lines.
  s_live_task_layer = make_text_layer(window_layer, GRect(x, y, w, 50),
                                      FONT_KEY_GOTHIC_24_BOLD, GTextAlignmentCenter);
  text_layer_set_overflow_mode(s_live_task_layer, GTextOverflowModeWordWrap);
  y += 54;

  // Then the state line ("Tracking on Desktop" / "Stopped on Desktop"), smaller.
  s_live_state_layer = make_text_layer(window_layer, GRect(x, y, w, 22),
                                       FONT_KEY_GOTHIC_18, GTextAlignmentCenter);
  y += 26;

  s_live_elapsed_layer = make_text_layer(window_layer, GRect(x, y, w, 30),
                                         FONT_KEY_GOTHIC_28_BOLD, GTextAlignmentCenter);

  s_live_hint_layer = make_text_layer(window_layer, GRect(x, bottom - 20, w, 18),
                                      FONT_KEY_GOTHIC_14, GTextAlignmentCenter);
  text_layer_set_text_color(s_live_hint_layer, GColorDarkGray);

  window_set_click_config_provider(window, live_window_click_config_provider);
  live_window_refresh();
}

static void live_window_unload(Window *window) {
  stop_live_tick();
#ifdef PBL_PLATFORM_EMERY
  layer_destroy(s_live_arc_layer);
  s_live_arc_layer = NULL;
#endif
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
  if (s_presence_state == 0 && s_tracking_task_id[0] == '\0') {
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

// MenuLayer owns the whole main-window click config now: UP/DOWN scroll, SELECT
// single (toggle / double-click notes) and long (the per-task action menu, where
// scheduling now lives - it used to be long-press UP/DOWN here).

static void window_load(Window *window) {
  Layer *window_layer;
  GRect content_bounds = window_chrome(window, &s_status_bar, &window_layer);

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
  // A cached list may already have a selection that needs to scroll.
  refresh_scroll_state(true);

  // Logo in a fixed strip at the bottom of the empty-state area; the text layer
  // gets the space above it.
  #define LOGO_SIZE 50
  #define LOGO_STRIP_HEIGHT 58
  GRect empty_text_bounds = GRect(content_bounds.origin.x, content_bounds.origin.y,
                                   content_bounds.size.w, content_bounds.size.h - LOGO_STRIP_HEIGHT);
  s_empty_layer = make_text_layer(window_layer, empty_text_bounds,
                                  EMPTY_MSG_FONT_KEY, GTextAlignmentCenter);

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
  s_sync_progress_layer = make_text_layer(window_layer, sync_progress_bounds,
                                          CHROME_FONT_KEY, GTextAlignmentCenter);
  layer_set_hidden(text_layer_get_layer(s_sync_progress_layer), true);
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
#ifndef PBL_PLATFORM_APLITE
  s_project_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PROJECT);
  s_project_white_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PROJECT_WHITE);
  s_stats_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_STATS);
  s_stats_white_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_STATS_WHITE);
  s_inbox_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_INBOX);
  s_inbox_white_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_INBOX_WHITE);
#endif

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
  s_error_layer = make_text_layer(window_layer, content_bounds, TITLE_FONT_KEY, GTextAlignmentCenter);
  text_layer_set_background_color(s_error_layer, GColorRed);
  // White, not black - better contrast on red, and on aplite GColorRed may
  // reduce to black, which would make black text invisible.
  text_layer_set_text_color(s_error_layer, GColorWhite);
  text_layer_set_overflow_mode(s_error_layer, GTextOverflowModeWordWrap);
  layer_set_hidden(text_layer_get_layer(s_error_layer), true);

#ifndef PBL_PLATFORM_APLITE
  // Over-estimate banner - a red strip across the top of the list, same
  // GColorRed + white-bold as s_error_layer. Hidden until a tracked task first
  // crosses its estimate.
  GRect overtime_bounds = GRect(content_bounds.origin.x, content_bounds.origin.y,
                                 content_bounds.size.w, OVERTIME_BANNER_HEIGHT);
  s_banner_frame = overtime_bounds; // resting frame for banner_slide
  s_overtime_banner_layer = make_text_layer(window_layer, overtime_bounds,
                                            TITLE_FONT_KEY, GTextAlignmentCenter);
  text_layer_set_background_color(s_overtime_banner_layer, GColorRed);
  text_layer_set_text_color(s_overtime_banner_layer, GColorWhite);
  text_layer_set_overflow_mode(s_overtime_banner_layer, GTextOverflowModeTrailingEllipsis);
  layer_set_hidden(text_layer_get_layer(s_overtime_banner_layer), true);
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
#ifdef PBL_PLATFORM_EMERY
  if (s_pending_done_timer) {
    app_timer_cancel(s_pending_done_timer);
    s_pending_done_timer = NULL;
  }
  s_pending_done_task_id[0] = '\0';
#endif
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
  gbitmap_destroy(s_project_bitmap);
  gbitmap_destroy(s_project_white_bitmap);
  gbitmap_destroy(s_stats_bitmap);
  gbitmap_destroy(s_stats_white_bitmap);
  gbitmap_destroy(s_inbox_bitmap);
  gbitmap_destroy(s_inbox_white_bitmap);
#endif
#ifndef PBL_PLATFORM_APLITE
  dictation_session_destroy(s_dictation_session);
  gbitmap_destroy(s_mic_bitmap);
  gbitmap_destroy(s_mic_white_bitmap);
#endif
  status_bar_layer_destroy(s_status_bar);
}

#ifdef BREAK_REMINDER
// "Not tracking" nudge - purely time-based (no step count), checked once a
// minute while the app is open and nothing is tracked. Counts minutes since
// the last stop and repeats every s_idle_reminder_min minutes until tracking
// resumes (start_tracking resets the count). Idle-while-tracking has no
// separate behaviour of its own - the break reminder already covers that off
// accumulated tracked time.
static void maybe_notify_idle(void) {
  if (s_idle_reminder_min <= 0 || s_error_overlay_active ||
      s_tracking_task_id[0] != '\0' || s_break_last_stop_epoch == 0) {
    return;
  }
  // A remote device actively tracking counts as "something is being tracked" -
  // no nudge, and hold the idle baseline at now so the count restarts from
  // when that remote session ends, not from this watch's last local stop.
  if (s_presence_state == 1) {
    s_break_last_stop_epoch = time(NULL);
    s_untracked_notify_count = 0;
    return;
  }
  int elapsed_min = (int)((time(NULL) - s_break_last_stop_epoch) / 60);
  int intervals = elapsed_min / s_idle_reminder_min;
  if (intervals <= s_untracked_notify_count) {
    return; // already nudged for the current interval
  }
  // Jump straight to the current interval instead of incrementing by one:
  // s_untracked_notify_count starts at 0 every app open, so if the app was
  // last open (or tracking last stopped) several intervals ago, a plain ++
  // fired the banner every single tick until the count caught up. Now it
  // fires at most once per tick, on a real interval boundary.
  s_untracked_notify_count = intervals;
  snprintf(s_overtime_banner_text, sizeof(s_overtime_banner_text),
           "Not tracking\n%d min", elapsed_min);
  show_top_banner(s_overtime_banner_text);
}
#endif

#ifndef PBL_PLATFORM_APLITE
// config.stopAtMidnight: close out a session still running past the local
// midnight. A local timer is stopped and its time logged only up to 00:00
// (stop_tracking_at); a remote session gets a stop request - the producing
// device owns how its time splits across the day. Runs on the minute tick,
// so a timer left going overnight is caught the next time the app is opened,
// not only if the app happens to be foregrounded at midnight.
static void maybe_stop_at_midnight(struct tm *now_tm) {
  if (!s_stop_at_midnight) {
    return;
  }
  time_t now = time(NULL);
  time_t today_start = now - (now_tm->tm_hour * 3600 + now_tm->tm_min * 60 + now_tm->tm_sec);

  if (s_tracking_task_id[0] != '\0' && s_tracking_start_epoch < today_start) {
    stop_tracking_at(today_start);
    menu_layer_reload_data(s_menu_layer);
    refresh_scroll_state(true);
    return;
  }
  if (s_presence_state == 1 && s_presence_can_stop && !s_presence_stopping &&
      s_presence_elapsed_base != 0 && s_presence_elapsed_base < today_start) {
    s_presence_stopping = true;
    send_presence_stop();
    live_window_refresh();
  }
}

// From 18:00, once per local day: nudge if a habit with a 2+ day streak still
// isn't done. s_streak_nudge_day latches on tm_yday. Undone means the streak
// is measured behind today (it's still alive right now but breaks at midnight),
// which is exactly what habitStreak sends as `streak` for a not-done habit.
static void maybe_notify_streak_at_risk(struct tm *now_tm) {
  if (!s_habit_streak_nudge || s_error_overlay_active || now_tm->tm_hour < 18 ||
      s_streak_nudge_day == now_tm->tm_yday) {
    return;
  }
  int at_risk = 0;
  const char *one_title = NULL;
  for (int i = 0; i < s_habit_count; i++) {
    if (!s_habits[i].done && s_habits[i].streak >= 2) {
      at_risk++;
      one_title = s_habits[i].title;
    }
  }
  if (at_risk == 0) {
    return;
  }
  s_streak_nudge_day = now_tm->tm_yday;
  if (at_risk == 1) {
    snprintf(s_overtime_banner_text, sizeof(s_overtime_banner_text),
             "Keep your streak\n%s", one_title);
  } else {
    snprintf(s_overtime_banner_text, sizeof(s_overtime_banner_text),
             "Keep your streaks\n%d habits", at_risk);
  }
  show_top_banner(s_overtime_banner_text);
}

// MINUTE_UNIT tick: when s_due_reminder_min is set, fires the banner as the
// soonest upcoming timed task comes within that window; also drives the idle
// check above. App-open only.
static void minute_tick_handler(struct tm *now_tm, TimeUnits units_changed) {
  maybe_stop_at_midnight(now_tm);
#ifdef BREAK_REMINDER
  maybe_notify_idle();
#endif
  maybe_notify_streak_at_risk(now_tm);
  if (s_error_overlay_active) {
    return;
  }
  int now_min = now_tm->tm_hour * 60 + now_tm->tm_min;

  // Per-task reminders (task.remindAt from the desktop): fire once when the
  // clock reaches the set time. One banner per tick; the rest catch up on
  // later ticks.
  for (int i = 0; i < s_task_count; i++) {
    if (s_tasks[i].remind_min >= 0 && !s_tasks[i].done && !s_tasks[i].remind_fired &&
        now_min >= s_tasks[i].remind_min) {
      s_tasks[i].remind_fired = true;
      snprintf(s_overtime_banner_text, sizeof(s_overtime_banner_text),
               "Reminder\n%s", s_tasks[i].title);
      show_top_banner(s_overtime_banner_text);
      break;
    }
  }

  if (s_due_reminder_min <= 0) {
    return;
  }
  // Global "notify before due" lead - skips tasks that carry their own
  // reminder (handled above).
  int soonest = -1;
  for (int i = 0; i < s_task_count; i++) {
    int d = s_tasks[i].due_min;
    if (!s_tasks[i].done && s_tasks[i].remind_min < 0 && d >= now_min &&
        (soonest < 0 || d < soonest)) {
      soonest = d;
    }
  }
  if (soonest < 0 || soonest - now_min > s_due_reminder_min || soonest == s_due_notified_min) {
    return;
  }
  s_due_notified_min = soonest;
  char at[16];
  format_due_time(soonest, at, sizeof(at)); // "@ 9:41 AM"
  snprintf(s_overtime_banner_text, sizeof(s_overtime_banner_text), "Task due\n%s", at);
  show_top_banner(s_overtime_banner_text);
}
#endif

#if HEAP_BACKED_LISTS
// The list arrays that are static .bss on every other platform. Heap-allocated
// here so they don't count against emery's 64 KB virtual-size ceiling; emery
// has ~90 KB of free app heap, so this ~50 KB block is safe. Never freed - they
// live for the whole app, and PebbleOS reclaims the heap on exit anyway.
static bool alloc_heap_lists(void) {
  s_tasks    = calloc(MAX_TASKS, sizeof(Task));
  s_incoming = calloc(MAX_TASKS, sizeof(Task));
  s_habits   = calloc(MAX_HABITS, sizeof(Habit));
  s_groups   = calloc(MAX_GROUPS, sizeof(TaskGroup));
  return s_tasks && s_incoming && s_habits && s_groups;
}
#endif

static void init(void) {
  // Set the starting status through set_status_code() (not its static
  // initializer - see s_status_code's comment) so this first "Syncing..."
  // stretch goes through the same chokepoint as every later status.
  set_status_code(STATUS_SYNCING);
#if HEAP_BACKED_LISTS
  if (!alloc_heap_lists()) {
    // Can't realistically happen on emery. If it somehow does, there are no
    // list buffers, so skip the inbox handler (nothing to parse into) and show
    // the sync-error screen instead of dereferencing a NULL array.
    str_copy(s_status_msg, "Out of memory", sizeof(s_status_msg));
    set_status_code(STATUS_ERROR);
    s_main_window = window_create();
    window_set_window_handlers(s_main_window, (WindowHandlers) {
      .load = window_load,
      .unload = window_unload,
    });
    window_stack_push(s_main_window, true);
    return;
  }
#endif
  load_tasks();
  load_habits();
  load_tracking();
#ifndef PBL_PLATFORM_APLITE
  load_habit_tracking();
  load_focus();
  // Drop a stale session: focus needs a live local track, and a session
  // already past its end is over (the app was closed when it expired).
  if (s_focus_end_epoch != 0 &&
      (s_tracking_task_id[0] == '\0' || time(NULL) >= s_focus_end_epoch)) {
    s_focus_end_epoch = 0;
    s_focus_on_break = false;
    save_focus();
  }
#endif
#ifdef BREAK_REMINDER
  load_break_state();
#endif
  recompute_groups();

#ifndef PBL_PLATFORM_APLITE
  // Re-pin a resumed tracking session (the pin setting isn't known until the
  // first sync; the section appears then). A stopped-but-in-grace task is NOT
  // re-pinned - the grace period isn't persisted state.
  if (s_tracking_task_id[0] != '\0') {
    str_copy(s_pinned_task_id, s_tracking_task_id, MAX_ID_LEN);
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

#ifndef PBL_PLATFORM_APLITE
  // A focus session that survived an app close (the inactivity timeout, or
  // the user backing out by mistake) comes straight back to its screen, and
  // re-arms the keepalive. This is the "never loses focus" half of the
  // feature that survives the app actually being killed.
  if (focus_active()) {
    push_live_window();
  }
#endif

#if defined(PBL_TOUCH)
  // Apply the default (off); the first sync turns it on if the phone says so.
  apply_touch_nav();
#endif
#ifndef PBL_PLATFORM_APLITE
  tick_timer_service_subscribe(MINUTE_UNIT, minute_tick_handler);
#endif
}

#ifndef PBL_PLATFORM_APLITE
// Launcher App Glance: one line under the app name in the launcher. The
// tracked task wins ("Now: <title>"); otherwise the soonest still-upcoming
// timed task today ("@ 9:41 AM  <title>"), same pick as minute_tick_handler.
// Set from deinit as the app closes - there's no background worker, so it
// reflects state at the last app close. aplite has no App Glance support.
static void glance_reload_cb(AppGlanceReloadSession *session, size_t limit,
                             void *context) {
  if (limit < 1) {
    return;
  }
  char subtitle[96];
  subtitle[0] = '\0';

  if (s_tracking_task_id[0] != '\0') {
    Task *t = find_task_by_id(s_tracking_task_id);
    if (t) {
      snprintf(subtitle, sizeof(subtitle), "Now: %s", t->title);
    }
  }

  if (subtitle[0] == '\0') {
    time_t now = time(NULL);
    struct tm *lt = localtime(&now);
    int now_min = lt->tm_hour * 60 + lt->tm_min;
    int soonest = -1, soonest_idx = -1;
    for (int i = 0; i < s_task_count; i++) {
      int d = s_tasks[i].due_min;
      if (!s_tasks[i].done && d >= now_min && (soonest < 0 || d < soonest)) {
        soonest = d;
        soonest_idx = i;
      }
    }
    if (soonest_idx >= 0) {
      char at[16];
      format_due_time(soonest, at, sizeof(at)); // "@ 9:41 AM"
      snprintf(subtitle, sizeof(subtitle), "%s  %s", at,
               s_tasks[soonest_idx].title);
    }
  }

  if (subtitle[0] == '\0') {
    return; // nothing to show - app_glance_reload already cleared old slices
  }

  // '{' / '}' get parsed as template-string tokens by the launcher and would
  // blank the line; task titles rarely contain them, but strip to be safe.
  for (char *p = subtitle; *p; p++) {
    if (*p == '{' || *p == '}') {
      *p = ' ';
    }
  }

  app_glance_add_slice(session, (AppGlanceSlice) {
    .layout = {
      .icon = APP_GLANCE_SLICE_DEFAULT_ICON,
      .subtitle_template_string = subtitle,
    },
    .expiration_time = APP_GLANCE_SLICE_NO_EXPIRATION,
  });
}
#endif

static void deinit(void) {
#ifndef PBL_PLATFORM_APLITE
  app_glance_reload(glance_reload_cb, NULL);
#endif
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
  if (s_schedule_window) {
    window_destroy(s_schedule_window);
  }
  if (s_upcoming_window) {
    window_destroy(s_upcoming_window);
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
