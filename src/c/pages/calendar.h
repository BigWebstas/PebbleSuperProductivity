// Calendar month view (emery).
// Textually #included by main.c (one translation unit) - not compiled alone.

// ---------- calendar month view (browse-only) ----------
// A month grid: Up/Down move the day cursor, long-Up/long-Down change month,
// Select opens that day's tasks in the browse window (BROWSE_CALENDAR_DAY),
// with "Mark done" and the rest of the action menu now reachable there.
// Pure watch-side month/weekday-grid math (day_of_week et al below) - the
// phone is only asked which days in the shown month have a due task
// (CAL_MONTH_MASK). config.enableCalendar, default off. Emery-only - the full
// grid window pushed basalt/chalk/diorite's APP region 784B past its ceiling
// (see emery-code-space-ceiling in memory), so unlike the rest of the
// date-browsing pages this one doesn't fit non-emery.
#ifdef PBL_PLATFORM_EMERY
static Layer *s_calendar_layer = NULL;
static StatusBarLayer *s_calendar_status_bar = NULL;
static int s_cal_month_offset = 0;    // months from the real current month
static int s_cal_selected_day = 1;    // 1-based cursor within the shown month
static uint32_t s_cal_month_mask = 0; // bit (day-1) set = that day has a due task
static bool s_cal_have_data = false;

static const char *const CAL_MONTH_NAMES[12] = {
  "January", "February", "March", "April", "May", "June",
  "July", "August", "September", "October", "November", "December",
};

// Sakamoto's algorithm - day of week (0 = Sunday) for Gregorian y-m-d, m 1-12.
// Self-contained so the month grid doesn't need mktime's normalization.
static int day_of_week(int y, int m, int d) {
  static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (m < 3) {
    y -= 1;
  }
  return (y + y / 4 - y / 100 + y / 400 + t[m - 1] + d) % 7;
}

static bool cal_is_leap_year(int y) {
  return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
}

static int cal_days_in_month(int y, int m0) { // m0: 0-11
  static const int d[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  return (m0 == 1 && cal_is_leap_year(y)) ? 29 : d[m0];
}

// The calendar's shown year/month: real current month + s_cal_month_offset,
// normalized so a run of long-Up/long-Down still lands on a valid month.
static void calendar_shown_month(int *year_out, int *month0_out) {
  time_t now = time(NULL);
  struct tm *lt = localtime(&now);
  int year = lt->tm_year + 1900;
  int month0 = lt->tm_mon + s_cal_month_offset;
  year += month0 / 12;
  month0 %= 12;
  if (month0 < 0) {
    month0 += 12;
    year -= 1;
  }
  *year_out = year;
  *month0_out = month0;
}

static void request_calendar_month(void) {
  s_cal_have_data = false;
  begin_send(MSG_CAL_MONTH_REQUEST, NULL, NULL, s_cal_month_offset);
}

#define CAL_LABEL_H 26
#define CAL_DOW_H 18

static void calendar_content_update_proc(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  int16_t w = b.size.w;
  int16_t h = b.size.h;
  fill_bg(ctx, b, GColorWhite);

  int year, month0;
  calendar_shown_month(&year, &month0);
  int days = cal_days_in_month(year, month0);
  int first_dow = day_of_week(year, month0 + 1, 1);

  char label[24];
  snprintf(label, sizeof(label), "%s %d", CAL_MONTH_NAMES[month0], year);
  graphics_context_set_text_color(ctx, GColorBlack);
  draw_text(ctx, label, HEADING_FONT_KEY, GRect(0, 2, w, CAL_LABEL_H - 2),
            GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter);

  static const char *const DOW_LABELS[7] = {"S", "M", "T", "W", "T", "F", "S"};
  int16_t col_w = w / 7;
  graphics_context_set_text_color(ctx, GColorDarkGray);
  for (int c = 0; c < 7; c++) {
    draw_text(ctx, DOW_LABELS[c], CHROME_FONT_KEY,
              GRect(c * col_w, CAL_LABEL_H, col_w, CAL_DOW_H),
              GTextOverflowModeFill, GTextAlignmentCenter);
  }

  int16_t grid_y = CAL_LABEL_H + CAL_DOW_H;
  int16_t grid_h = h - grid_y;
  int16_t row_h = grid_h / 6;

  bool is_current_month = (s_cal_month_offset == 0); // "today" only means something then
  time_t now = time(NULL);
  int today_mday = localtime(&now)->tm_mday;

  for (int day = 1; day <= days; day++) {
    int idx = first_dow + day - 1;
    int row = idx / 7;
    int col = idx % 7;
    GRect cell = GRect(col * col_w, grid_y + row * row_h, col_w, row_h);

    if (is_current_month && day == today_mday) {
      graphics_context_set_fill_color(ctx, GColorLightGray);
      graphics_fill_rect(ctx, GRect(cell.origin.x + 2, cell.origin.y + 2,
                                    cell.size.w - 4, cell.size.h - 4), 2, GCornersAll);
    }
    if (day == s_cal_selected_day) {
      graphics_context_set_stroke_color(ctx, GColorBlack);
      graphics_draw_rect(ctx, GRect(cell.origin.x + 1, cell.origin.y + 1,
                                    cell.size.w - 2, cell.size.h - 2));
    }

    char dbuf[3];
    snprintf(dbuf, sizeof(dbuf), "%d", day);
    graphics_context_set_text_color(ctx, GColorBlack);
    draw_text(ctx, dbuf, CHROME_FONT_KEY,
              GRect(cell.origin.x, cell.origin.y + 2, cell.size.w, cell.size.h - 6),
              GTextOverflowModeFill, GTextAlignmentCenter);

    if (s_cal_month_mask & (1UL << (day - 1))) {
      graphics_context_set_fill_color(ctx, GColorBlueMoon);
      graphics_fill_circle(ctx, GPoint(cell.origin.x + cell.size.w / 2,
                                       cell.origin.y + cell.size.h - 4), 2);
    }
  }

  if (!s_cal_have_data) {
    graphics_context_set_text_color(ctx, GColorDarkGray);
    draw_text(ctx, "Loading…", CHROME_FONT_KEY, GRect(0, h - 18, w, 16),
              GTextOverflowModeFill, GTextAlignmentCenter);
  }
}

static void calendar_render(void) {
  if (s_calendar_layer) {
    layer_mark_dirty(s_calendar_layer);
  }
}

static void calendar_open_selected_day(void) {
  int year, month0;
  calendar_shown_month(&year, &month0);
  char date[11];
  snprintf(date, sizeof(date), "%04d-%02d-%02d", year, month0 + 1, s_cal_selected_day);
  s_browse_mode = BROWSE_CALENDAR_DAY;
  push_browse_window(date);
}

static void calendar_day_prev_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_cal_selected_day > 1) {
    s_cal_selected_day--;
    calendar_render();
  }
}

static void calendar_day_next_handler(ClickRecognizerRef recognizer, void *context) {
  int year, month0;
  calendar_shown_month(&year, &month0);
  if (s_cal_selected_day < cal_days_in_month(year, month0)) {
    s_cal_selected_day++;
    calendar_render();
  }
}

static void calendar_month_prev_handler(ClickRecognizerRef recognizer, void *context) {
  s_cal_month_offset--;
  s_cal_selected_day = 1;
  calendar_render();
  request_calendar_month();
}

static void calendar_month_next_handler(ClickRecognizerRef recognizer, void *context) {
  s_cal_month_offset++;
  s_cal_selected_day = 1;
  calendar_render();
  request_calendar_month();
}

static void calendar_select_handler(ClickRecognizerRef recognizer, void *context) {
  calendar_open_selected_day();
}

static void calendar_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, calendar_day_prev_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, calendar_day_next_handler);
  window_long_click_subscribe(BUTTON_ID_UP, 0, calendar_month_prev_handler, NULL);
  window_long_click_subscribe(BUTTON_ID_DOWN, 0, calendar_month_next_handler, NULL);
  window_single_click_subscribe(BUTTON_ID_SELECT, calendar_select_handler);
}

static void calendar_window_load(Window *window) {
  Layer *window_layer;
  GRect content_bounds = window_chrome(window, &s_calendar_status_bar, &window_layer);
  s_calendar_layer = layer_create(content_bounds);
  layer_set_update_proc(s_calendar_layer, calendar_content_update_proc);
  layer_add_child(window_layer, s_calendar_layer);
  window_set_click_config_provider(window, calendar_click_config_provider);

  s_cal_month_offset = 0;
  time_t now = time(NULL);
  s_cal_selected_day = localtime(&now)->tm_mday;
  s_cal_have_data = false;
  s_cal_month_mask = 0;
  request_calendar_month();
}

static void calendar_window_unload(Window *window) {
  layer_destroy(s_calendar_layer);
  s_calendar_layer = NULL;
  status_bar_layer_destroy(s_calendar_status_bar);
  s_calendar_status_bar = NULL;
}

static void push_calendar_window(void) {
  if (!s_calendar_window) {
    s_calendar_window = window_create();
    window_set_window_handlers(s_calendar_window, (WindowHandlers) {
      .load = calendar_window_load,
      .unload = calendar_window_unload,
    });
  }
  window_stack_push(s_calendar_window, true);
}

// Answers MSG_CAL_MONTH_DATA. The offset check drops a reply for a month
// we've since navigated away from (a fast run of long-Up/long-Down can fire
// several requests before the first reply lands).
static void handle_calendar_month_data(DictionaryIterator *it) {
  int offset = tuple_int(it, KEY_CAL_MONTH_OFFSET, s_cal_month_offset);
  if (offset != s_cal_month_offset) {
    return;
  }
  Tuple *mask_tuple = dict_find(it, KEY_CAL_MONTH_MASK);
  s_cal_month_mask = mask_tuple ? (uint32_t)mask_tuple->value->int32 : 0;
  s_cal_have_data = true;
  calendar_render();
}
#endif
