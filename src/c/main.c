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

  graphics_context_set_text_color(ctx, GColorBlue);
  graphics_draw_text(ctx, name, bold_font, text_rect,
                      GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  // Underline sized to the actual rendered text, directly beneath it.
  GSize text_size = graphics_text_layout_get_content_size(
      name, bold_font, text_rect, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
  int16_t underline_y = 2 + text_size.h;
  graphics_context_set_stroke_color(ctx, GColorBlue);
  graphics_draw_line(ctx, GPoint(6, underline_y), GPoint(6 + text_size.w, underline_y));

  // A second, full-width divider along the bottom of the header cell -
  // separates this group from its tasks more clearly than the text-width
  // underline alone, especially once several groups are on screen at once.
  int16_t divider_y = bounds.size.h - 2;
  graphics_draw_line(ctx, GPoint(0, divider_y), GPoint(bounds.size.w, divider_y));
}

// Resolves the row MenuLayer currently has highlighted to a Task, or NULL
// if the selection isn't on a task row at all (the Resync row, or nothing
// selectable yet).
static Task *resolve_selected_task(void) {
  if (s_task_count == 0) {
    return NULL;
  }
  MenuIndex sel = menu_layer_get_selected_index(s_menu_layer);
  if (sel.section == 0) {
    return NULL;
  }
  int group_idx = (int)sel.section - 1;
  if (group_idx >= s_group_count) {
    return NULL;
  }
  int task_idx = s_groups[group_idx].start + (int)sel.row;
  if (task_idx < 0 || task_idx >= s_task_count) {
    return NULL;
  }
  return &s_tasks[task_idx];
}

static int16_t title_natural_width(const char *title) {
  GSize size = graphics_text_layout_get_content_size(
      title, fonts_get_system_font(TITLE_FONT_KEY), GRect(0, 0, 2000, 100),
      GTextOverflowModeFill, GTextAlignmentLeft);
  return size.w;
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
    GRect bounds = layer_get_bounds(cell_layer);
    graphics_context_set_fill_color(ctx, GColorRed);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    graphics_context_set_text_color(ctx, GColorBlack);
    GRect title_box = GRect(TITLE_BOX_X, TITLE_BOX_Y, bounds.size.w - TITLE_BOX_X * 2, 22);
    graphics_draw_text(ctx, "Resync", fonts_get_system_font(TITLE_FONT_KEY), title_box,
                        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    GRect subtitle_box = GRect(TITLE_BOX_X, bounds.size.h - 18, bounds.size.w - TITLE_BOX_X * 2, 18);
    graphics_draw_text(ctx, subtitle, fonts_get_system_font(FONT_KEY_GOTHIC_14), subtitle_box,
                        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    return;
  }
  int group_idx = (int)cell_index->section - 1;
  if (group_idx >= s_group_count) {
    return;
  }
  int task_idx = s_groups[group_idx].start + (int)cell_index->row;
  if (task_idx < 0 || task_idx >= s_task_count) {
    return;
  }
  Task *task = &s_tasks[task_idx];

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
  graphics_context_set_fill_color(ctx, bg);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  graphics_context_set_text_color(ctx, fg);

  GFont title_font = fonts_get_system_font(TITLE_FONT_KEY);
  GRect title_box = GRect(TITLE_BOX_X, TITLE_BOX_Y, bounds.size.w - TITLE_BOX_X * 2, bounds.size.h - TITLE_BOX_Y);

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
  int group_idx = (int)cell_index->section - 1;
  if (group_idx >= s_group_count) {
    return;
  }
  int task_idx = s_groups[group_idx].start + (int)cell_index->row;
  if (task_idx < 0 || task_idx >= s_task_count) {
    return;
  }
  Task *task = &s_tasks[task_idx];
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

static void update_empty_layer(void) {
  bool show_empty = (s_task_count == 0);
  layer_set_hidden(text_layer_get_layer(s_empty_layer), !show_empty);
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
      // refreshes the Resync row's status subtitle in that case.
      menu_layer_reload_data(s_menu_layer);
      update_empty_layer();
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

  s_empty_layer = text_layer_create(content_bounds);
  text_layer_set_text_alignment(s_empty_layer, GTextAlignmentCenter);
  text_layer_set_font(s_empty_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_layer, text_layer_get_layer(s_empty_layer));

  update_empty_layer();
  request_sync();
}

static void window_unload(Window *window) {
  stop_scroll_timer();
  stop_syncing_animation();
  menu_layer_destroy(s_menu_layer);
  text_layer_destroy(s_empty_layer);
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
