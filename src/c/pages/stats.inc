// Stats page.
// Textually #included by main.c (one translation unit) - not compiled alone.

// ---------- stats page ----------
// A pinned section-0 row ("Stats", between Projects and Add Task) opens this
// read-only scrollable summary: estimate remaining today, time worked today,
// time tracked without a break, the current tracking session, tasks completed
// today, then every project with its task count. The phone sends the two
// durations, the done count and a preformatted "Name - count" project block in
// one MSG_STATS_DATA. A custom layer inside a ScrollLayer draws each metric as a
// black label bar (white text) with its value below in black - so the label
// reads as a heading and the value as data. "Without a break" and "Current
// session" are recomputed on every draw from watch-local state (s_break_*,
// s_tracking_start_epoch), or - for the session - a remote device's live
// presence (s_presence_state / s_presence_elapsed_base).
#ifdef PBL_PLATFORM_EMERY
#define STATS_LABEL_FONT FONT_KEY_GOTHIC_24_BOLD
#define STATS_VALUE_FONT FONT_KEY_GOTHIC_28_BOLD
#define STATS_LINE_FONT  FONT_KEY_GOTHIC_24
#define STATS_LABEL_H 28
#define STATS_VALUE_H 34
#define STATS_LINE_H  30
#else
#define STATS_LABEL_FONT FONT_KEY_GOTHIC_18_BOLD
#define STATS_VALUE_FONT FONT_KEY_GOTHIC_24_BOLD
#define STATS_LINE_FONT  FONT_KEY_GOTHIC_18
#define STATS_LABEL_H 22
#define STATS_VALUE_H 28
#define STATS_LINE_H  24
#endif
#define STATS_GAP 6
#define STATS_PAD_X 6
// A "\x03"-prefixed line in the shared page-window text is a subtext line -
// small grey, indented, shorter row height. Only the search blob emits it
// (the project a result lives in, under its title).
#define PAGE_SUB_H (STATS_LINE_H * 2 / 3)
static Window *s_stats_window;
static StatusBarLayer *s_stats_status_bar;
static ScrollLayer *s_stats_scroll_layer;
static Layer *s_stats_content_layer;
static GRect s_stats_content_bounds;
static char s_stats_est[24] = "";
static char s_stats_worked[24] = "";
static char s_stats_session[40] = "";
static char s_stats_nobreak[24] = "";

// Recompute the value strings (the durations from the last payload, the
// session from live tracking state).
static void stats_compute_values(void) {
  format_duration_ms(s_stats_est_remaining_ms < 0 ? 0 : s_stats_est_remaining_ms, false,
                     s_stats_est, sizeof(s_stats_est));
  format_duration_ms(s_stats_worked_today_ms < 0 ? 0 : s_stats_worked_today_ms, false,
                     s_stats_worked, sizeof(s_stats_worked));
  char dur[24];
  if (s_tracking_task_id[0] != '\0') {
    int e = (int)((time(NULL) - s_tracking_start_epoch) * 1000);
    format_duration_ms(e < 0 ? 0 : e, false, dur, sizeof(dur));
    str_copy(s_stats_session, dur, sizeof(s_stats_session));
  } else if (s_presence_state == 1) {
    int e = (int)((time(NULL) - s_presence_elapsed_base) * 1000);
    format_duration_ms(e < 0 ? 0 : e, false, dur, sizeof(dur));
    snprintf(s_stats_session, sizeof(s_stats_session), "%s (%s)", dur,
             s_presence_device[0] != '\0' ? s_presence_device : "remote");
  } else {
    str_copy(s_stats_session, "Not tracking", sizeof(s_stats_session));
  }

  // "Without a break" - the break reminder's tally: watch-tracked time banked
  // since the last pause of BREAK_RESET_GAP_S or more, plus the running session.
  int nb = s_break_accum_s;
  if (s_tracking_task_id[0] != '\0' && time(NULL) > s_tracking_start_epoch) {
    nb += (int)(time(NULL) - s_tracking_start_epoch);
  }
  format_duration_ms(nb * 1000, false, s_stats_nobreak, sizeof(s_stats_nobreak));
}

// On emery, a black header bar - stats metric/section, the Upcoming/Notes/
// Search page's day/section headers, or a habit row's streak-milestone badge -
// wipes in left-to-right over ~200ms when its page opens (header_begin_reveal),
// rather than snapping in fully drawn; the white label/pips/text pops in once
// the wipe reaches the far edge. One shared timer, since only one such window
// is ever open at a time. Other platforms skip this - header_bar_w is just `w`.
static Layer *s_upcoming_content_layer; // real declaration further down, with the rest of its window's state
#ifdef PBL_PLATFORM_EMERY
#define HEADER_REVEAL_STEPS 8
#define HEADER_REVEAL_STEP_MS 25
static AppTimer *s_header_reveal_timer = NULL;
static uint8_t s_header_reveal_tick = HEADER_REVEAL_STEPS; // full unless animating

static void header_reveal_tick_cb(void *data) {
  s_header_reveal_timer = NULL;
  s_header_reveal_tick++;
  if (s_stats_content_layer) {
    layer_mark_dirty(s_stats_content_layer);
  }
  if (s_upcoming_content_layer) {
    layer_mark_dirty(s_upcoming_content_layer);
  }
  if (s_habits_menu_layer) {
    layer_mark_dirty(menu_layer_get_layer(s_habits_menu_layer));
  }
  if (s_header_reveal_tick < HEADER_REVEAL_STEPS) {
    s_header_reveal_timer = app_timer_register(HEADER_REVEAL_STEP_MS, header_reveal_tick_cb, NULL);
  }
}

static void header_begin_reveal(void) {
  if (s_header_reveal_timer) {
    app_timer_cancel(s_header_reveal_timer);
  }
  s_header_reveal_tick = 0;
  s_header_reveal_timer = app_timer_register(HEADER_REVEAL_STEP_MS, header_reveal_tick_cb, NULL);
}

static void header_cancel_reveal(void) {
  if (s_header_reveal_timer) {
    app_timer_cancel(s_header_reveal_timer);
    s_header_reveal_timer = NULL;
  }
  s_header_reveal_tick = HEADER_REVEAL_STEPS;
}

static int16_t header_bar_w(int16_t w) {
  if (s_header_reveal_tick >= HEADER_REVEAL_STEPS) {
    return w;
  }
  return (int16_t)(((int32_t)w * s_header_reveal_tick) / HEADER_REVEAL_STEPS);
}
#else
static void header_begin_reveal(void) {}
static void header_cancel_reveal(void) {}
static int16_t header_bar_w(int16_t w) { return w; }
#endif

// Draws one label bar (black, white text) with its value below (black text).
static int16_t stats_draw_metric(GContext *ctx, int16_t y, int16_t w,
                                  const char *label, const char *value) {
  int16_t bar_w = header_bar_w(w);
  fill_bg(ctx, GRect(0, y, bar_w, STATS_LABEL_H), GColorBlack);
  if (bar_w >= w) {
    graphics_context_set_text_color(ctx, GColorWhite);
    draw_text(ctx, label, STATS_LABEL_FONT, GRect(STATS_PAD_X, y, w - STATS_PAD_X * 2, STATS_LABEL_H), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
  }
  y += STATS_LABEL_H;
  graphics_context_set_text_color(ctx, GColorBlack);
  draw_text(ctx, value, STATS_VALUE_FONT, GRect(STATS_PAD_X, y, w - STATS_PAD_X * 2, STATS_VALUE_H), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
  return y + STATS_VALUE_H + STATS_GAP;
}

// Counts s_stats_projects' lines by kind: "\x02"-prefixed section headers draw
// as a black bar (STATS_LABEL_H + 2), the rest as plain lines (STATS_LINE_H).
static void stats_text_line_counts(int *label_lines, int *text_lines) {
  *label_lines = 0;
  *text_lines = 0;
  if (!s_stats_projects || s_stats_projects[0] == '\0') {
    *text_lines = 1;
    return;
  }
  bool sol = true;
  for (const char *p = s_stats_projects; *p; p++) {
    if (sol) {
      if (*p == '\x02') {
        (*label_lines)++;
      } else {
        (*text_lines)++;
      }
      sol = false;
    }
    if (*p == '\n') {
      sol = true;
    }
  }
}

static int16_t stats_content_height(void) {
  int label_lines, text_lines;
  stats_text_line_counts(&label_lines, &text_lines);
  return (s_yesterday_stats_enabled ? STATS_LABEL_H + 2 : 0)  // Today/Yesterday bar
       + (STATS_LABEL_H + STATS_VALUE_H + STATS_GAP) * 7       // the seven metrics
       + label_lines * (STATS_LABEL_H + 2)
       + text_lines * STATS_LINE_H
       + 8;
}

static void stats_content_update_proc(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  int16_t w = b.size.w;
  fill_bg(ctx, b, GColorWhite);

  if (!s_stats_have_data) {
    graphics_context_set_text_color(ctx, GColorBlack);
    draw_text(ctx, "Loading…", STATS_LINE_FONT, GRect(STATS_PAD_X, 8, w - STATS_PAD_X * 2, STATS_LINE_H), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
    return;
  }

  char done_buf[12];
  snprintf(done_buf, sizeof(done_buf), "%d", s_stats_done_today);
  focus_roll_day();
  char focus_buf[12];
  snprintf(focus_buf, sizeof(focus_buf), "%d", s_focus_completed_today);
  // "Break time" - count / total duration of today's real breaks.
  break_roll_day();
  char break_buf[24];
  if (s_break_count_today > 0) {
    char bdur[16];
    format_duration_ms(s_break_total_s_today * 1000, false, bdur, sizeof(bdur));
    snprintf(break_buf, sizeof(break_buf), "%d / %s", s_break_count_today, bdur);
  } else {
    str_copy(break_buf, "0", sizeof(break_buf));
  }
  // "Yesterday" view (toggled with long Up/Down): the two figures the phone
  // sends for that day; the live watch-local metrics show a dash.
  bool yd = s_stats_show_yesterday;
  char worked_y[24];
  if (yd) {
    format_duration_ms(s_stats_worked_yesterday_ms < 0 ? 0 : s_stats_worked_yesterday_ms,
                       false, worked_y, sizeof(worked_y));
    snprintf(done_buf, sizeof(done_buf), "%d", s_stats_done_yesterday);
  }

  int16_t y = 0;
  // When the yesterday toggle is on, a centred bar names which day is shown.
  if (s_yesterday_stats_enabled) {
    int16_t bar_w = header_bar_w(w);
    fill_bg(ctx, GRect(0, y, bar_w, STATS_LABEL_H), GColorBlack);
    if (bar_w >= w) {
      graphics_context_set_text_color(ctx, GColorWhite);
      draw_text(ctx, yd ? "Yesterday" : "Today", STATS_LABEL_FONT,
                GRect(STATS_PAD_X, y, w - STATS_PAD_X * 2, STATS_LABEL_H),
                GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter);
    }
    y += STATS_LABEL_H + 2;
  }
  y = stats_draw_metric(ctx, y, w, "Estimate remaining", yd ? "-" : s_stats_est);
  y = stats_draw_metric(ctx, y, w, yd ? "Worked yesterday" : "Worked today", yd ? worked_y : s_stats_worked);
  y = stats_draw_metric(ctx, y, w, "Without a break", yd ? "-" : s_stats_nobreak);
  y = stats_draw_metric(ctx, y, w, "Break time", yd ? "-" : break_buf);
  y = stats_draw_metric(ctx, y, w, "Current session", yd ? "-" : s_stats_session);
  y = stats_draw_metric(ctx, y, w, yd ? "Done yesterday" : "Completed today", done_buf);
  y = stats_draw_metric(ctx, y, w, "Focus sessions", yd ? "-" : focus_buf);

  graphics_context_set_text_color(ctx, GColorBlack);
  GFont line_font = fonts_get_system_font(STATS_LINE_FONT);
  if (!s_stats_projects || s_stats_projects[0] == '\0') {
    graphics_draw_text(ctx, "None", line_font,
                        GRect(STATS_PAD_X, y, w - STATS_PAD_X * 2, STATS_LINE_H),
                        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
    return;
  }
  // Verbatim lines from s_stats_projects: a "\x02" prefix marks a section
  // header (black bar), everything else is a plain line. See handleStatsRequest.
  const char *p = s_stats_projects;
  char line[80];
  while (*p) {
    const char *nl = strchr(p, '\n');
    size_t len = nl ? (size_t)(nl - p) : strlen(p);
    if (len >= sizeof(line)) {
      len = sizeof(line) - 1;
    }
    memcpy(line, p, len);
    line[len] = '\0';
    if (line[0] == '\x02') {
      int16_t bar_w = header_bar_w(w);
      fill_bg(ctx, GRect(0, y, bar_w, STATS_LABEL_H), GColorBlack);
      if (bar_w >= w) {
        graphics_context_set_text_color(ctx, GColorWhite);
        draw_text(ctx, line + 1, STATS_LABEL_FONT,
                  GRect(STATS_PAD_X, y, w - STATS_PAD_X * 2, STATS_LABEL_H),
                  GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
        graphics_context_set_text_color(ctx, GColorBlack);
      }
      y += STATS_LABEL_H + 2;
    } else {
      graphics_draw_text(ctx, line, line_font,
                          GRect(STATS_PAD_X, y, w - STATS_PAD_X * 2, STATS_LINE_H),
                          GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
      y += STATS_LINE_H;
    }
    if (!nl) {
      break;
    }
    p = nl + 1;
  }
}

static void stats_render(void) {
  if (!s_stats_content_layer) {
    return;
  }
  stats_compute_values();
  int16_t h = s_stats_have_data ? stats_content_height() : s_stats_content_bounds.size.h;
  if (h < s_stats_content_bounds.size.h) {
    h = s_stats_content_bounds.size.h;
  }
  layer_set_frame(s_stats_content_layer, GRect(0, 0, s_stats_content_bounds.size.w, h));
  scroll_layer_set_content_size(s_stats_scroll_layer, GSize(s_stats_content_bounds.size.w, h));
  layer_mark_dirty(s_stats_content_layer);
}

static void request_stats(void) {
  begin_send(MSG_STATS_REQUEST, NULL, NULL, 0);
}

// Long Up/Down on the Stats page flips between today and yesterday, when the
// "Yesterday toggle" setting is on. Wraps the scroll layer's own click config.
static ClickConfigProvider s_stats_ccp = NULL;

static void stats_toggle_yesterday_handler(ClickRecognizerRef recognizer, void *context) {
  if (!s_yesterday_stats_enabled) {
    return;
  }
  s_stats_show_yesterday = !s_stats_show_yesterday;
  vibes_short_pulse();
  stats_render();
}

static void stats_window_click_config_provider(void *context) {
  if (s_stats_ccp) {
    s_stats_ccp(context);
  }
  window_long_click_subscribe(BUTTON_ID_UP, 0, stats_toggle_yesterday_handler, NULL);
  window_long_click_subscribe(BUTTON_ID_DOWN, 0, stats_toggle_yesterday_handler, NULL);
}

static void stats_window_load(Window *window) {
  Layer *window_layer;
  s_stats_content_bounds = window_chrome(window, &s_stats_status_bar, &window_layer);

  // Heap-backed text buffer, alive only for this window (see its declaration).
  free(s_stats_projects);
  s_stats_projects = malloc(STATS_TEXT_CAP);
  if (s_stats_projects) {
    s_stats_projects[0] = '\0';
  }
  s_stats_have_data = false;

  s_stats_scroll_layer = scroll_layer_create(s_stats_content_bounds);
  scroll_layer_set_content_size(s_stats_scroll_layer, s_stats_content_bounds.size);
  scroll_layer_set_click_config_onto_window(s_stats_scroll_layer, window);
  s_stats_ccp = window_get_click_config_provider(window);
  window_set_click_config_provider_with_context(window, stats_window_click_config_provider,
                                                window_get_click_config_context(window));

  s_stats_content_layer = layer_create(GRect(0, 0, s_stats_content_bounds.size.w,
                                             s_stats_content_bounds.size.h));
  layer_set_update_proc(s_stats_content_layer, stats_content_update_proc);
  scroll_layer_add_child(s_stats_scroll_layer, s_stats_content_layer);
  layer_add_child(window_layer, scroll_layer_get_layer(s_stats_scroll_layer));

  stats_render();
}

static void stats_window_unload(Window *window) {
  s_stats_show_yesterday = false;
  s_stats_ccp = NULL;
  header_cancel_reveal();
  layer_destroy(s_stats_content_layer);
  s_stats_content_layer = NULL;
  scroll_layer_destroy(s_stats_scroll_layer);
  s_stats_scroll_layer = NULL;
  status_bar_layer_destroy(s_stats_status_bar);
  s_stats_status_bar = NULL;
  free(s_stats_projects);
  s_stats_projects = NULL;
  s_stats_have_data = false;
}

// Created once and reused, like push_notes_window. Keeps whatever data the
// last visit fetched on screen and re-requests in the background.
static void push_stats_window(void) {
  if (!s_stats_window) {
    s_stats_window = window_create();
    window_set_window_handlers(s_stats_window, (WindowHandlers) {
      .load = stats_window_load,
      .unload = stats_window_unload,
    });
  }
  window_stack_push(s_stats_window, true);
  request_stats();
}
