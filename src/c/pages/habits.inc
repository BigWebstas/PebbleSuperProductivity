// Habits window: counters, streaks, timers, and the title marquee.
// Textually #included by main.c (one translation unit) - not compiled alone.

// ---------- habits window ----------

static Habit *resolve_habit_at(MenuIndex index) {
  if (index.section != 0) {
    return NULL;
  }
#ifdef PBL_PLATFORM_APLITE
  // Timer-type habits are skipped entirely on aplite, so row indices walk only
  // the plain-count subset (is_stopwatch alone is enough - is_countdown is
  // never true there).
  int visible_row = 0;
  for (int i = 0; i < s_habit_count; i++) {
    if (s_habits[i].is_stopwatch) {
      continue;
    }
    if (visible_row == (int)index.row) {
      return &s_habits[i];
    }
    visible_row++;
  }
  return NULL;
#else
  if ((int)index.row >= s_habit_count) {
    return NULL;
  }
  return &s_habits[index.row];
#endif
}

static void send_habit_adjust(Habit *habit, int32_t delta) {
#ifdef PBL_PLATFORM_APLITE
  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) != APP_MSG_OK) {
    return;
  }
  dict_write_int32(iter, KEY_MSG_TYPE, MSG_HABIT_ADJUST);
  dict_write_cstring(iter, KEY_HABIT_ID, habit->id);
  dict_write_int32(iter, KEY_HABIT_DELTA, delta);
  app_message_outbox_send();
#else
  begin_send(MSG_HABIT_ADJUST, habit->id, NULL, delta);
#endif
}

// Everything through stop_habit_tracking_and_report is the StopWatch/countdown
// habit timer - aplite-excluded. habits_menu_draw_row/select_long_click have
// narrower guards so a StopWatch still shows its progress read-only there.
#ifndef PBL_PLATFORM_APLITE
static void send_habit_track_stop(const char *habit_id, int32_t tracked_ms) {
  begin_send(MSG_HABIT_TRACK_STOP, habit_id, NULL, tracked_ms);
}

static void stop_habit_tracking_tick(void) {
  if (s_habit_tracking_tick_timer) {
    app_timer_cancel(s_habit_tracking_tick_timer);
    s_habit_tracking_tick_timer = NULL;
  }
}

// A "di-di-daah" for a genuine win - streak milestone, day's last habit, a focus
// session run to completion. Distinct from the plain pulses that just mean
// "registered" or "heads up". On a speaker watch, a rising four-note figure too
// when the user has audible notifications on.
static void vibe_celebrate(void) {
  static const uint32_t segs[] = { 70, 90, 70, 90, 260 };
  VibePattern pat = { .durations = segs, .num_segments = ARRAY_LENGTH(segs) };
  vibes_enqueue_custom_pattern(pat);
#ifdef PBL_SPEAKER
  if (s_audible_notify && !speaker_is_muted()) {
    static const SpeakerNote win[] = {
      { .midi_note = 72, .waveform = SpeakerWaveformSine, .duration_ms = 80, .velocity = 0 },
      { .midi_note = 76, .waveform = SpeakerWaveformSine, .duration_ms = 80, .velocity = 0 },
      { .midi_note = 79, .waveform = SpeakerWaveformSine, .duration_ms = 80, .velocity = 0 },
      { .midi_note = 84, .waveform = SpeakerWaveformSine, .duration_ms = 180, .velocity = 0 },
    };
    speaker_play_notes(win, ARRAY_LENGTH(win),
                       s_audible_volume < 0 ? 0 : (s_audible_volume > 100 ? 100 : s_audible_volume));
  }
#endif
}

#ifdef PBL_PLATFORM_EMERY
// A brief full-screen "day complete" moment on Finish Day: a big checkmark
// strokes itself in, a celebratory buzz + chime, then - after FINISHDAY_HOLD_MS
// or a button press - the app closes, the same close Finish Day always did,
// just with a beat first. The archive send already went out before this pushed.
#define FINISHDAY_DRAW_MS 440
#define FINISHDAY_STEP_MS 30
#define FINISHDAY_HOLD_MS 1900
static Window *s_finishday_window = NULL;
static Layer *s_finishday_layer = NULL;
static AppTimer *s_finishday_anim_timer = NULL;
static AppTimer *s_finishday_hold_timer = NULL;
static int s_finishday_tick = 0;

static void finishday_dismiss(void) {
  // Leave a send-failure error overlay standing; otherwise close the app.
  if (s_error_overlay_active) {
    if (s_finishday_window) {
      window_stack_pop(true);
    }
  } else {
    window_stack_pop_all(true);
  }
}

static void finishday_hold_cb(void *data) {
  s_finishday_hold_timer = NULL;
  finishday_dismiss();
}

static void finishday_anim_cb(void *data) {
  s_finishday_anim_timer = NULL;
  s_finishday_tick++;
  if (s_finishday_tick * FINISHDAY_STEP_MS < FINISHDAY_DRAW_MS) {
    s_finishday_anim_timer = app_timer_register(FINISHDAY_STEP_MS, finishday_anim_cb, NULL);
  }
  if (s_finishday_layer) {
    layer_mark_dirty(s_finishday_layer);
  }
}

static void finishday_update_proc(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, b, 0, GCornerNone);

  int p = s_finishday_tick * FINISHDAY_STEP_MS * 1000 / FINISHDAY_DRAW_MS;
  if (p > 1000) {
    p = 1000;
  }
  GPoint c = grect_center_point(&b);
  int s = (b.size.w < b.size.h ? b.size.w : b.size.h) / 3;
  GPoint a = GPoint(c.x - s, c.y - s / 6);
  GPoint elbow = GPoint(c.x - s / 3, c.y + s / 2);
  GPoint e = GPoint(c.x + s, c.y - s / 2);
  graphics_context_set_stroke_color(ctx, GColorGreen);
  graphics_context_set_stroke_width(ctx, 8);
  int leg1 = 380; // draw the short leg first, then the long one
  if (p > 0 && p < leg1) {
    graphics_draw_line(ctx, a,
        GPoint(a.x + (elbow.x - a.x) * p / leg1, a.y + (elbow.y - a.y) * p / leg1));
  } else if (p >= leg1) {
    graphics_draw_line(ctx, a, elbow);
    int q = p - leg1, d = 1000 - leg1;
    graphics_draw_line(ctx, elbow,
        GPoint(elbow.x + (e.x - elbow.x) * q / d, elbow.y + (e.y - elbow.y) * q / d));
  }
  graphics_context_set_stroke_width(ctx, 1);

  if (p >= 1000) {
    graphics_context_set_text_color(ctx, GColorWhite);
    draw_text(ctx, "Day complete", FONT_KEY_GOTHIC_24_BOLD,
              GRect(0, c.y + s + 6, b.size.w, 30), GTextOverflowModeFill, GTextAlignmentCenter);
  }
}

static void finishday_skip_click(ClickRecognizerRef r, void *ctx) {
  finishday_dismiss();
}

static void finishday_click_config(void *ctx) {
  window_single_click_subscribe(BUTTON_ID_SELECT, finishday_skip_click);
  window_single_click_subscribe(BUTTON_ID_BACK, finishday_skip_click);
  window_single_click_subscribe(BUTTON_ID_UP, finishday_skip_click);
  window_single_click_subscribe(BUTTON_ID_DOWN, finishday_skip_click);
}

static void finishday_window_load(Window *window) {
  Layer *wl = window_get_root_layer(window);
  s_finishday_layer = layer_create(layer_get_bounds(wl));
  layer_set_update_proc(s_finishday_layer, finishday_update_proc);
  layer_add_child(wl, s_finishday_layer);
  window_set_click_config_provider(window, finishday_click_config);
  s_finishday_tick = 0;
  s_finishday_anim_timer = app_timer_register(FINISHDAY_STEP_MS, finishday_anim_cb, NULL);
  s_finishday_hold_timer = app_timer_register(FINISHDAY_HOLD_MS, finishday_hold_cb, NULL);
  vibe_celebrate();
}

static void finishday_window_unload(Window *window) {
  if (s_finishday_anim_timer) {
    app_timer_cancel(s_finishday_anim_timer);
    s_finishday_anim_timer = NULL;
  }
  if (s_finishday_hold_timer) {
    app_timer_cancel(s_finishday_hold_timer);
    s_finishday_hold_timer = NULL;
  }
  layer_destroy(s_finishday_layer);
  s_finishday_layer = NULL;
}

static void push_finishday_window(void) {
  if (!s_finishday_window) {
    s_finishday_window = window_create();
    window_set_background_color(s_finishday_window, GColorBlack);
    window_set_window_handlers(s_finishday_window, (WindowHandlers) {
      .load = finishday_window_load,
      .unload = finishday_window_unload,
    });
  }
  window_stack_push(s_finishday_window, true);
}

static void habit_flash_timer_cb(void *data) {
  s_habit_flash_timer = NULL;
  s_habit_flash_tick++;
  int limit = s_habit_flash_gold ? HABIT_MILESTONE_MS : HABIT_FLASH_MS;
  if (s_habit_flash_tick * HABIT_FLASH_STEP_MS < limit) {
    s_habit_flash_timer = app_timer_register(HABIT_FLASH_STEP_MS, habit_flash_timer_cb, NULL);
  } else {
    s_habit_flash_id[0] = '\0';
  }
  if (s_habits_menu_layer) {
    layer_mark_dirty(menu_layer_get_layer(s_habits_menu_layer));
  }
}

// Pulse the given habit's row: green when a bump just took it to goal, gold
// (longer) when the streak crossed a 7 / 30 / 100 milestone.
static void begin_habit_flash(const char *habit_id, bool milestone) {
  if (s_habit_flash_timer) {
    app_timer_cancel(s_habit_flash_timer);
  }
  str_copy(s_habit_flash_id, habit_id, MAX_HABIT_ID_LEN);
  s_habit_flash_tick = 0;
  s_habit_flash_gold = milestone;
  s_habit_flash_timer = app_timer_register(HABIT_FLASH_STEP_MS, habit_flash_timer_cb, NULL);
  if (milestone) {
    vibe_celebrate();
  }
}

static void stop_habit_flash(void) {
  if (s_habit_flash_timer) {
    app_timer_cancel(s_habit_flash_timer);
    s_habit_flash_timer = NULL;
  }
  s_habit_flash_id[0] = '\0';
  s_habit_milestone_id[0] = '\0';
  if (s_streak_roll_timer) {
    app_timer_cancel(s_streak_roll_timer);
    s_streak_roll_timer = NULL;
  }
  s_streak_roll_id[0] = '\0';
}

// A non-milestone streak +1: roll "N-1 streak" -> "N streak" up on that row over
// STREAK_ROLL_MS. s_streak_roll_id / _prev are set in MSG_HABIT_ITEM; this just
// (re)starts the ticker once MSG_HABIT_SYNC_END has the new count in place.
static void streak_roll_timer_cb(void *data) {
  s_streak_roll_timer = NULL;
  s_streak_roll_tick++;
  if (s_streak_roll_tick * STREAK_ROLL_STEP_MS < STREAK_ROLL_MS) {
    s_streak_roll_timer = app_timer_register(STREAK_ROLL_STEP_MS, streak_roll_timer_cb, NULL);
  } else {
    s_streak_roll_id[0] = '\0';
  }
  if (s_habits_menu_layer) {
    layer_mark_dirty(menu_layer_get_layer(s_habits_menu_layer));
  }
}

static void begin_streak_roll(void) {
  if (s_streak_roll_timer) {
    app_timer_cancel(s_streak_roll_timer);
  }
  s_streak_roll_tick = 0;
  s_streak_roll_timer = app_timer_register(STREAK_ROLL_STEP_MS, streak_roll_timer_cb, NULL);
}

// After a bump completes a habit: if that was the day's last unfinished one,
// arm the Habits nav-row gold flash on the main list (seen once the user backs
// out). Kicks the main scroll timer so the flash actually ticks down.
static void maybe_flash_last_habit(void) {
  if (s_habit_count == 0) {
    return;
  }
  for (int i = 0; i < s_habit_count; i++) {
    if (!s_habits[i].done) {
      return;
    }
  }
  s_lasthabit_flash_active = true;
  s_lasthabit_flash_tick = 0;
  vibe_celebrate();
  refresh_scroll_state(false);
}

// --- habit-title marquee (the habits menu has no scroll timer of its own) ---
static void habits_marquee_tick(void *data) {
  s_habits_marquee_timer = NULL;
  s_scroll_offset_px += SCROLL_STEP_PX;
  if (s_habits_menu_layer) {
    layer_mark_dirty(menu_layer_get_layer(s_habits_menu_layer));
  }
  s_habits_marquee_timer = app_timer_register(SCROLL_INTERVAL_MS, habits_marquee_tick, NULL);
}

static void stop_habits_marquee(void) {
  if (s_habits_marquee_timer) {
    app_timer_cancel(s_habits_marquee_timer);
    s_habits_marquee_timer = NULL;
  }
}

// True if the selected habit's title is wider than its row.
static bool selected_habit_title_overflows(void) {
  if (!s_habits_menu_layer) {
    return false;
  }
  Habit *h = resolve_habit_at(menu_layer_get_selected_index(s_habits_menu_layer));
  if (!h) {
    return false;
  }
  int16_t avail = layer_get_bounds(menu_layer_get_layer(s_habits_menu_layer)).size.w - TITLE_BOX_X * 2;
  return title_natural_width_font(h->title, fonts_get_system_font(TITLE_FONT_KEY)) > avail;
}

// Run the marquee timer only while the selected habit needs it; reset the shared
// offset on a selection change so the new title starts from the left.
static void habits_marquee_refresh(bool reset_offset) {
  if (reset_offset) {
    s_scroll_offset_px = 0;
  }
  if (selected_habit_title_overflows()) {
    if (!s_habits_marquee_timer) {
      s_habits_marquee_timer = app_timer_register(SCROLL_INTERVAL_MS, habits_marquee_tick, NULL);
    }
  } else {
    stop_habits_marquee();
  }
}
#endif

// Total elapsed ms for the current countdown session, paused or running. Only
// meaningful while s_tracking_habit_id is an is_countdown habit.
static int countdown_elapsed_ms(void) {
  if (s_habit_countdown_paused) {
    return s_habit_countdown_frozen_elapsed_ms;
  }
  time_t elapsed_s = time(NULL) - s_tracking_habit_start_epoch;
  int running_ms = elapsed_s > 0 ? (int)elapsed_s * 1000 : 0;
  return s_habit_countdown_frozen_elapsed_ms + running_ms;
}

// A RepeatedCountdownReminder's timer reaching zero: +1 to today's count via
// the same HABIT_ADJUST path a ClickCounter's Select uses (idempotent - the
// phone applies the delta and uploads a plain replace). Fires automatically at
// zero, but does NOT auto-restart the next round (that would rack up completions
// unattended); long-select starts the next round.
static void complete_habit_countdown(Habit *habit) {
  s_tracking_habit_id[0] = '\0';
  s_tracking_habit_start_epoch = 0;
  s_habit_countdown_paused = false;
  s_habit_countdown_frozen_elapsed_ms = 0;
  save_habit_tracking();
  stop_habit_tracking_tick();
  habit->value += 1;
  habit->done = habit->value >= habit->goal;
  save_habits();
  send_habit_adjust(habit, 1);
  if (s_habits_menu_layer) {
    menu_layer_reload_data(s_habits_menu_layer);
  }
}

// Redraw-only ticker for a tracked StopWatch/countdown habit, mirroring
// tracking_tick_callback - mark_dirty, not reload_data (except a countdown
// reaching zero). Guarded on s_habits_menu_layer being non-NULL (it's torn down
// on habits-window unload).
static void habit_tracking_tick_callback(void *data) {
  if (s_tracking_habit_id[0] != '\0') {
    Habit *tracked_habit = find_habit_by_id(s_tracking_habit_id);
    if (tracked_habit && tracked_habit->is_countdown) {
      int remaining_ms = tracked_habit->countdown_ms - countdown_elapsed_ms();
      if (remaining_ms <= 0) {
        complete_habit_countdown(tracked_habit); // re-registers nothing - this round is over
        return;
      }
    }
  }
  if (s_habits_menu_layer) {
    layer_mark_dirty(menu_layer_get_layer(s_habits_menu_layer));
  }
  s_habit_tracking_tick_timer = app_timer_register(TRACKING_TICK_INTERVAL_MS, habit_tracking_tick_callback, NULL);
}

static void start_habit_tracking_tick(void) {
  if (!s_habit_tracking_tick_timer) {
    s_habit_tracking_tick_timer = app_timer_register(TRACKING_TICK_INTERVAL_MS, habit_tracking_tick_callback, NULL);
  }
}

static void start_habit_tracking(Habit *habit) {
  str_copy(s_tracking_habit_id, habit->id, MAX_HABIT_ID_LEN);
  s_tracking_habit_start_epoch = time(NULL);
  // A fresh round always starts running - clear any leftover pause state.
  s_habit_countdown_paused = false;
  s_habit_countdown_frozen_elapsed_ms = 0;
  save_habit_tracking();
  start_habit_tracking_tick();
}

// Select on a tracking is_countdown row toggles paused/running; long-select
// still ends the round. Not offered for a StopWatch (pause is meaningless for
// an open-ended up-count).
static void toggle_habit_countdown_pause(void) {
  if (s_habit_countdown_paused) {
    s_habit_countdown_paused = false;
    s_tracking_habit_start_epoch = time(NULL); // start a fresh running segment
    start_habit_tracking_tick();
  } else {
    s_habit_countdown_frozen_elapsed_ms = countdown_elapsed_ms(); // fold in the running segment before flipping the flag
    s_habit_countdown_paused = true;
    stop_habit_tracking_tick();
  }
  save_habit_tracking();
  if (s_habits_menu_layer) {
    menu_layer_reload_data(s_habits_menu_layer);
  }
}

// Stops whatever StopWatch/countdown habit is being tracked (a no-op if none) -
// mirrors stop_tracking_at, including the optimistic local bump. For a
// RepeatedCountdownReminder this only means "cancelled before completion" (zero
// is handled by complete_habit_countdown), so it's a silent no-upload cancel.
// end_epoch caps the logged duration (the midnight auto-stop passes 00:00 so
// an overnight StopWatch run isn't all dumped onto the new day).
static void stop_habit_tracking_at(time_t end_epoch) {
  if (s_tracking_habit_id[0] == '\0') {
    return;
  }
  Habit *tracked_habit = find_habit_by_id(s_tracking_habit_id);
  if (!(tracked_habit && tracked_habit->is_countdown)) {
    time_t elapsed_s = end_epoch - s_tracking_habit_start_epoch;
    if (elapsed_s > 0) {
      int32_t elapsed_ms = (int32_t)elapsed_s * 1000;
      send_habit_track_stop(s_tracking_habit_id, elapsed_ms);
      if (tracked_habit) {
        tracked_habit->value += elapsed_ms;
        tracked_habit->done = tracked_habit->value >= tracked_habit->goal;
        save_habits();
      }
    }
  }
  s_tracking_habit_id[0] = '\0';
  s_tracking_habit_start_epoch = 0;
  s_habit_countdown_paused = false;
  s_habit_countdown_frozen_elapsed_ms = 0;
  save_habit_tracking();
  stop_habit_tracking_tick();
}

static void stop_habit_tracking_and_report(void) {
  stop_habit_tracking_at(time(NULL));
}
#endif

static uint16_t habits_menu_get_num_sections(MenuLayer *menu_layer, void *context) {
  return 1;
}

static uint16_t habits_menu_get_num_rows(MenuLayer *menu_layer, uint16_t section_index, void *context) {
#ifdef PBL_PLATFORM_APLITE
  // Matches resolve_habit_at's aplite-only filtering.
  uint16_t visible = 0;
  for (int i = 0; i < s_habit_count; i++) {
    if (!s_habits[i].is_stopwatch) {
      visible++;
    }
  }
  return visible;
#else
  return (uint16_t)s_habit_count;
#endif
}

static void habits_menu_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context) {
  Habit *habit = resolve_habit_at(*cell_index);
  if (!habit) {
    return;
  }
  bool is_selected = menu_layer_get_selected_index(s_habits_menu_layer).row == cell_index->row;
  GRect bounds = layer_get_bounds(cell_layer);
  // Cerulean selected background, matching the Habits nav row, so a highlighted
  // habit ties back to the row that brought you here.
  GColor bg = is_selected ? GColorVividCerulean : GColorWhite;
#ifdef PBL_PLATFORM_EMERY
  // Bump-to-goal green pulse, or the longer gold pulse for a streak milestone.
  if (s_habit_flash_id[0] != '\0' &&
      strncmp(s_habit_flash_id, habit->id, MAX_HABIT_ID_LEN) == 0 &&
      (s_habit_flash_tick / 2) % 2 == 0) {
    bg = s_habit_flash_gold ? GColorYellow : GColorMintGreen;
  }
#endif
  // A done habit's title stays full-strength (unlike a done task's, which dims):
  // the "- Done" subtitle carries the signal, and a habit gets incremented past
  // goal / decremented below it on the same day, so dimming would flicker.
  // Text stays black even when selected - the cerulean is light enough.
  GColor fg = GColorBlack;
  fill_bg(ctx, bounds, bg);
  graphics_context_set_text_color(ctx, fg);

  // Non-emery: the fixed 30px top-aligned title box, unchanged. Emery: a
  // measured, vertically-centred block matching draw_task_row.
#ifdef PBL_PLATFORM_EMERY
  GFont habit_title_font = fonts_get_system_font(TITLE_FONT_KEY);
  GSize habit_line = graphics_text_layout_get_content_size(
      "Ag", habit_title_font, GRect(0, 0, 200, 100), GTextOverflowModeFill, GTextAlignmentLeft);
  int16_t habit_title_h = habit_line.h > 0 ? habit_line.h : HEADING_TITLE_H;
  GRect title_box = GRect(TITLE_BOX_X, ROW_TITLE_TOP_Y(bounds.size.h, habit_title_h, SUBTITLE_STRIP_H),
                           bounds.size.w - TITLE_BOX_X * 2, habit_title_h);
  bool title_marquee = is_selected &&
      title_natural_width_font(habit->title, habit_title_font) > title_box.size.w;
  draw_marquee_title(ctx, title_box, habit->title, habit_title_font, title_marquee);
#else
  GRect title_box = GRect(TITLE_BOX_X, TITLE_BOX_Y, bounds.size.w - TITLE_BOX_X * 2, 30);
  draw_text(ctx, habit->title, TITLE_FONT_KEY, title_box, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
#endif

  // "value/goal" always visible, " - Done" appended once the count reaches goal
  // (kept alongside "Done" since incrementing past goal is possible - "Done"
  // alone can't tell 3/3 from 7/3).
  // Sized for the StopWatch worst case: two format_duration_ms outputs joined by
  // " / " plus " - Done".
  char subtitle[56];
#ifndef PBL_PLATFORM_APLITE
  if (habit->is_stopwatch) {
    // "spent / goal" via format_duration_ms - value/goal are ms here.
    // effective_ms adds this session's running elapsed to the synced value.
    bool is_tracking_this = s_tracking_habit_id[0] != '\0' &&
                             strncmp(s_tracking_habit_id, habit->id, MAX_HABIT_ID_LEN) == 0;
    int effective_ms = habit->value;
    if (is_tracking_this) {
      time_t elapsed_s = time(NULL) - s_tracking_habit_start_epoch;
      if (elapsed_s > 0) {
        effective_ms += (int)elapsed_s * 1000;
      }
    }
    bool effective_done = effective_ms >= habit->goal;
    char time_text[20];
    format_duration_ms(effective_ms, is_tracking_this, time_text, sizeof(time_text));
    char goal_text[20];
    format_duration_ms(habit->goal, false, goal_text, sizeof(goal_text));
    if (effective_done) {
      snprintf(subtitle, sizeof(subtitle), "%s / %s - Done", time_text, goal_text);
    } else {
      snprintf(subtitle, sizeof(subtitle), "%s / %s", time_text, goal_text);
    }
  } else if (habit->is_countdown) {
    // Running: remaining time counting to zero, with format_duration_ms's "> "
    // prefix as the "live" marker (dropped while paused). Otherwise: a plain
    // completed-rounds count, same shape as a ClickCounter's value/goal.
    bool is_tracking_this = s_tracking_habit_id[0] != '\0' &&
                             strncmp(s_tracking_habit_id, habit->id, MAX_HABIT_ID_LEN) == 0;
    if (is_tracking_this) {
      int remaining_ms = habit->countdown_ms - countdown_elapsed_ms();
      if (remaining_ms < 0) {
        remaining_ms = 0;
      }
      char time_text[20];
      format_duration_ms(remaining_ms, !s_habit_countdown_paused, time_text, sizeof(time_text));
      snprintf(subtitle, sizeof(subtitle), s_habit_countdown_paused ? "%s left - Paused" : "%s left", time_text);
    } else if (habit->done) {
      snprintf(subtitle, sizeof(subtitle), "%d/%d - Done", habit->value, habit->goal);
    } else {
      snprintf(subtitle, sizeof(subtitle), "%d/%d", habit->value, habit->goal);
    }
  } else
#endif
  // On aplite both branches above are compiled out (resolve_habit_at filters
  // timer habits out), leaving just this plain-count path.
  if (habit->done) {
    snprintf(subtitle, sizeof(subtitle), "%d/%d - Done", habit->value, habit->goal);
  } else {
    snprintf(subtitle, sizeof(subtitle), "%d/%d", habit->value, habit->goal);
  }
#ifdef PBL_PLATFORM_EMERY
  GRect subtitle_box = GRect(TITLE_BOX_X, ROW_SUBTITLE_TOP_Y(bounds.size.h, habit_title_h, SUBTITLE_STRIP_H),
                              bounds.size.w - TITLE_BOX_X * 2, SUBTITLE_STRIP_H);
#else
  GRect subtitle_box = GRect(TITLE_BOX_X, bounds.size.h - SUBTITLE_STRIP_H,
                              bounds.size.w - TITLE_BOX_X * 2, SUBTITLE_STRIP_H);
#endif
  GRect left_sub = subtitle_box;
#ifndef PBL_PLATFORM_APLITE
  // Right-aligned streak label on the subtitle line. Current streak from 2 up
  // ("1 streak" reads oddly); at 7+ it's a milestone - bold white-on-black
  // rounded badge (black-on-white contrasts on the cerulean selected row too).
  // Milestone tiers 7 / 30 / 100 add 1 / 2 / 3 white pips inside the badge.
  // Once the current streak lapses below 2, show "best N" instead as the
  // record to chase, when there is one.
  char st[16];
  bool milestone = false;
  if (habit->streak >= 2) {
    snprintf(st, sizeof(st), "%d streak", habit->streak);
    milestone = habit->streak >= 7;
  } else if (habit->best_streak >= 2) {
    snprintf(st, sizeof(st), "best %d", habit->best_streak);
  } else {
    st[0] = '\0';
  }
  // A live streak still unextended late in the day: the badge goes amber - a
  // nudge without a notification.
  bool at_risk = false;
#ifdef PBL_COLOR
  if (!habit->done && habit->streak >= 2) {
    time_t nowt = time(NULL);
    struct tm *lt = localtime(&nowt);
    at_risk = lt->tm_hour >= 17;
  }
#endif
  if (st[0]) {
    GFont sf = fonts_get_system_font(milestone ? FONT_KEY_GOTHIC_14_BOLD : FONT_KEY_GOTHIC_14);
    GSize ss = graphics_text_layout_get_content_size(
        st, sf, subtitle_box, GTextOverflowModeTrailingEllipsis, GTextAlignmentRight);
    int16_t sw = ss.w;
    if (sw > subtitle_box.size.w / 2) {
      sw = subtitle_box.size.w / 2;
    }
    int tier = habit->streak >= 100 ? 3 : habit->streak >= 30 ? 2 : milestone ? 1 : 0;
    int16_t pips_w = tier ? tier * 4 + 1 : 0; // tier 3px pips, 1px apart, +2px lead
    int16_t pad = milestone ? 3 : 0;
    int16_t badge_w = sw + pad * 2 + pips_w;
    int16_t badge_x = subtitle_box.origin.x + subtitle_box.size.w - badge_w;
    GRect text_box = GRect(badge_x + pad + pips_w, subtitle_box.origin.y + 2,
                           sw, subtitle_box.size.h - 2);
    // On open, the badge itself wipes in from its (fixed, right-aligned) far
    // edge inward - header_bar_w's usual left-to-right growth, mirrored so a
    // right-anchored badge still grows away from its anchor. Pips/text pop in
    // once the wipe reaches the near edge, like every other header_bar_w use.
    bool milestone_revealed = true;
    if (milestone) {
      int16_t rw = header_bar_w(badge_w);
      int16_t rx = badge_x + (badge_w - rw);
      milestone_revealed = rw >= badge_w;
      graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(at_risk ? GColorOrange : GColorBlack, GColorBlack));
      graphics_fill_rect(ctx, GRect(rx, subtitle_box.origin.y, rw, subtitle_box.size.h),
                         3, GCornersAll);
      if (milestone_revealed) {
        graphics_context_set_fill_color(ctx, GColorWhite);
        for (int p = 0; p < tier; p++) {
          graphics_fill_rect(ctx, GRect(badge_x + 3 + p * 4,
                                        subtitle_box.origin.y + subtitle_box.size.h / 2 - 1, 3, 3),
                             0, GCornerNone);
        }
        graphics_context_set_text_color(ctx, GColorWhite);
      }
    } else if (at_risk) {
      graphics_context_set_text_color(ctx, GColorOrange);
    }
    bool rolling = false;
#ifdef PBL_PLATFORM_EMERY
    rolling = !milestone && s_streak_roll_id[0] != '\0' &&
              strncmp(s_streak_roll_id, habit->id, MAX_HABIT_ID_LEN) == 0;
    if (rolling) {
      // "N-1 streak" slides up and out, "N streak" slides up into its place.
      int rp = s_streak_roll_tick * STREAK_ROLL_STEP_MS * 1000 / STREAK_ROLL_MS;
      if (rp > 1000) {
        rp = 1000;
      }
      int16_t travel = text_box.size.h;
      GRect wide = GRect(text_box.origin.x - 2, text_box.origin.y, sw + 4, travel);
      graphics_draw_text(ctx, s_streak_roll_prev, sf,
          GRect(wide.origin.x, wide.origin.y - travel * rp / 1000, wide.size.w, travel),
          GTextOverflowModeFill, GTextAlignmentRight, NULL);
      graphics_draw_text(ctx, st, sf,
          GRect(wide.origin.x, wide.origin.y + travel * (1000 - rp) / 1000, wide.size.w, travel),
          GTextOverflowModeFill, GTextAlignmentRight, NULL);
      // Trim the slide's overshoot back to the row background, above and below.
      graphics_context_set_fill_color(ctx, bg);
      graphics_fill_rect(ctx, GRect(wide.origin.x, subtitle_box.origin.y - 6, wide.size.w, 6), 0, GCornerNone);
      graphics_fill_rect(ctx, GRect(wide.origin.x, subtitle_box.origin.y + subtitle_box.size.h,
                                    wide.size.w, 6), 0, GCornerNone);
    }
#endif
    if (!rolling && milestone_revealed) {
      graphics_draw_text(ctx, st, sf, text_box,
          GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
    }
    if (milestone || at_risk) {
      graphics_context_set_text_color(ctx, fg);
    }
    left_sub.size.w -= (badge_w + 4);
  }
#endif
  draw_text(ctx, subtitle, SUBTITLE_FONT_KEY, left_sub, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
}

// Select +1, long-select -1 (never below 0). The phone applies the delta to its
// cached value and uploads a plain replace, so this local bump is a guess
// corrected by the next full sync.
static void adjust_habit(MenuIndex index, int32_t delta) {
  Habit *habit = resolve_habit_at(index);
  if (!habit || habit->value + delta < 0) {
    return; // already at 0, trying to go lower - silent no-op
  }
  bool was_done = habit->done;
  habit->value += delta;
  habit->done = habit->value >= habit->goal;
#ifdef PBL_PLATFORM_EMERY
  if (habit->done && !was_done) {
    begin_habit_flash(habit->id, false);
    maybe_flash_last_habit();
  }
#else
  (void)was_done;
#endif
  save_habits();
  menu_layer_reload_data(s_habits_menu_layer);
  send_habit_adjust(habit, delta);
}

static void habits_menu_select_click(MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  backlight_touch();
#if defined(PBL_TOUCH)
  // A tap that moved the highlight to a new row only selects it (see
  // habits_menu_selection_changed); a tap on the already-selected row marks
  // it done, mirroring menu_select_click on the main list. Long-hold unmarks
  // - see touch_longpress_fire, which calls habits_menu_select_long_click.
  if (consume_tap_select_guard()) {
    if (s_tap_moved_sel || s_touch_longpress_fired) {
      return;
    }
  }
#endif
  Habit *habit = resolve_habit_at(*cell_index);
#ifndef PBL_PLATFORM_APLITE
  // Select pauses/resumes an is_countdown habit's timer while it's tracking
  // (long-select still ends the round). Not for a StopWatch. Otherwise falls
  // through to the no-op below - long-select starts a fresh round.
  if (habit && habit->is_countdown) {
    bool is_tracking_this = s_tracking_habit_id[0] != '\0' &&
                             strncmp(s_tracking_habit_id, habit->id, MAX_HABIT_ID_LEN) == 0;
    if (is_tracking_this) {
      toggle_habit_countdown_pause();
    }
    return;
  }
#endif
  // is_countdown is never true on aplite - is_stopwatch alone saves a few bytes.
#ifdef PBL_PLATFORM_APLITE
  if (habit && habit->is_stopwatch) {
#else
  if (habit && (habit->is_stopwatch || habit->is_countdown)) {
#endif
    return; // no plain-count action for a timer habit - long-select runs its timer
  }
  adjust_habit(*cell_index, 1);
}

// Long-select toggles the timer on a StopWatch/countdown habit (one at a time,
// mirroring the task timer), or decrements a ClickCounter by one.
// stop_habit_tracking_and_report() handles the two timer types itself.
static void habits_menu_select_long_click(MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  backlight_touch();
  Habit *habit = resolve_habit_at(*cell_index);
#ifdef PBL_PLATFORM_APLITE
  if (habit && habit->is_stopwatch) {
#else
  if (habit && (habit->is_stopwatch || habit->is_countdown)) {
#endif
#ifndef PBL_PLATFORM_APLITE
    bool already_tracking_this = s_tracking_habit_id[0] != '\0' &&
                                  strncmp(s_tracking_habit_id, habit->id, MAX_HABIT_ID_LEN) == 0;
    stop_habit_tracking_and_report();
    if (!already_tracking_this) {
      start_habit_tracking(habit);
    }
    menu_layer_reload_data(s_habits_menu_layer);
#endif
    return; // no tracking on aplite - a long-press on a timer row is a no-op there
  }
  adjust_habit(*cell_index, -1);
}

static void update_habits_empty_layer(void) {
  // habits_menu_get_num_rows(), not s_habit_count - on aplite a list of only
  // StopWatch habits has s_habit_count > 0 but zero visible rows.
  bool show_empty = (habits_menu_get_num_rows(NULL, 0, NULL) == 0);
  layer_set_hidden(text_layer_get_layer(s_habits_empty_layer), !show_empty);
  layer_set_hidden(menu_layer_get_layer(s_habits_menu_layer), show_empty);
}

#ifndef PBL_PLATFORM_APLITE
static void habits_menu_selection_changed(MenuLayer *menu_layer, MenuIndex new_index, MenuIndex old_index, void *context) {
  backlight_touch();
#ifdef PBL_PLATFORM_EMERY
  habits_marquee_refresh(true);
#endif
#if defined(PBL_TOUCH)
  // A tap that moved the highlight to a new row is select-only; a tap on the
  // already-selected row marks it done - see habits_menu_select_click.
  s_tap_moved_sel = true;
#endif
}
#endif

#ifndef PBL_PLATFORM_APLITE
// Long-Up on a plain ClickCounter habit opens the value picker (Select +1 and
// long-Select -1 are unchanged; a timer habit has no set-value action). Wraps
// the MenuLayer's own provider the same way the main / browse windows do.
static ClickConfigProvider s_habits_menu_ccp = NULL;

static void habits_value_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (!s_habits_menu_layer) {
    return;
  }
  Habit *h = resolve_habit_at(menu_layer_get_selected_index(s_habits_menu_layer));
  if (!h || h->is_stopwatch || h->is_countdown) {
    return;
  }
  backlight_touch();
  push_value_picker(PICK_HABIT, h->id, h->value);
}

static void habits_menu_click_config_provider(void *context) {
  if (s_habits_menu_ccp) {
    s_habits_menu_ccp(context);
  }
  window_long_click_subscribe(BUTTON_ID_UP, 0, habits_value_long_click_handler, NULL);
}
#endif

static void habits_window_load(Window *window) {
  Layer *window_layer;
  GRect content_bounds = window_chrome(window, &s_habits_status_bar, &window_layer);

  s_habits_menu_layer = menu_layer_create(content_bounds);
  menu_layer_set_callbacks(s_habits_menu_layer, NULL, (MenuLayerCallbacks) {
    .get_num_sections = habits_menu_get_num_sections,
    .get_num_rows = habits_menu_get_num_rows,
    .draw_row = habits_menu_draw_row,
    .select_click = habits_menu_select_click,
    .select_long_click = habits_menu_select_long_click,
#ifndef PBL_PLATFORM_APLITE
    .selection_changed = habits_menu_selection_changed,
#endif
  });
  menu_layer_set_click_config_onto_window(s_habits_menu_layer, window);
#ifndef PBL_PLATFORM_APLITE
  s_habits_menu_ccp = window_get_click_config_provider(window);
  window_set_click_config_provider_with_context(window, habits_menu_click_config_provider,
                                                window_get_click_config_context(window));
#endif
  layer_add_child(window_layer, menu_layer_get_layer(s_habits_menu_layer));

  s_habits_empty_layer = make_text_layer(window_layer, content_bounds,
                                         EMPTY_MSG_FONT_KEY, GTextAlignmentCenter);
  text_layer_set_text(s_habits_empty_layer, "No habits synced.");

  update_habits_empty_layer();

#ifndef PBL_PLATFORM_APLITE
  // Resume the live-ticking redraw if a habit was already being tracked (the
  // elapsed time comes from the persisted start timestamp; this just restarts
  // the redraw). Not while paused - nothing to tick.
  if (s_tracking_habit_id[0] != '\0' && !s_habit_countdown_paused) {
    start_habit_tracking_tick();
  }
#endif
#ifdef PBL_PLATFORM_EMERY
  habits_marquee_refresh(true); // in case the first selected habit's title is long
  header_begin_reveal(); // streak-milestone badges wipe in on open
#endif
}

static void habits_window_unload(Window *window) {
#ifndef PBL_PLATFORM_APLITE
  // Cancel before destroying s_habits_menu_layer - a still-running timer
  // touching a destroyed layer is what this ordering avoids.
  stop_habit_tracking_tick();
#endif
#ifdef PBL_PLATFORM_EMERY
  stop_habit_flash();
  stop_habits_marquee();
  header_cancel_reveal();
#endif
  menu_layer_destroy(s_habits_menu_layer);
#ifndef PBL_PLATFORM_APLITE
  s_habits_menu_ccp = NULL;
#endif
  text_layer_destroy(s_habits_empty_layer);
  status_bar_layer_destroy(s_habits_status_bar);
}

// Created once and reused (pushed again on every visit) - only its layers are
// torn down and rebuilt each time, so at most one window's worth exists.
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
