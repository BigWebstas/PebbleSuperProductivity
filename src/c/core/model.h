// Data model: platform limits, Task/Habit/Group types, and the window/layer/list globals.
// Textually #included by main.c (one translation unit) - not compiled alone.

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
// 28, not 30, to keep basalt/chalk/diorite off the 64 KB virtual-size ceiling
// (each slot is ~336 B of static .bss there). Tasks past this on the today
// list just don't show on the watch - they're still on the phone.
#define MAX_TASKS 28
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
  // Which external tracker issue_key (if any) came from - ISSUE_SRC_* below.
  // Drives a small drawn glyph ahead of the issue_key text; set independently
  // of issue_key so a CalDAV task whose uid was too long for a text badge
  // still gets a marker. 0 (ISSUE_SRC_NONE) draws nothing.
  uint8_t issue_src;
  // PRIO_* below. Drawn as a right-aligned icon on the subtitle line.
  uint8_t priority;
#endif
  bool done;
  bool recurs; // has a repeat config - draws a small ↻ glyph on the row
  // A subtask row (see task-store.js pushTaskAndSubtasks) - the title line
  // indents and draws a small tree-connector glyph ahead of it. All-platform
  // like recurs: cheap enough that aplite doesn't need to lose it, unlike
  // issue_key/tags/notes above. Replaced an earlier phone-baked "»" text
  // prefix, which worked but gambled on the watch font having that glyph.
  bool is_subtask;
#ifndef PBL_PLATFORM_APLITE
#ifdef PBL_PLATFORM_EMERY
  // Set on a parent task and on each of its subtasks by the action menu's
  // "Hide subtasks" row; a subtask with it set is skipped by the today list's
  // row walks (task_row_shown). Carried across a rebuild by id like
  // remind_fired, so it lasts the app-open session but not a relaunch.
  // Emery-only: the other platforms have no code space left for it.
  bool collapsed;
#endif
  // The task's own reminder fired this app-open session (minute_tick_handler).
  // Carried across a list rebuild by id (MSG_TASK_SYNC_END) so a frequent
  // re-push doesn't re-fire it.
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
// task->issue_src categories - a git-host tracker (GitHub/GitLab/Gitea, drawn
// as a small branch glyph), a ticket tracker (Jira/Redmine/OpenProject, a
// ticket glyph), or a calendar (CalDAV, a calendar glyph). See task-store.js
// taskIssueSrc for the phone-side mapping.
#define ISSUE_SRC_NONE 0
#define ISSUE_SRC_GIT 1
#define ISSUE_SRC_TICKET 2
#define ISSUE_SRC_CALENDAR 3
// task->priority - the desktop's own numeric Low/Medium/High encoding.
#define PRIO_NONE 0
#define PRIO_LOW 1
#define PRIO_MEDIUM 2
#define PRIO_HIGH 3
#define PRIO_NUMBERS 4 // flag bit: draw circled numbers (desktop priorityIconPreset)

// One entry per contiguous run of equal Task.project in s_tasks (the phone
// pre-sorts by project when grouping is on). Grouping off = '' for every task =
// one group covering the whole list, same as the old flat list.
typedef struct {
  char name[MAX_PROJECT_LEN];
  // Index (via s_row_map when Later Today is active - see recompute_groups)
  // into s_tasks, and how many logical positions from there belong here.
  int start;
  int count;
#ifndef PBL_PLATFORM_APLITE
  // Copied from the group's first task so the selectable project row can fetch
  // this project's notes. aplite keeps a plain non-selectable header.
  char project_id[MAX_PROJECT_ID_LEN];
  // True only for the synthetic trailing "Later Today" group (see
  // recompute_groups) - it pools tasks from every project, so it isn't a
  // real project: no colour swatch, no per-project notes.
  bool is_later_today;
  // Select on this group's project row toggles this - see menu_select_click.
  // recompute_groups() never touches it (unlike is_later_today above, which
  // it always sets fresh), so a collapsed group stays collapsed across a
  // normal sync as long as group order doesn't change. Lives on the group
  // itself rather than a side array so the two places that already hold a
  // resolved TaskGroup* (menu_draw_row, menu_select_click) can read/flip it
  // without a second GROUP_SECTION_BASE lookup.
  bool user_collapsed;
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
#ifdef PBL_PLATFORM_EMERY
static Layer *s_scroll_pip_layer = NULL; // faint scrollbar overlay on the task list
#endif
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
typedef enum { DICT_ADD_TASK, DICT_NOTE_APPEND, DICT_REFLECT, DICT_SEARCH, DICT_NOTESPAGE_APPEND } DictationTarget;
static DictationTarget s_dictation_target = DICT_ADD_TASK;
// Long-press Up (unschedule) and long-press Down / swipe-left (move to tomorrow)
// each open a 5s cancel window on the selected task: the row's subtitle shows
// "Moving to tomorrow..." / "Un-
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
// Project-group headers wipe in (header_bar_w) the first time task data
// arrives after the app opens, same as the Stats/Upcoming/Notes bars.
static bool s_group_headers_have_data = false;
// The Resync row holds green for SYNC_GREEN_MS on a SYNCING -> OK edge - long
// enough to actually register "it synced". The Add Task row flashes "Added" for
// the briefer ADDTASK_FLASH_MS once a dictated task is sent. Both driven by the
// scroll timer's tick, like the bar. emery only.
#define SYNC_GREEN_MS 3000
#define ADDTASK_FLASH_MS 700
static bool s_sync_check_active = false;
static int s_sync_check_tick = 0;
static bool s_addtask_flash_active = false;
static int s_addtask_flash_tick = 0;
// The Habits nav row pulses gold for LASTHABIT_FLASH_MS after a bump completes
// the day's last unfinished habit - long, since the user is usually still on
// the habits sub-page and only sees it after backing out.
#define LASTHABIT_FLASH_MS 4000
static bool s_lasthabit_flash_active = false;
static int s_lasthabit_flash_tick = 0;
// A habit row pulses green for HABIT_FLASH_MS the moment a bump takes it to its
// goal, or gold for the longer HABIT_MILESTONE_MS when the streak just crossed
// 7 / 30 / 100. Its own short repeating timer (the habits list has no ticker).
#define HABIT_FLASH_MS 720
#define HABIT_MILESTONE_MS 2000
#define HABIT_FLASH_STEP_MS 90
static AppTimer *s_habit_flash_timer = NULL;
static char s_habit_flash_id[MAX_HABIT_ID_LEN] = "";
static int s_habit_flash_tick = 0;
static bool s_habit_flash_gold = false;
// Habit id whose streak crossed a milestone in the batch currently arriving;
// the pop fires on MSG_HABIT_SYNC_END once the row data is in place.
static char s_habit_milestone_id[MAX_HABIT_ID_LEN] = "";
// A non-milestone streak increment (old>=2, new==old+1) rolls the "N streak"
// label up one on that row. Milestone crossings keep the gold pop instead.
#define STREAK_ROLL_MS 200
#define STREAK_ROLL_STEP_MS 33
static AppTimer *s_streak_roll_timer = NULL;
static char s_streak_roll_id[MAX_HABIT_ID_LEN] = "";
static char s_streak_roll_prev[16] = "";
static int s_streak_roll_tick = 0;
// Marquee for an overflowing habit title on the selected row - the habits menu
// isn't wired to the main scroll timer, so it runs its own (shared offset).
static AppTimer *s_habits_marquee_timer = NULL;
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
// Once elapsed reaches an hour, format_duration_ms/live_set_elapsed drop to
// "Xh MMm" and stop showing live seconds - ticking every second past that
// point just wakes the CPU (and repaints) for a value that's already on
// screen. Battery lever for a long tracked session; see
// tracking_tick_interval_ms.
#define TRACKING_TICK_SLOW_MS 30000

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
// The same window also shows read-only voice-search results
// (config.enableSearch, default off, mic): dictate a query ->
// MSG_SEARCH_REQUEST -> MSG_SEARCH_DATA blob (one "\x02" header + "title  ·
// project" lines), rendered by upcoming_content_update_proc.
static bool s_search_enabled = false;
#ifdef PBL_PLATFORM_EMERY
// Calendar month view (config.enableCalendar, default off) - browse-only, own
// window with a month grid; selecting a day opens the browse window in
// BROWSE_CALENDAR_DAY mode (see calendar_open_selected_day), not this shared
// page window. Emery-only - see the window's own comment.
static bool s_calendar_enabled = false;
#endif
static char s_search_query[48] = "";
typedef enum { PAGE_UPCOMING, PAGE_NOTES, PAGE_SEARCH } PageMode;
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
static uint8_t s_tags_enabled = 0; // bit 0 Tags row, bit 1 Views row
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
// s_browse_tasks (below) is malloc'd, not static, and was sized off the same
// "~90KB free app heap" assumption that MAX_TASKS's comment above makes -
// wrong on real hardware (confirmed 2026-09-15: emery's real free heap mid-
// session, with s_browse_projects already resident, is ~10KB; basalt/chalk/
// diorite's is smaller still, ~1.8KB). A full MAX_TASKS-sized allocation
// silently failed malloc there, leaving s_browse_tasks NULL and the
// PROJECT_TASKS_START/END replies dropped forever - the "stuck on Loading"
// bug reported against both Projects and Tags (both share this path via
// browse_descend). Capped per-platform to what's actually been proven to fit;
// a project/tag with more tasks than this just shows the first N - see
// browse_descend's malloc-failure fallback for what happens if even this is
// too much.
#ifdef PBL_PLATFORM_EMERY
#define MAX_BROWSE_TASKS 16
#elif defined(PBL_PLATFORM_APLITE)
#define MAX_BROWSE_TASKS 0 // PROJECTS_BROWSER is compiled out on aplite anyway
#else
#define MAX_BROWSE_TASKS 5
#endif
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
// BROWSE_CALENDAR_DAY is the opposite - level-1-ONLY, entered straight from
// the Calendar month view's day tap (push_browse_window's jump-to path),
// never level 0 - s_browse_project_id carries the "YYYY-MM-DD" date instead
// of a project/tag id. emery-only (Calendar itself is), see browse_descend.
typedef enum { BROWSE_PROJECTS, BROWSE_TAGS, BROWSE_TAG_EDIT, BROWSE_MOVE, BROWSE_CALENDAR_DAY } BrowseMode;
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
// time(NULL) when s_presence_spent_ms was last stamped fresh from the phone -
// the anchor for locally ticking it forward between pushes. NOT the same
// anchor as s_presence_elapsed_base (the session's start): s_presence_spent_ms
// is the phone's task.timeSpent AS OF THIS PUSH, which can already include
// some of the current session's progress (the desktop flushes it mid-session),
// so extrapolating from the session's start double-counts that overlap.
// Extrapolating from the LAST PUSH instead only ever covers the (small) gap
// since the last known-good total, never overlapping with time already baked
// into s_presence_spent_ms.
static time_t s_presence_spent_received_epoch = 0;
static Window *s_live_window = NULL;
#ifdef PBL_PLATFORM_EMERY
// Forward-declared here (defined with the rest of the calendar window,
// further down the file) so touch_handler can reach it below - see the
// right-swipe comment there.
static Window *s_calendar_window = NULL;
#endif
static TextLayer *s_live_state_layer = NULL;
static TextLayer *s_live_task_layer = NULL;
static TextLayer *s_live_elapsed_layer = NULL;
static TextLayer *s_live_hint_layer = NULL;
static StatusBarLayer *s_live_status_bar = NULL;
static AppTimer *s_live_tick_timer = NULL;
#ifdef PBL_PLATFORM_EMERY
// Canvas layer behind the live window's text, alive only while the window is -
// draws the estimate budget bar, see live_arc_update_proc.
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
// Per-tap flags the menu handlers read (declared up here so menu_selection_changed
// / menu_select_click can see them); set in the touch handler far below.
static bool s_tap_moved_sel = false;
static bool s_touch_longpress_fired = false;
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
// overflow rather than dropping tasks, so a list with more distinct projects
// than this still renders (the tail share one header).
#define MAX_GROUPS 14
#if HEAP_BACKED_LISTS
static TaskGroup *s_groups; // calloc'd in alloc_heap_lists()
#else
static TaskGroup s_groups[MAX_GROUPS];
#endif
static int s_group_count = 0;

#ifndef PBL_PLATFORM_APLITE
// "Later Today" (config.laterToday): with tasks grouped by project, one
// scheduled later than right now is pulled out of its project's group and
// pooled with every other later task into one group at the very end, sorted
// by time - mirrors the desktop. Only takes effect when the list actually
// is grouped (some task has a non-empty project - see recompute_groups).
// Recomputed on every regroup (a real sync AND the minute tick), so a task
// rejoins its normal group the instant its time passes, no re-sync needed.
static bool s_later_today_enabled = false;
// "Select on a group header collapses it" (config.collapseGroups, default
// off). Off: Select on a project header opens it in the Projects browser.
// On: Select toggles that group's TaskGroup.user_collapsed instead.
static bool s_collapse_groups = false;
// Maps a *logical* position (what TaskGroup.start/count and every group-
// relative row index describe) to the real index into s_tasks. Identity
// (s_row_map[i] == i) whenever Later Today is off - s_tasks itself is never
// reordered, so this is the only new indirection the rest of the grouping
// code needs. See rebuild_row_map / recompute_groups.
#if HEAP_BACKED_LISTS
static int *s_row_map; // calloc'd in alloc_heap_lists()
#else
static int s_row_map[MAX_TASKS];
#endif
#endif

// Marquee-scrolls the selected task row's title when it's too wide to fit
// (MenuLayer has no scrolling-text cell). Only the selected row scrolls.
#define SCROLL_GAP_PX 24
