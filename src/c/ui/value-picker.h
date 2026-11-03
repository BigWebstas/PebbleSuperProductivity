// Shared value picker: estimate, deadline, habit count, time.
// Textually #included by main.c (one translation unit) - not compiled alone.

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

#ifdef PBL_PLATFORM_EMERY
// Up/Down slides the old value out and the new one in - a short vertical roll.
#define PICK_ROLL_MS 130
#define PICK_ROLL_STEP_MS 20
static AppTimer *s_pick_roll_timer = NULL;
static int s_pick_roll_tick = 0;
static int s_pick_roll_dir = 0;      // +1 value went up, -1 down, 0 idle
static char s_pick_roll_prev[16] = "";

static void pick_roll_tick(void *data) {
  s_pick_roll_timer = NULL;
  s_pick_roll_tick++;
  if (s_pick_roll_tick * PICK_ROLL_STEP_MS < PICK_ROLL_MS) {
    s_pick_roll_timer = app_timer_register(PICK_ROLL_STEP_MS, pick_roll_tick, NULL);
  } else {
    s_pick_roll_dir = 0;
  }
  if (s_pick_layer) {
    layer_mark_dirty(s_pick_layer);
  }
}

static void stop_pick_roll(void) {
  if (s_pick_roll_timer) {
    app_timer_cancel(s_pick_roll_timer);
    s_pick_roll_timer = NULL;
  }
  s_pick_roll_dir = 0;
}
#endif

static void pick_layer_update_proc(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  fill_bg(ctx, b, GColorWhite);
  graphics_context_set_text_color(ctx, GColorBlack);
  int16_t cy = b.size.h / 2;
  int16_t vy = cy - 22; // value box top
  char val[16];
  pick_format(val, sizeof(val));
#ifdef PBL_PLATFORM_EMERY
  if (s_pick_roll_dir != 0) {
    int p = s_pick_roll_tick * PICK_ROLL_STEP_MS * 1000 / PICK_ROLL_MS;
    if (p > 1000) {
      p = 1000;
    }
    int travel = 22;
    int new_off = s_pick_roll_dir * travel * (1000 - p) / 1000;   // comes to rest
    int old_off = -s_pick_roll_dir * travel * p / 1000;           // leaves
    draw_text(ctx, s_pick_roll_prev, FONT_KEY_GOTHIC_28_BOLD,
              GRect(0, vy + old_off, b.size.w, 34), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter);
    draw_text(ctx, val, FONT_KEY_GOTHIC_28_BOLD,
              GRect(0, vy + new_off, b.size.w, 34), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter);
    // Mask whatever the slide poked past the value band, then redraw the chrome.
    fill_bg(ctx, GRect(0, 0, b.size.w, vy), GColorWhite);
    fill_bg(ctx, GRect(0, cy + 12, b.size.w, b.size.h - (cy + 12)), GColorWhite);
  } else
#endif
  {
    draw_text(ctx, val, FONT_KEY_GOTHIC_28_BOLD,
              GRect(0, vy, b.size.w, 34), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter);
  }
  draw_text(ctx, s_pick_kind == PICK_ESTIMATE ? "Estimate"
                 : s_pick_kind == PICK_DEADLINE ? "Deadline"
                 : s_pick_kind == PICK_TIME ? "Schedule Time" : "Count",
            CHROME_FONT_KEY,
            GRect(0, cy - 44, b.size.w, 20), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter);
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
#ifdef PBL_PLATFORM_EMERY
    pick_format(s_pick_roll_prev, sizeof(s_pick_roll_prev)); // the value we're leaving
    s_pick_idx = n;
    s_pick_roll_dir = delta > 0 ? 1 : -1;
    s_pick_roll_tick = 0;
    if (!s_pick_roll_timer) {
      s_pick_roll_timer = app_timer_register(PICK_ROLL_STEP_MS, pick_roll_tick, NULL);
    }
#else
    s_pick_idx = n;
#endif
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
          begin_habit_flash(h->id, false);
          maybe_flash_last_habit();
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
#ifdef PBL_PLATFORM_EMERY
  stop_pick_roll(); // no stale roll from a previous open
#endif
}

static void pick_window_unload(Window *window) {
#ifdef PBL_PLATFORM_EMERY
  stop_pick_roll();
#endif
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
