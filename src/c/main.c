#include <pebble.h>

// No runtime API exposes the app's own versionLabel (package.json's
// "version") to C code - keep this in sync by hand on every version bump.
#define APP_VERSION "0.6.8"

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
};

// STATUS_CODE values sent from the phone.
enum {
  STATUS_OK = 0,
  STATUS_SYNCING = 1,
  STATUS_NOT_PAIRED = 2,
  STATUS_ERROR = 3,
};

#define MAX_TASKS 30
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

typedef struct {
  char id[MAX_ID_LEN];
  char title[MAX_TITLE_LEN];
  char project[MAX_PROJECT_LEN]; // '' when the phone isn't grouping by project
  bool done;
  int due_min;       // minutes since local midnight, or -1 when the task has no dueWithTime
  int time_spent_ms; // total tracked time (all days, all devices), 0 if none
  int time_estimate_ms; // 0 if none
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
// counters (value in ms of tracked time, not a "did you do this today"
// count - nothing to increment/decrement) are filtered out entirely before
// ever reaching the watch (getActiveHabits in task-store.js), so every
// Habit here is always manipulable.
// Kept low (unlike MAX_TASKS' 30) because aplite's ~24KB RAM budget is
// already tight after MAX_ID_LEN's own 96-byte bump for calendar tasks.
// aplite specifically, not a shared cap - basalt/chalk/diorite still have
// ~41KB free and emery ~106KB, so holding every platform back to aplite's
// number here would be needlessly conservative for the other four.
// Confirmed via pebble build's own per-platform memory report: 8 overflowed
// aplite's linked binary by 280 bytes even before the row-icon resources
// added below; those pushed the workable aplite number down further to 3.
#ifdef PBL_PLATFORM_APLITE
#define MAX_HABITS 3
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
  bool done;
  int value; // today's count
  int goal;  // streakMinValue-derived target used for the "value/goal" subtitle
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

static Task s_tasks[MAX_TASKS];
static int s_task_count = 0;      // tasks currently shown (committed)
static int s_incoming_total = 0;  // total announced by the current sync batch
static Task s_incoming[MAX_TASKS];
static int s_status_code = STATUS_SYNCING;
#define MAX_STATUS_MSG_LEN 64
static char s_status_msg[MAX_STATUS_MSG_LEN] = "";

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

static void request_sync(void);
static void hide_error_overlay(void);
static void push_habits_window(void);
static void update_habits_empty_layer(void);
#ifndef PBL_PLATFORM_APLITE
static void start_add_task_dictation(void);
#endif

// ---------- menu layer callbacks ----------

// Section 0 is always the "Resync" + "Habits" actions, plus "Add Task" on
// mic-equipped platforms (2 or 3 rows, no header) - PBL_IF_MICROPHONE_ELSE
// resolves per-platform (aplite has no mic hardware), so aplite never even
// reports the third row, let alone draws or routes clicks to it.
// When there are tasks, sections 1..s_group_count are one per project group
// (see recompute_groups()), followed by one final section (group_idx ==
// s_group_count) holding a single static, non-interactive row that shows
// the app version - always the very last row in the list. When the list is
// empty, section 0 doubles as the empty/error screen's retry target and
// there are no further sections (no version footer on that screen either).
static uint16_t menu_get_num_sections(MenuLayer *menu_layer, void *context) {
  return s_task_count > 0 ? (uint16_t)(1 + s_group_count + 1) : 1;
}

static uint16_t menu_get_num_rows(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  if (s_task_count == 0) {
    // The menu layer is hidden whenever s_task_count is 0 (see
    // update_empty_layer), but it still owns the window's click config, so
    // it needs at least one reportable row for SELECT to be dispatched at
    // all - otherwise "Select to retry" on the error screen is dead text.
    // Never actually drawn in that state since the layer is hidden. This
    // also means the Habits row (see below) isn't reachable while the list
    // is empty/erroring - a deliberately narrow scope, not a full port of
    // this screen's navigation.
    return 1;
  }
  if (section_index == 0) {
    return PBL_IF_MICROPHONE_ELSE(3, 2); // Resync, Habits, (Add Task)
  }
  int group_idx = (int)section_index - 1;
  if (group_idx == s_group_count) {
    return 1; // version-footer row
  }
  if (group_idx > s_group_count) {
    return 0;
  }
  return (uint16_t)s_groups[group_idx].count;
}

static int16_t menu_get_header_height(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  if (s_task_count == 0 || section_index == 0) {
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
  if (s_task_count == 0 || section_index == 0) {
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
// (not a Unicode glyph - see the subtask marker's own comment on why this
// codebase avoids those) so a live-ticking number reads unambiguously as
// "currently running" versus a static previously-accumulated total.
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
}

static void menu_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context) {
  if (s_task_count == 0) {
    return;
  }
  if (cell_index->section == 0) {
    bool is_selected = menu_layer_get_selected_index(s_menu_layer).section == cell_index->section &&
                        menu_layer_get_selected_index(s_menu_layer).row == cell_index->row;
    GRect bounds = layer_get_bounds(cell_layer);

    if (cell_index->row == 1) {
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
    if (cell_index->row == 2) {
      // Only reachable on mic-equipped platforms - menu_get_num_rows never
      // reports this row on aplite, so this branch is simply never drawn
      // there (and, per the #ifndef, not even compiled in on that
      // platform - see s_mic_bitmap's own comment). Starts dictation via
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
    // Static version-footer row, always last - ignores is_selected (never
    // inverts like a real row would) and menu_select_click/long_click
    // already no-op here since resolve_task_at returns NULL for a
    // group_idx past s_group_count, so this never behaves like a task row.
    GRect bounds = layer_get_bounds(cell_layer);
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    graphics_context_set_text_color(ctx, GColorBlack);
    graphics_draw_text(ctx, "v" APP_VERSION, fonts_get_system_font(FONT_KEY_GOTHIC_14), bounds,
                        GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
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
    GRect subtitle_box = GRect(TITLE_BOX_X, bounds.size.h - 18, bounds.size.w - TITLE_BOX_X * 2, 18);
    graphics_draw_text(ctx, "Done", fonts_get_system_font(FONT_KEY_GOTHIC_14), subtitle_box,
                        GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  } else {
    // Tracked time is shown BEHIND (after) the due time when the task has
    // one, both sharing the single subtitle line - not its own row, since
    // every other subtitle here already lives in that same 18px strip.
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
      GRect subtitle_box = GRect(TITLE_BOX_X, bounds.size.h - 18, bounds.size.w - TITLE_BOX_X * 2, 18);
      graphics_draw_text(ctx, subtitle, fonts_get_system_font(FONT_KEY_GOTHIC_14), subtitle_box,
                          GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    }
  }
}

static void send_task_toggle(Task *task) {
  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) != APP_MSG_OK) {
    return;
  }
  dict_write_int32(iter, KEY_MSG_TYPE, MSG_TASK_TOGGLE);
  dict_write_cstring(iter, KEY_TASK_ID, task->id);
  dict_write_int32(iter, KEY_TASK_DONE, task->done ? 1 : 0);
  app_message_outbox_send();
}

static void send_track_time_stop(const char *task_id, int32_t tracked_ms) {
  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) != APP_MSG_OK) {
    return;
  }
  dict_write_int32(iter, KEY_MSG_TYPE, MSG_TRACK_TIME_STOP);
  dict_write_cstring(iter, KEY_TASK_ID, task_id);
  dict_write_int32(iter, KEY_TRACKED_MS, tracked_ms);
  app_message_outbox_send();
}

#ifndef PBL_PLATFORM_APLITE
static void send_task_add(const char *title) {
  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) != APP_MSG_OK) {
    return;
  }
  dict_write_int32(iter, KEY_MSG_TYPE, MSG_TASK_ADD);
  dict_write_cstring(iter, KEY_TASK_TITLE, title);
  app_message_outbox_send();
}

// Fires when dictation finishes (success, cancel, or failure). Only ever
// registered on mic-equipped platforms - compiled out entirely on aplite
// (see s_mic_bitmap's own comment for why).
static void dictation_status_callback(DictationSession *session, DictationSessionStatus status,
                                       char *transcription, void *context) {
  s_dictation_pending = false;
  if (status == DictationSessionStatusSuccess) {
    send_task_add(transcription);
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
  if (s_error_overlay_active) {
    // Click routing goes through MenuLayer's own config regardless of
    // whether its layer is currently hidden (same mechanism the empty-
    // screen's phantom row 0 below already relies on), so this fires even
    // though s_menu_layer is hidden behind the overlay right now.
    hide_error_overlay();
    return;
  }
  if (s_task_count == 0) {
    // The empty/error screen's phantom row 0 (see menu_get_num_rows) lands
    // here - this is what makes "Select to retry" actually retry.
    request_sync();
    return;
  }
  if (cell_index->section == 0) {
    if (cell_index->row == 1) {
      push_habits_window();
#ifndef PBL_PLATFORM_APLITE
    } else if (cell_index->row == 2) {
      // Only reachable on mic-equipped platforms (see menu_get_num_rows).
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
  task->done = !task->done;
  save_tasks();
  menu_layer_reload_data(s_menu_layer);
  send_task_toggle(task);
}

// Long-select toggles time tracking on the highlighted task: starts it if
// nothing (or a different task) is being tracked, stops it if this task is
// the one already being tracked. Only one task tracks at a time, so
// starting a new one first stops-and-reports whatever was running before -
// mirrors the real app's single global "current task", not a per-task
// independent timer each.
static void menu_select_long_click(MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  if (s_error_overlay_active || s_task_count == 0 || cell_index->section == 0) {
    return;
  }
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
}

static void syncing_timer_callback(void *data) {
  s_syncing_dots = (s_syncing_dots + 1) % 4;
  static char s_syncing_text[64];
  // A first sync can genuinely take a few minutes on a large/old account
  // (replaying its whole op-log history) with no way to give an accurate
  // percentage worth trusting - a plain heads-up reads better than a
  // number that's either absent for most of the wait or jumps around.
  snprintf(s_syncing_text, sizeof(s_syncing_text), "Syncing%.*s\n\nThis may take a few minutes", s_syncing_dots, "...");
  text_layer_set_text(s_empty_layer, s_syncing_text);
  s_syncing_timer = app_timer_register(SYNCING_ANIM_INTERVAL_MS, syncing_timer_callback, NULL);
}

static void start_syncing_animation(void) {
  if (s_syncing_timer) {
    return;
  }
  s_syncing_dots = 0;
  text_layer_set_text(s_empty_layer, "Syncing\n\nThis may take a few minutes");
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
  if (s_error_overlay_active) {
    return;
  }
  bool show_empty = (s_task_count == 0);
  layer_set_hidden(text_layer_get_layer(s_empty_layer), !show_empty);
  layer_set_hidden(bitmap_layer_get_layer(s_logo_layer), !show_empty);
  layer_set_hidden(menu_layer_get_layer(s_menu_layer), show_empty);

  // Only the empty screen (no cached list yet at all - i.e. the very first
  // sync) gets the animation; a resync with a populated list already has
  // its own live status via the Resync row's subtitle.
  bool is_initial_syncing = show_empty && s_status_code == STATUS_SYNCING;
  if (is_initial_syncing) {
    start_syncing_animation();
  } else {
    stop_syncing_animation();
  }

  if (!show_empty || is_initial_syncing) {
    return;
  }

  static char s_empty_text[MAX_STATUS_MSG_LEN + 32];

  switch (s_status_code) {
    case STATUS_NOT_PAIRED:
      text_layer_set_text(s_empty_layer, "Open the app on\nyour phone to pair\nwith SuperSync.");
      break;
    case STATUS_ERROR:
      if (s_status_msg[0] != '\0') {
        snprintf(s_empty_text, sizeof(s_empty_text), "Sync error:\n%s\nSelect to retry.", s_status_msg);
        text_layer_set_text(s_empty_layer, s_empty_text);
      } else {
        text_layer_set_text(s_empty_layer, "Sync error.\nSelect to retry.");
      }
      break;
    default:
      text_layer_set_text(s_empty_layer, "No tasks for today.");
      break;
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

// ---------- AppMessage ----------

static void request_sync(void) {
  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) != APP_MSG_OK) {
    return;
  }
  dict_write_int32(iter, KEY_MSG_TYPE, MSG_REQUEST_SYNC);
  app_message_outbox_send();
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
      break;
    }
    case MSG_TASK_SYNC_END: {
      int count = s_incoming_total < MAX_TASKS ? s_incoming_total : MAX_TASKS;
      memcpy(s_tasks, s_incoming, sizeof(Task) * (size_t)count);
      s_task_count = count;
      s_status_code = STATUS_OK;
      recompute_groups();
      save_tasks();
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
      break;
    }
    default:
      break;
  }
}

static void inbox_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_WARNING, "AppMessage dropped: %d", (int)reason);
}

static void outbox_failed_handler(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_WARNING, "AppMessage send failed: %d", (int)reason);
  // Previously silent (log-only): a watch->phone send (task toggle, track-
  // time-stop, resync request) that fails here - e.g. the phone app is
  // backgrounded or Bluetooth is momentarily busy - used to vanish with zero
  // on-screen feedback, indistinguishable from "worked fine". Routing it
  // through the same fullscreen overlay as a phone->watch sync error makes
  // every send failure visible instead of just this one class of them.
  s_status_code = STATUS_ERROR;
  strncpy(s_status_msg, "Couldn't reach phone app", MAX_STATUS_MSG_LEN - 1);
  s_status_msg[MAX_STATUS_MSG_LEN - 1] = '\0';
  show_error_overlay();
}

// ---------- habits window ----------

static Habit *resolve_habit_at(MenuIndex index) {
  if (index.section != 0 || (int)index.row >= s_habit_count) {
    return NULL;
  }
  return &s_habits[index.row];
}

static void send_habit_adjust(Habit *habit, int32_t delta) {
  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) != APP_MSG_OK) {
    return;
  }
  dict_write_int32(iter, KEY_MSG_TYPE, MSG_HABIT_ADJUST);
  dict_write_cstring(iter, KEY_HABIT_ID, habit->id);
  dict_write_int32(iter, KEY_HABIT_DELTA, delta);
  app_message_outbox_send();
}

static uint16_t habits_menu_get_num_sections(MenuLayer *menu_layer, void *context) {
  return 1;
}

static uint16_t habits_menu_get_num_rows(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  return (uint16_t)s_habit_count;
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
  GColor fg = is_selected ? GColorWhite : GColorBlack;
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
  // tell you 3/3 from 7/3. StopWatch-type counters (no simple increment/
  // decrement state - see the Habit struct's own comment) are filtered out
  // entirely before ever reaching the watch (getActiveHabits in
  // task-store.js), so every row here is manipulable.
  char subtitle[32];
  if (habit->done) {
    snprintf(subtitle, sizeof(subtitle), "%d/%d - Done", habit->value, habit->goal);
  } else {
    snprintf(subtitle, sizeof(subtitle), "%d/%d", habit->value, habit->goal);
  }
  GRect subtitle_box = GRect(TITLE_BOX_X, bounds.size.h - 18, bounds.size.w - TITLE_BOX_X * 2, 18);
  graphics_draw_text(ctx, subtitle, fonts_get_system_font(FONT_KEY_GOTHIC_14), subtitle_box,
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
  adjust_habit(*cell_index, 1);
}

static void habits_menu_select_long_click(MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  adjust_habit(*cell_index, -1);
}

static void update_habits_empty_layer(void) {
  bool show_empty = (s_habit_count == 0);
  layer_set_hidden(text_layer_get_layer(s_habits_empty_layer), !show_empty);
  layer_set_hidden(menu_layer_get_layer(s_habits_menu_layer), show_empty);
}

static void habits_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

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
  });
  menu_layer_set_click_config_onto_window(s_habits_menu_layer, window);
  layer_add_child(window_layer, menu_layer_get_layer(s_habits_menu_layer));

  s_habits_empty_layer = text_layer_create(content_bounds);
  text_layer_set_text_alignment(s_habits_empty_layer, GTextAlignmentCenter);
  text_layer_set_font(s_habits_empty_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text(s_habits_empty_layer, "No habits synced.");
  layer_add_child(window_layer, text_layer_get_layer(s_habits_empty_layer));

  update_habits_empty_layer();
}

static void habits_window_unload(Window *window) {
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

// ---------- window lifecycle ----------

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

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
  text_layer_destroy(s_error_layer);
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
  load_tasks();
  load_habits();
  load_tracking();
  recompute_groups();

  app_message_register_inbox_received(inbox_received_handler);
  app_message_register_inbox_dropped(inbox_dropped_handler);
  app_message_register_outbox_failed(outbox_failed_handler);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());

  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_main_window, true);
}

static void deinit(void) {
  window_destroy(s_main_window);
  if (s_habits_window) {
    window_destroy(s_habits_window);
  }
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
