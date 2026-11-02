// End-of-day reflect (retro) menu.
// Textually #included by main.c (one translation unit) - not compiled alone.

// ---------- reflect (before-Finish-Day retro) ----------
// A menu reached with Select on the Finish Day row (config.enableReflect;
// long-Select there still archives too). Each row logs one field of today's
// metric straight away (optimistic, phone does [Metric] Upsert Metric):
//   Energy      - Select cycles Low/OK/Good        -> MSG_METRIC_ENERGY (1-3)
//   Day rating  - Select cycles 1..4 (impactOfWork) -> MSG_METRIC_RATING (1-4)
//   Improvement - Select dictates one thing to improve -> MSG_METRIC_REFLECT
// Values start unset ("-") - the watch never fetches the existing metric.
// A trailing "Finish Day" row archives, same as long-Select on the main row -
// a visible way to commit the day right after logging the retro fields.
// State statics are declared up near s_reflect_enabled.

#define REFLECT_HAS_MIC PBL_IF_MICROPHONE_ELSE(true, false)
#define REFLECT_FINISH_ROW (REFLECT_HAS_MIC ? 3 : 2)

static uint16_t reflect_num_rows(MenuLayer *ml, uint16_t section, void *ctx) {
  return REFLECT_FINISH_ROW + 1;
}

static void reflect_draw_row(GContext *ctx, const Layer *cell, MenuIndex *idx, void *c) {
  if (idx->row == REFLECT_FINISH_ROW) {
    menu_cell_basic_draw(ctx, cell, "Finish Day", NULL, NULL);
    return;
  }
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
  if (idx->row == REFLECT_FINISH_ROW) {
    // Same archive path as long-Select on the main list's Finish Day row.
#ifdef PBL_PLATFORM_EMERY
    send_finish_day();
    s_close_after_finish_day_sent = false;
    push_finishday_window();
#else
    s_close_after_finish_day_sent = true;
    send_finish_day();
#endif
    return;
  }
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
