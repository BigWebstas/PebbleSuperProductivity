// Live tracking detail window and focus mode.
// Textually #included by main.c (one translation unit) - not compiled alone.

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
  // POMODORO: a break that ran out loops back into the next work session
  // (work -> break -> work -> ... until a long-hold ends it or tracking stops).
  if (ran_out && s_focus_on_break) {
    s_focus_on_break = false;
    if (s_focus_type == FOCUS_POMODORO && s_pomodoro_work_min > 0 &&
        s_tracking_task_id[0] != '\0') {
      s_focus_start_epoch = time(NULL);
      s_focus_end_epoch = time(NULL) + (time_t)s_pomodoro_work_min * 60;
      save_focus();
      vibes_double_pulse();
      show_top_banner("Back to work");
      menu_layer_reload_data(s_menu_layer);
      return;
    }
    s_focus_end_epoch = 0;
    s_focus_start_epoch = 0;
    save_focus();
    vibes_double_pulse();
    show_top_banner("Break over");
    menu_layer_reload_data(s_menu_layer);
    return;
  }
  // POMODORO: a work session that ran out chains into a break.
  if (ran_out && s_focus_type == FOCUS_POMODORO && s_pomodoro_break_min > 0) {
    focus_bump_completed(); // the work session counts for the Stats page
    s_focus_on_break = true;
    s_focus_start_epoch = time(NULL);
    s_focus_end_epoch = time(NULL) + (time_t)s_pomodoro_break_min * 60;
    save_focus();
    vibe_celebrate();
    snprintf(s_overtime_banner_text, sizeof(s_overtime_banner_text),
             "Break: %d min", s_pomodoro_break_min);
    show_top_banner(s_overtime_banner_text);
    menu_layer_reload_data(s_menu_layer);
    return;
  }
  // Plain end - a manual stop (COUNTDOWN / FLOWTIME / an early Pomodoro stop),
  // or a COUNTDOWN reaching zero.
  s_focus_on_break = false;
  s_focus_end_epoch = 0;
  s_focus_start_epoch = 0;
  save_focus();
  if (ran_out) {
    vibe_celebrate();
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
  if (focus_active()) {
    focus_end(false);
  } else {
    if (s_tracking_task_id[0] == '\0') {
      return;
    }
    s_focus_on_break = false;
    s_focus_start_epoch = time(NULL);
    if (s_focus_type == FOCUS_FLOWTIME) {
      s_focus_end_epoch = 0; // counts up, no end
    } else {
      int len = s_focus_type == FOCUS_POMODORO ? s_pomodoro_work_min : s_focus_len_min;
      s_focus_end_epoch = time(NULL) + (time_t)len * 60;
    }
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
    layer_mark_dirty(s_live_arc_layer); // budget bar tracks elapsed time
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
      if (s_focus_end_epoch == 0) {
        // FLOWTIME: count up from the session start.
        int up_s = (int)(time(NULL) - s_focus_start_epoch);
        if (up_s < 0) {
          up_s = 0;
        }
        int h = up_s / 3600;
        if (h > 0) {
          snprintf(elapsed_buf, sizeof(elapsed_buf), "%d:%02d:%02d", h, (up_s % 3600) / 60, up_s % 60);
        } else {
          snprintf(elapsed_buf, sizeof(elapsed_buf), "%d:%02d", up_s / 60, up_s % 60);
        }
        text_layer_set_text(s_live_state_layer, "Flowing");
      } else {
        int left_s = (int)(s_focus_end_epoch - time(NULL));
        if (left_s < 0) {
          left_s = 0;
        }
        snprintf(elapsed_buf, sizeof(elapsed_buf), "%d:%02d", left_s / 60, left_s % 60);
        text_layer_set_text(s_live_state_layer, s_focus_on_break ? "Break" : "Focusing");
      }
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
      // Focus mode's Flowtime/Countdown branches above always show live
      // seconds (even past an hour, unlike the plain "Tracking" branch's
      // live_set_elapsed) - so only the plain branch gets to slow down.
      uint32_t interval = focus_active() ? TRACKING_TICK_INTERVAL_MS : tracking_tick_interval_ms();
      s_live_tick_timer = app_timer_register(interval, live_tick_callback, NULL);
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
    // "spent / estimate" when the remote task has an estimate, ticked forward
    // from s_presence_spent_received_epoch (this push's receipt time) - NOT
    // s_presence_elapsed_base (the session's start), which double-counts (see
    // s_presence_spent_received_epoch's own comment). No estimate: the plain
    // running session clock, anchored to the session's actual start - same
    // number the pinned row's own remote display shows.
    int session_s = s_presence_estimate_ms > 0
        ? (int)(time(NULL) - s_presence_spent_received_epoch)
        : (int)(time(NULL) - s_presence_elapsed_base);
    live_set_elapsed(elapsed_buf, sizeof(elapsed_buf), s_presence_spent_ms, s_presence_estimate_ms,
                     session_s);
    if (!s_live_tick_timer) {
      s_live_tick_timer = app_timer_register(tracking_tick_interval_ms(), live_tick_callback, NULL);
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
static void live_arc_update_proc(Layer *layer, GContext *ctx) {
  // Budget bar: while tracking locally (not in a focus session) and the task
  // carries an estimate, a thin bar shows
  // spent / estimate, green -> amber (>=75%) -> red (over).
  if (!focus_active() && s_tracking_task_id[0] != '\0') {
    Task *bt = find_task_by_id(s_tracking_task_id);
    if (bt && bt->time_estimate_ms > 0) {
      int spent_s = bt->time_spent_ms / 1000;
      time_t el = time(NULL) - s_tracking_start_epoch;
      if (el > 0) {
        spent_s += (int)el;
      }
      int est_s = bt->time_estimate_ms / 1000;
      if (est_s < 1) {
        est_s = 1;
      }
      int pct = spent_s * 100 / est_s;          // int-safe: spent_s well under 2^24
      int fill = pct > 100 ? 100 : pct;
      GRect bb = layer_get_bounds(layer);
      int16_t bw = bb.size.w - 48;
      int16_t bx = bb.origin.x + 24;
      int16_t by = bb.origin.y + bb.size.h * 62 / 100;
      graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorDarkGray, GColorBlack));
      graphics_fill_rect(ctx, GRect(bx, by, bw, 6), 2, GCornersAll);
      graphics_context_set_fill_color(ctx,
          pct > 100 ? GColorRed : pct >= 75 ? GColorChromeYellow : GColorJaegerGreen);
      graphics_fill_rect(ctx, GRect(bx, by, bw * fill / 100, 6), 2, GCornersAll);
    }
  }
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
  // The budget-bar layer sits behind everything (added first).
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
  text_layer_set_text_color(s_live_hint_layer, GColorBlack);

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
