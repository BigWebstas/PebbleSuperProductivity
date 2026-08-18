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

typedef struct {
  char id[MAX_ID_LEN];
  char title[MAX_TITLE_LEN];
  bool done;
} Task;

static Window *s_main_window;
static MenuLayer *s_menu_layer;
static StatusBarLayer *s_status_bar;
static TextLayer *s_empty_layer;

static Task s_tasks[MAX_TASKS];
static int s_task_count = 0;      // tasks currently shown (committed)
static int s_incoming_total = 0;  // total announced by the current sync batch
static Task s_incoming[MAX_TASKS];
static int s_status_code = STATUS_SYNCING;

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

// ---------- menu layer callbacks ----------

static uint16_t menu_get_num_rows(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  return (uint16_t)s_task_count;
}

static void menu_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context) {
  if (cell_index->row >= (uint16_t)s_task_count) {
    return;
  }
  Task *task = &s_tasks[cell_index->row];
  menu_cell_basic_draw(ctx, cell_layer, task->title, task->done ? "Done" : NULL, NULL);
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
  if (cell_index->row >= (uint16_t)s_task_count) {
    return;
  }
  Task *task = &s_tasks[cell_index->row];
  task->done = !task->done;
  save_tasks();
  menu_layer_reload_data(s_menu_layer);
  send_task_toggle(task);
}

// ---------- empty / status placeholder ----------

static void update_empty_layer(void) {
  bool show_empty = (s_task_count == 0);
  layer_set_hidden(text_layer_get_layer(s_empty_layer), !show_empty);
  layer_set_hidden(menu_layer_get_layer(s_menu_layer), show_empty);

  if (!show_empty) {
    return;
  }

  switch (s_status_code) {
    case STATUS_NOT_PAIRED:
      text_layer_set_text(s_empty_layer, "Open the app on\nyour phone to pair\nwith SuperSync.");
      break;
    case STATUS_ERROR:
      text_layer_set_text(s_empty_layer, "Sync error.\nSelect to retry.");
      break;
    case STATUS_SYNCING:
      text_layer_set_text(s_empty_layer, "Syncing...");
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
      s_incoming[idx].done = done_tuple && done_tuple->value->int32 != 0;
      break;
    }
    case MSG_TASK_SYNC_END: {
      int count = s_incoming_total < MAX_TASKS ? s_incoming_total : MAX_TASKS;
      memcpy(s_tasks, s_incoming, sizeof(Task) * (size_t)count);
      s_task_count = count;
      s_status_code = STATUS_OK;
      save_tasks();
      menu_layer_reload_data(s_menu_layer);
      update_empty_layer();
      break;
    }
    case MSG_SYNC_STATUS: {
      Tuple *status_tuple = dict_find(iterator, KEY_STATUS_CODE);
      if (status_tuple) {
        s_status_code = status_tuple->value->int32;
      }
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
    .get_num_rows = menu_get_num_rows,
    .draw_row = menu_draw_row,
    .select_click = menu_select_click,
  });
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));

  s_empty_layer = text_layer_create(content_bounds);
  text_layer_set_text_alignment(s_empty_layer, GTextAlignmentCenter);
  text_layer_set_font(s_empty_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  layer_add_child(window_layer, text_layer_get_layer(s_empty_layer));

  update_empty_layer();
  request_sync();
}

static void window_unload(Window *window) {
  menu_layer_destroy(s_menu_layer);
  text_layer_destroy(s_empty_layer);
  status_bar_layer_destroy(s_status_bar);
}

static void init(void) {
  load_tasks();

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
