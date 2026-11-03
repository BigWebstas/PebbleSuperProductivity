// Upcoming (and Notes) page: scrollable future-dated list.
// Textually #included by main.c (one translation unit) - not compiled alone.

// ---------- upcoming window ----------
// Optional section-0 row ("Upcoming"). A read-only scrollable list of every
// task scheduled for a day AFTER today, grouped by day - the phone computes it
// (store.computeUpcoming) and sends the whole thing preformatted in one
// MSG_UPCOMING_DATA: a "\x02"-prefixed day header line before each day's tasks.
// The watch just parses and draws, reusing the Stats page's fonts/metrics.
static Window *s_upcoming_window;
static StatusBarLayer *s_upcoming_status_bar;
static ScrollLayer *s_upcoming_scroll_layer;
static Layer *s_upcoming_content_layer;
static GRect s_upcoming_content_bounds;

static int16_t upcoming_content_height(void) {
  int16_t h = 8;
  if (!s_upcoming_text) {
    return h;
  }
  const char *p = s_upcoming_text;
  bool at_line_start = true;
  for (; *p; p++) {
    if (at_line_start) {
      h += (*p == '\x02') ? STATS_LABEL_H : (*p == '\x03') ? PAGE_SUB_H : STATS_LINE_H;
    }
    at_line_start = (*p == '\n');
  }
  return h;
}

static void upcoming_content_update_proc(Layer *layer, GContext *ctx) {
  GRect b = layer_get_bounds(layer);
  int16_t w = b.size.w;
  fill_bg(ctx, b, GColorWhite);

  if (!s_upcoming_have_data || !s_upcoming_text || s_upcoming_text[0] == '\0') {
    graphics_context_set_text_color(ctx, GColorBlack);
    const char *empty = s_page_mode == PAGE_NOTES ? "No pinned notes"
                        : s_page_mode == PAGE_SEARCH ? "No matches" : "Nothing scheduled";
    draw_text(ctx, s_upcoming_have_data ? empty : "Loading…", STATS_LINE_FONT,
              GRect(STATS_PAD_X, 8, w - STATS_PAD_X * 2, STATS_LINE_H),
              GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
    return;
  }

  const char *p = s_upcoming_text;
  char line[96];
  int16_t y = 4;
  while (*p) {
    const char *nl = strchr(p, '\n');
    size_t len = nl ? (size_t)(nl - p) : strlen(p);
    char kind = (*p == '\x02' || *p == '\x03') ? *p : 0;
    const char *src = kind ? p + 1 : p;
    size_t slen = kind ? (len ? len - 1 : 0) : len;
    if (slen >= sizeof(line)) {
      slen = sizeof(line) - 1;
    }
    memcpy(line, src, slen);
    line[slen] = '\0';
    if (kind == '\x02') {
      int16_t bar_w = header_bar_w(w);
      fill_bg(ctx, GRect(0, y, bar_w, STATS_LABEL_H), GColorBlack);
      if (bar_w >= w) {
        graphics_context_set_text_color(ctx, GColorWhite);
        draw_text(ctx, line, STATS_LABEL_FONT, GRect(STATS_PAD_X, y, w - STATS_PAD_X * 2, STATS_LABEL_H), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
      }
      y += STATS_LABEL_H;
    } else if (kind == '\x03') {
      graphics_context_set_text_color(ctx, GColorDarkGray);
      draw_text(ctx, line, FONT_KEY_GOTHIC_14,
                GRect(STATS_PAD_X + 10, y - 2, w - STATS_PAD_X * 2 - 10, PAGE_SUB_H + 2),
                GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
      y += PAGE_SUB_H;
    } else {
      graphics_context_set_text_color(ctx, GColorBlack);
      draw_text(ctx, line, STATS_LINE_FONT, GRect(STATS_PAD_X, y, w - STATS_PAD_X * 2, STATS_LINE_H), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
      y += STATS_LINE_H;
    }
    if (!nl) {
      break;
    }
    p = nl + 1;
  }
}

static void upcoming_render(void) {
  if (!s_upcoming_content_layer) {
    return;
  }
  int16_t h = s_upcoming_have_data ? upcoming_content_height() : s_upcoming_content_bounds.size.h;
  if (h < s_upcoming_content_bounds.size.h) {
    h = s_upcoming_content_bounds.size.h;
  }
  layer_set_frame(s_upcoming_content_layer, GRect(0, 0, s_upcoming_content_bounds.size.w, h));
  scroll_layer_set_content_size(s_upcoming_scroll_layer, GSize(s_upcoming_content_bounds.size.w, h));
  layer_mark_dirty(s_upcoming_content_layer);
}

static void request_upcoming(void) {
  if (s_page_mode == PAGE_SEARCH) {
    begin_send(MSG_SEARCH_REQUEST, s_search_query, NULL, 0);
    return;
  }
  begin_send(s_page_mode == PAGE_NOTES ? MSG_NOTESPAGE_REQUEST : MSG_UPCOMING_REQUEST, NULL, NULL, 0);
}

// Long-Select on the shared page window - only meaningful in PAGE_NOTES,
// where it dictates an append (see start_notespage_append_dictation). Wraps
// the scroll layer's own click config, same shape as stats_window_click_config_provider.
static ClickConfigProvider s_page_ccp = NULL;

static void page_notes_append_handler(ClickRecognizerRef recognizer, void *context) {
  if (s_page_mode != PAGE_NOTES) {
    return;
  }
  start_notespage_append_dictation();
}

static void page_window_click_config_provider(void *context) {
  if (s_page_ccp) {
    s_page_ccp(context);
  }
  window_long_click_subscribe(BUTTON_ID_SELECT, 0, page_notes_append_handler, NULL);
}

static void upcoming_window_load(Window *window) {
  Layer *window_layer;
  free(s_upcoming_text);
  s_upcoming_text = malloc(PAGE_TEXT_CAP);
  if (s_upcoming_text) {
    s_upcoming_text[0] = '\0';
  }
  s_upcoming_have_data = false;
  s_upcoming_content_bounds = window_chrome(window, &s_upcoming_status_bar, &window_layer);
  s_upcoming_scroll_layer = scroll_layer_create(s_upcoming_content_bounds);
  scroll_layer_set_content_size(s_upcoming_scroll_layer, s_upcoming_content_bounds.size);
  scroll_layer_set_click_config_onto_window(s_upcoming_scroll_layer, window);
  s_page_ccp = window_get_click_config_provider(window);
  window_set_click_config_provider_with_context(window, page_window_click_config_provider,
                                                window_get_click_config_context(window));
  s_upcoming_content_layer = layer_create(GRect(0, 0, s_upcoming_content_bounds.size.w,
                                                s_upcoming_content_bounds.size.h));
  layer_set_update_proc(s_upcoming_content_layer, upcoming_content_update_proc);
  scroll_layer_add_child(s_upcoming_scroll_layer, s_upcoming_content_layer);
  layer_add_child(window_layer, scroll_layer_get_layer(s_upcoming_scroll_layer));
  upcoming_render();
}

static void upcoming_window_unload(Window *window) {
  header_cancel_reveal();
  s_page_ccp = NULL;
  layer_destroy(s_upcoming_content_layer);
  s_upcoming_content_layer = NULL;
  scroll_layer_destroy(s_upcoming_scroll_layer);
  s_upcoming_scroll_layer = NULL;
  status_bar_layer_destroy(s_upcoming_status_bar);
  s_upcoming_status_bar = NULL;
  free(s_upcoming_text);
  s_upcoming_text = NULL;
  s_upcoming_have_data = false;
}

static void push_page_window(PageMode mode) {
  s_page_mode = mode; // window_load allocs + clears s_upcoming_text
  if (!s_upcoming_window) {
    s_upcoming_window = window_create();
    window_set_window_handlers(s_upcoming_window, (WindowHandlers) {
      .load = upcoming_window_load,
      .unload = upcoming_window_unload,
    });
  }
  window_stack_push(s_upcoming_window, true);
  request_upcoming();
}
