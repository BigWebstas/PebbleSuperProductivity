// Interactive checklist built from a task note's "- [ ]" lines.
// Textually #included by main.c (one translation unit) - not compiled alone.

// ---------- notes checklist ----------
// The "- [ ]" / "- [x]" lines in a task's notes markdown, shown as a tickable
// list. Reached with Select from the notes window (which sits below this in the
// stack, keeping s_notes_full_text alive) when the fetched notes have any.
// A toggle flips the row locally and sends MSG_TASK_TOGGLE_CHECK (index = the
// row's position among checklist lines); the phone rewrites that one line and
// pushes a plain updateTask. On close, a re-fetch refreshes the notes text.
#define MAX_CHECKLIST 32
#define CHECKLIST_LABELS_CAP 1024
// offsets/lengths into the <=1024-byte s_checklist_labels buffer - uint16 is
// ample and halves the array's .bss.
typedef struct { uint16_t label_off; bool checked; } ChecklistItem;
static ChecklistItem s_checklist[MAX_CHECKLIST];
static int s_checklist_count = 0;
// malloc'd only while the checklist window is open - basalt heap has no room to
// spare it permanently.
static char *s_checklist_labels = NULL;
static int s_checklist_labels_len = 0;
static bool s_checklist_dirty = false;
static Window *s_checklist_window = NULL;
static MenuLayer *s_checklist_menu = NULL;
static StatusBarLayer *s_checklist_status_bar = NULL;
#ifdef PBL_PLATFORM_EMERY
// The just-ticked row's checkmark strokes itself on over CHECKLIST_CHECK_MS.
#define CHECKLIST_CHECK_MS 240
#define CHECKLIST_CHECK_STEP_MS 40
static AppTimer *s_checklist_anim_timer = NULL;
static int s_checklist_anim_row = -1;
static int s_checklist_anim_tick = 0;

static void checklist_anim_cb(void *data) {
  s_checklist_anim_timer = NULL;
  s_checklist_anim_tick++;
  if (s_checklist_anim_tick * CHECKLIST_CHECK_STEP_MS < CHECKLIST_CHECK_MS) {
    s_checklist_anim_timer = app_timer_register(CHECKLIST_CHECK_STEP_MS, checklist_anim_cb, NULL);
  } else {
    s_checklist_anim_row = -1;
  }
  if (s_checklist_menu) {
    layer_mark_dirty(menu_layer_get_layer(s_checklist_menu));
  }
}

static void begin_checklist_anim(int row) {
  if (s_checklist_anim_timer) {
    app_timer_cancel(s_checklist_anim_timer);
  }
  s_checklist_anim_row = row;
  s_checklist_anim_tick = 0;
  s_checklist_anim_timer = app_timer_register(CHECKLIST_CHECK_STEP_MS, checklist_anim_cb, NULL);
}
#endif

// Rebuild s_checklist from s_notes_full_text. A checklist line is optional
// indent, "-" or "*", " [", one of " xX", "]", then the label. Labels are
// snapshotted into s_checklist_labels so a later notes re-fetch can't dangle
// them.
static void parse_checklist(void) {
  s_checklist_count = 0;
  s_checklist_labels_len = 0;
  const char *buf = s_notes_full_text;
  if (!buf || !s_checklist_labels) {
    return;
  }
  const char *p = buf;
  while (*p && s_checklist_count < MAX_CHECKLIST) {
    const char *q = p;
    while (*q == ' ' || *q == '\t') {
      q++;
    }
    if ((q[0] == '-' || q[0] == '*') && q[1] == ' ' && q[2] == '[' && q[3] &&
        (q[3] == ' ' || q[3] == 'x' || q[3] == 'X') && q[4] == ']') {
      const char *lbl = q + 5;
      if (*lbl == ' ') {
        lbl++;
      }
      const char *le = lbl;
      while (*le && *le != '\n') {
        le++;
      }
      int len = (int)(le - lbl);
      if (len > CHECKLIST_LABELS_CAP - 1 - s_checklist_labels_len) {
        len = CHECKLIST_LABELS_CAP - 1 - s_checklist_labels_len;
      }
      if (len < 0) {
        len = 0;
      }
      ChecklistItem *it = &s_checklist[s_checklist_count++];
      it->label_off = s_checklist_labels_len;
      it->checked = (q[3] == 'x' || q[3] == 'X');
      memcpy(s_checklist_labels + s_checklist_labels_len, lbl, (size_t)len);
      s_checklist_labels_len += len;
      s_checklist_labels[s_checklist_labels_len++] = '\0';
    }
    while (*p && *p != '\n') {
      p++;
    }
    if (*p == '\n') {
      p++;
    }
  }
}

static uint16_t checklist_num_rows(MenuLayer *ml, uint16_t section, void *ctx) {
  return (uint16_t)s_checklist_count;
}

static void checklist_draw_row(GContext *ctx, const Layer *cell, MenuIndex *idx, void *c) {
  if ((int)idx->row >= s_checklist_count || !s_checklist_labels) {
    return;
  }
  ChecklistItem *it = &s_checklist[idx->row];
  GRect b = layer_get_bounds(cell);
  bool sel = menu_layer_get_selected_index(s_checklist_menu).row == idx->row;
  GColor fg = sel ? GColorWhite : GColorBlack;
  GRect box = GRect(b.origin.x + 6, b.origin.y + (b.size.h - 16) / 2, 16, 16);
  graphics_context_set_stroke_color(ctx, fg);
  graphics_draw_rect(ctx, box);
  if (it->checked) {
#ifdef PBL_PLATFORM_EMERY
    // A checkmark from two lines; strokes itself on for the row just ticked.
    GPoint a = GPoint(box.origin.x + 3, box.origin.y + 8);
    GPoint elbow = GPoint(box.origin.x + 6, box.origin.y + 12);
    GPoint e = GPoint(box.origin.x + 13, box.origin.y + 3);
    int p = 1000; // per-mille progress; full unless this row is animating
    if (s_checklist_anim_row == (int)idx->row) {
      p = s_checklist_anim_tick * CHECKLIST_CHECK_STEP_MS * 1000 / CHECKLIST_CHECK_MS;
      if (p > 1000) {
        p = 1000;
      }
    }
    graphics_context_set_stroke_width(ctx, 2);
    int leg1 = 400; // short leg first
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
#else
    graphics_context_set_fill_color(ctx, fg);
    graphics_fill_rect(ctx, GRect(box.origin.x + 4, box.origin.y + 4, 8, 8), 0, GCornerNone);
#endif
  }
  graphics_context_set_text_color(ctx, fg);
  GRect tb = GRect(box.origin.x + 26, b.origin.y, b.size.w - (box.origin.x + 26) - 4, b.size.h);
  draw_text(ctx, s_checklist_labels + it->label_off, FONT_KEY_GOTHIC_18,
            tb, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
}

static void checklist_select(MenuLayer *ml, MenuIndex *idx, void *c) {
  if ((int)idx->row >= s_checklist_count) {
    return;
  }
  backlight_touch();
  ChecklistItem *it = &s_checklist[idx->row];
  it->checked = !it->checked;
  s_checklist_dirty = true;
#ifdef PBL_PLATFORM_EMERY
  if (it->checked) {
    begin_checklist_anim(idx->row);
  } else if (s_checklist_anim_row == (int)idx->row) {
    s_checklist_anim_row = -1; // un-ticked mid-stroke
  }
#endif
  menu_layer_reload_data(s_checklist_menu);
  // s_notes_overlay_subject_id is the task id - try_open_checklist bars the
  // project-notes case.
  begin_send(MSG_TASK_TOGGLE_CHECK, s_notes_overlay_subject_id, NULL,
             ((int32_t)idx->row << 1) | (it->checked ? 1 : 0));
}

static void checklist_window_load(Window *window) {
  Layer *wl;
  GRect cb = window_chrome(window, &s_checklist_status_bar, &wl);
  s_checklist_menu = menu_layer_create(cb);
  menu_layer_set_callbacks(s_checklist_menu, NULL, (MenuLayerCallbacks) {
    .get_num_rows = checklist_num_rows,
    .draw_row = checklist_draw_row,
    .select_click = checklist_select,
  });
  menu_layer_set_normal_colors(s_checklist_menu, GColorWhite, GColorBlack);
  menu_layer_set_highlight_colors(s_checklist_menu, GColorVividCerulean, GColorWhite);
  menu_layer_set_click_config_onto_window(s_checklist_menu, window);
  layer_add_child(wl, menu_layer_get_layer(s_checklist_menu));
}

static void checklist_window_unload(Window *window) {
#ifdef PBL_PLATFORM_EMERY
  if (s_checklist_anim_timer) {
    app_timer_cancel(s_checklist_anim_timer);
    s_checklist_anim_timer = NULL;
  }
  s_checklist_anim_row = -1;
#endif
  menu_layer_destroy(s_checklist_menu);
  s_checklist_menu = NULL;
  status_bar_layer_destroy(s_checklist_status_bar);
  free(s_checklist_labels);
  s_checklist_labels = NULL;
  s_checklist_count = 0;
  // Re-pull the notes so the text under us reflects the toggles (the phone
  // rewrote the markdown). Only if something changed.
  if (s_checklist_dirty && s_notes_overlay_active && !s_notes_overlay_is_project) {
    s_checklist_dirty = false;
    reset_notes_full_buffer();
    s_notes_display_text = NOTES_LOADING_TEXT;
    s_notes_is_loading = true;
    request_notes_full(s_notes_overlay_subject_id, false);
    render_notes_overlay_content();
  }
}

static bool try_open_checklist(void) {
  if (s_notes_overlay_is_project || s_notes_is_loading || s_checklist_labels) {
    return false;
  }
  s_checklist_labels = malloc(CHECKLIST_LABELS_CAP);
  if (!s_checklist_labels) {
    return false;
  }
  parse_checklist();
  if (s_checklist_count == 0) {
    free(s_checklist_labels);
    s_checklist_labels = NULL;
    return false;
  }
  s_checklist_dirty = false;
  if (!s_checklist_window) {
    s_checklist_window = window_create();
    window_set_window_handlers(s_checklist_window, (WindowHandlers) {
      .load = checklist_window_load,
      .unload = checklist_window_unload,
    });
  }
  window_stack_push(s_checklist_window, true);
  return true;
}
