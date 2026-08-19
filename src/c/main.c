#include <pebble.h>

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
#define KEY_STATUS_CODE MESSAGE_KEY_STATUS_CODE
#define KEY_STATUS_MSG MESSAGE_KEY_STATUS_MSG

// MSG_TYPE values, watch <-> phone.
enum {
  MSG_TASK_SYNC_START = 1, // phone -> watch: TASK_TOTAL follows
  MSG_TASK_ITEM = 2,       // phone -> watch: one task (TASK_INDEX/ID/TITLE/DONE)
  MSG_TASK_SYNC_END = 3,   // phone -> watch: list is complete, redraw
  MSG_SYNC_STATUS = 4,     // phone -> watch: STATUS_CODE (+ optional STATUS_MSG)
  MSG_REQUEST_SYNC = 5,    // watch -> phone: please refresh
  MSG_TASK_TOGGLE = 6,     // watch -> phone: TASK_ID + TASK_DONE (new state)
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
#define MAX_ID_LEN 40
#define MAX_PROJECT_LEN 32

typedef struct {
  char id[MAX_ID_LEN];
  char title[MAX_TITLE_LEN];
  char project[MAX_PROJECT_LEN]; // '' when the phone isn't grouping by project
  bool done;
  int due_min; // minutes since local midnight, or -1 when the task has no dueWithTime
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

static Window *s_main_window;
static MenuLayer *s_menu_layer;
static StatusBarLayer *s_status_bar;
static TextLayer *s_empty_layer;
static BitmapLayer *s_logo_layer;
static GBitmap *s_logo_bitmap;
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

static void request_sync(void);
static void hide_error_overlay(void);

// ---------- menu layer callbacks ----------

// Section 0 is always the "Resync" action (1 row, no header). When there
// are tasks, sections 1..s_group_count are one per project group (see
// recompute_groups()); when the list is empty, section 0 doubles as the
// empty/error screen's retry target and there are no further sections.
static uint16_t menu_get_num_sections(MenuLayer *menu_layer, void *context) {
  return s_task_count > 0 ? (uint16_t)(1 + s_group_count) : 1;
}

static uint16_t menu_get_num_rows(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  if (s_task_count == 0) {
    // The menu layer is hidden whenever s_task_count is 0 (see
    // update_empty_layer), but it still owns the window's click config, so
    // it needs at least one reportable row for SELECT to be dispatched at
    // all - otherwise "Select to retry" on the error screen is dead text.
    // Never actually drawn in that state since the layer is hidden.
    return 1;
  }
  if (section_index == 0) {
    return 1; // Resync row
  }
  int group_idx = (int)section_index - 1;
  if (group_idx >= s_group_count) {
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
    // Reflects live sync status so a resync failure is visible even while
    // the previously-cached list is still showing, instead of being
    // silently swallowed (only the empty/error screen showed status
    // before this row existed).
    static char s_resync_subtitle[MAX_STATUS_MSG_LEN + 16];
    const char *subtitle = "Get latest tasks";
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
      default:
        break;
    }
    // Bypasses menu_cell_basic_draw so this row reads as a standing
    // call-to-action rather than just another list item - background stays
    // red regardless of selection state, so it can't be mistaken for a task.
    // Text itself still inverts to white on select, matching every other
    // row's selection feedback instead of looking inert when highlighted.
    bool is_selected = menu_layer_get_selected_index(s_menu_layer).section == cell_index->section &&
                        menu_layer_get_selected_index(s_menu_layer).row == cell_index->row;
    GRect bounds = layer_get_bounds(cell_layer);
    graphics_context_set_fill_color(ctx, GColorRed);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    graphics_context_set_text_color(ctx, is_selected ? GColorWhite : GColorBlack);
    GRect title_box = GRect(TITLE_BOX_X, TITLE_BOX_Y, bounds.size.w - TITLE_BOX_X * 2, 30);
    graphics_draw_text(ctx, "Resync", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), title_box,
                        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    GRect subtitle_box = GRect(TITLE_BOX_X, bounds.size.h - 18, bounds.size.w - TITLE_BOX_X * 2, 18);
    graphics_draw_text(ctx, subtitle, fonts_get_system_font(FONT_KEY_GOTHIC_14), subtitle_box,
                        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
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
  } else if (task->due_min >= 0) {
    char due_text[16];
    format_due_time(task->due_min, due_text, sizeof(due_text));
    GRect subtitle_box = GRect(TITLE_BOX_X, bounds.size.h - 18, bounds.size.w - TITLE_BOX_X * 2, 18);
    graphics_draw_text(ctx, due_text, fonts_get_system_font(FONT_KEY_GOTHIC_14), subtitle_box,
                        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
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
    // The "Resync" row atop the populated list.
    request_sync();
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

// ---------- empty / status placeholder ----------

static void stop_syncing_animation(void) {
  if (s_syncing_timer) {
    app_timer_cancel(s_syncing_timer);
    s_syncing_timer = NULL;
  }
}

static void syncing_timer_callback(void *data) {
  s_syncing_dots = (s_syncing_dots + 1) % 4;
  static char s_syncing_text[16];
  snprintf(s_syncing_text, sizeof(s_syncing_text), "Syncing%.*s", s_syncing_dots, "...");
  text_layer_set_text(s_empty_layer, s_syncing_text);
  s_syncing_timer = app_timer_register(SYNCING_ANIM_INTERVAL_MS, syncing_timer_callback, NULL);
}

static void start_syncing_animation(void) {
  if (s_syncing_timer) {
    return;
  }
  s_syncing_dots = 0;
  text_layer_set_text(s_empty_layer, "Syncing");
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
}

static void window_unload(Window *window) {
  stop_scroll_timer();
  stop_syncing_animation();
  menu_layer_destroy(s_menu_layer);
  text_layer_destroy(s_empty_layer);
  text_layer_destroy(s_error_layer);
  bitmap_layer_destroy(s_logo_layer);
  gbitmap_destroy(s_logo_bitmap);
  status_bar_layer_destroy(s_status_bar);
}

static void init(void) {
  load_tasks();
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
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
