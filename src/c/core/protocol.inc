// Phone <-> watch protocol: version, AppMessage keys, MSG_* ids, status codes.
// Textually #included by main.c (one translation unit) - not compiled alone.

// Keep in sync by hand with package.json "version" on every bump - no runtime
// API exposes it to C.
#define APP_VERSION "0.6.56"

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
#define KEY_TASK_ISSUE_SRC MESSAGE_KEY_TASK_ISSUE_SRC
#define KEY_TASK_IS_SUBTASK MESSAGE_KEY_TASK_IS_SUBTASK
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
#define KEY_TASK_PRIORITY MESSAGE_KEY_TASK_PRIORITY
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
#define KEY_SYNC_INTERVAL_MIN MESSAGE_KEY_SYNC_INTERVAL_MIN
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
#define KEY_LATER_TODAY_ENABLED MESSAGE_KEY_LATER_TODAY_ENABLED
#define KEY_COLLAPSE_GROUPS MESSAGE_KEY_COLLAPSE_GROUPS
#define KEY_UPCOMING_TEXT MESSAGE_KEY_UPCOMING_TEXT
#define KEY_CALENDAR_ENABLED MESSAGE_KEY_CALENDAR_ENABLED
#define KEY_CAL_MONTH_OFFSET MESSAGE_KEY_CAL_MONTH_OFFSET
#define KEY_CAL_MONTH_MASK MESSAGE_KEY_CAL_MONTH_MASK
#define KEY_CALDAY_DATE MESSAGE_KEY_CALDAY_DATE
#define KEY_NOTESPAGE_ENABLED MESSAGE_KEY_NOTESPAGE_ENABLED
#define KEY_NOTESPAGE_TEXT MESSAGE_KEY_NOTESPAGE_TEXT
#define KEY_SEARCH_ENABLED MESSAGE_KEY_SEARCH_ENABLED
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
  // Optional voice search (config.enableSearch, non-aplite, mic only). The
  // watch dictates a query; the phone searches every task and replies with a
  // read-only "\x02"-header result blob, reusing UPCOMING_TEXT + the shared
  // page window (PAGE_SEARCH).
  MSG_SEARCH_REQUEST = 61,          // watch -> phone: TASK_TITLE (the dictated query)
  MSG_SEARCH_DATA = 62,             // phone -> watch: UPCOMING_TEXT
  // Watch-dictated append to the Notes page's pinned-to-today notes - long-Select
  // on the page, same "append to the canonical note, or create one" shape as
  // MSG_NOTE_APPEND/MSG_PROJECT_NOTE_APPEND (see firstPinnedNote in index.js).
  MSG_NOTESPAGE_APPEND = 63,        // watch -> phone: NOTE_TEXT (dictated text)
  // Calendar month view (browse-only) - a month grid on its own window, plus a
  // single day's tasks reusing the browse window (BROWSE_CALENDAR_DAY, see
  // browse_descend). The watch computes its own month/weekday grid from its
  // clock; the phone is only asked which days in the shown month have a due
  // task. The day's task list rides MSG_CALDAY_REQUEST -> the same
  // MSG_PROJECT_TASKS_START/ITEM/END triplet a project/tag task list uses
  // (see handleCalDayRequest in index.js) - no dedicated reply message needed.
  MSG_CAL_MONTH_REQUEST = 64, // watch -> phone: CAL_MONTH_OFFSET (months from the real current month)
  MSG_CAL_MONTH_DATA = 65,    // phone -> watch: CAL_MONTH_OFFSET (echoed - see handle_calendar_month_data) + CAL_MONTH_MASK
  MSG_CALDAY_REQUEST = 66,    // watch -> phone: CALDAY_DATE ("YYYY-MM-DD")
  // The action menu's "Delete" row - a second press past the armed "Confirm
  // delete?" label (s_delete_armed) before this ever sends; no undo once it
  // does. See action_select's ACT_DELETE case.
  MSG_TASK_DELETE = 67,       // watch -> phone: TASK_ID
  // The action menu's "Priority" row: Select cycles none -> high -> medium ->
  // low in place. TASK_PRIORITY is the PRIO_* code (0-3).
  MSG_TASK_SET_PRIORITY = 68, // watch -> phone: TASK_ID + TASK_PRIORITY
};
