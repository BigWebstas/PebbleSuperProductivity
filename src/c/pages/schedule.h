// Schedule page: today's timed tasks in order.
// Textually #included by main.c (one translation unit) - not compiled alone.

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
// uint8_t: an index into s_tasks, and MAX_TASKS <= 50 < 256.
static uint8_t s_schedule_order[MAX_TASKS];
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
