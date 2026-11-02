// Notes window and touch navigation (non-aplite).
// Textually #included by main.c (one translation unit) - not compiled alone.

// Select dismisses the notes window; Back also does, for free, via Pebble's
// default pop behavior.
static void notes_window_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  // A task note with "- [ ]" lines: Select opens the interactive checklist.
  // Otherwise it dismisses (Back always dismisses).
  if (try_open_checklist()) {
    return;
  }
  hide_notes_overlay();
}

// Long-select dictates text to append to the currently-shown task's notes
// - see start_note_append_dictation.
static void notes_window_select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  start_note_append_dictation();
}

#if defined(PBL_TOUCH)
// --- Touch navigation (emery and any other PBL_TOUCH platform) ---
//
// Hybrid model. The system touch-nav bridge (app_touch_navigation_enable)
// handles most of it: a swipe scrolls, a tap on a row moves the MenuLayer
// highlight there and is delivered as a SELECT single-click.
//
// This raw touch_service handler runs alongside the bridge and adds what the
// bridge can't:
//   - long-press -> the selected row's long-click (tracking, Finish Day,
//     habit -1, dictate a note); no point->row mapping needed (MenuLayer
//     has no API), it just acts on whatever is selected.
//   - left-swipe on the main list -> move the selected task to tomorrow
//     (see begin_pending_reschedule; the Down button long-press does the same).
//   - tap -> SELECT-ONLY: the bridge's synthesised SELECT click is swallowed
//     (arm_tap_select_guard) so a tap just selects the row, it never toggles
//     it done or bumps a habit. Those stay button-only actions.
// It does NOT handle swipe-right = Back (the bridge already does; a second pop
// double-popped out of the app) or vertical drags (the bridge's scrolling).
//
// The long-press acts on the MenuLayer's selected row, like the physical
// long-click - no need to map a touch point to a row (MenuLayer has no API).
//
// HARDWARE STATE (first-gen Time 2, firmware ~4.33): the touch driver reports a
// large drift toward screen centre on edge touches (~130-170px over ~200ms), so
// the bridge reads an edge tap as a swipe and the long-press slop trips.
// Centre taps and holds are clean. This is why the pairing setting defaults
// OFF - opt-in until a firmware fix lands. TOUCH_SLOP_PX is generous to ride
// out the smaller drift on a near-centre hold.

#define TOUCH_LONGPRESS_MS 700
#define TOUCH_SLOP_PX 25  // movement past this cancels the long-press / is not a tap
// A decisive leftward drag on the task list = move the selected task to
// tomorrow (the button long-press Down does the same). Right/vertical drags
// stay the bridge's (Back / scroll). Gated on a shallow dy so a diagonal
// scroll doesn't count. Kept modest (a real flick clears it in a frame or two)
// because the old 60px value plus the slop-disarm below meant a swipe that
// began even slightly vertical never registered.
#define TOUCH_SWIPE_PX 45

static AppTimer *s_touch_longpress_timer = NULL;
static GPoint s_touch_down_point;
static bool s_touch_armed = false;       // a gesture is in progress (armed at Touchdown)
static bool s_touch_moved = false;       // finger has left the tap zone (scroll / swipe)
static bool s_touch_swipe_fired = false; // the left-swipe already handled this gesture
// s_tap_moved_sel / s_touch_longpress_fired are declared up by s_touch_nav_enabled.

#ifdef PBL_PLATFORM_EMERY
// Scroll-pip drag (see scroll_pip_update_proc): a touch that starts in the
// right-edge PIP_TOUCH_ZONE_PX and a list long enough to show the pip enters a
// scrub - PositionUpdates map the finger's Y straight onto a list row.
#define PIP_MIN_ROWS 7
#define PIP_TOUCH_ZONE_PX 24
static bool s_touch_pip_scrub = false;

// Total rows across every section of the main list menu.
static int pip_total_rows(void) {
  if (!s_menu_layer || s_task_count == 0) {
    return 0;
  }
  uint16_t nsec = menu_get_num_sections(s_menu_layer, NULL);
  int total = 0;
  for (uint16_t s = 0; s < nsec; s++) {
    total += menu_get_num_rows(s_menu_layer, s, NULL);
  }
  return total;
}

// Map a screen-space Y to a flat list row and select it.
static void pip_scrub_to(int16_t screen_y) {
  int total = pip_total_rows();
  if (total < PIP_MIN_ROWS || !s_scroll_pip_layer) {
    return;
  }
  int16_t th = layer_get_bounds(s_scroll_pip_layer).size.h - 8;
  int rel = screen_y - (STATUS_BAR_LAYER_HEIGHT + 4);
  if (rel < 0) {
    rel = 0;
  } else if (rel > th) {
    rel = th;
  }
  int flat = th > 0 ? rel * (total - 1) / th : 0;
  uint16_t nsec = menu_get_num_sections(s_menu_layer, NULL);
  for (uint16_t s = 0; s < nsec; s++) {
    int n = menu_get_num_rows(s_menu_layer, s, NULL);
    if (flat < n) {
      menu_layer_set_selected_index(s_menu_layer, MenuIndex(s, (uint16_t)flat),
                                    MenuRowAlignCenter, false);
      return;
    }
    flat -= n;
  }
}
#endif

static void touch_longpress_timer_cancel(void) {
  if (s_touch_longpress_timer) {
    app_timer_cancel(s_touch_longpress_timer);
    s_touch_longpress_timer = NULL;
  }
}

// The guard window: any menu SELECT click that lands within this long of a
// touch event is the bridge's synthesised tap-click, not a physical button
// press. Generous because the bridge can emit the click slightly before OR
// after the raw Liftoff, and the Time-2 driver's latency is not tight.
#define TAP_GUARD_MS 450

static void tap_guard_timer_cb(void *data) {
  s_tap_guard_timer = NULL;
  s_ignore_next_menu_select = false;
}

// Arm / refresh the guard. Called on every touch event that could precede a
// synthesised SELECT (Touchdown, Liftoff, long-press fire) - NOT gated on a
// motion "was it a tap" test, which is unreliable given the driver drift the
// HARDWARE STATE note describes. A scroll / swipe / long-press just leaves the
// guard to expire unused.
static void arm_tap_select_guard(void) {
  s_ignore_next_menu_select = true;
  if (s_tap_guard_timer) {
    app_timer_cancel(s_tap_guard_timer);
  }
  s_tap_guard_timer = app_timer_register(TAP_GUARD_MS, tap_guard_timer_cb, NULL);
}

static void clear_tap_select_guard(void) {
  s_ignore_next_menu_select = false;
  if (s_tap_guard_timer) {
    app_timer_cancel(s_tap_guard_timer);
    s_tap_guard_timer = NULL;
  }
}

// True (and disarms) if a touch-synthesised SELECT is what's being handled -
// the caller should treat this click as "select the row only", not activate it.
static bool consume_tap_select_guard(void) {
  if (!s_ignore_next_menu_select) {
    return false;
  }
  clear_tap_select_guard();
  return true;
}

static void touch_longpress_fire(void *data) {
  s_touch_longpress_timer = NULL;
  s_touch_armed = false;  // consumed - the eventual liftoff does nothing more
  s_touch_longpress_fired = true;  // the trailing synthesised SELECT is a no-op
  arm_tap_select_guard();  // swallow any SELECT the bridge emits on the release
  backlight_touch();
  vibes_short_pulse();  // the only "it registered" cue before the action lands
  Window *top = window_stack_get_top_window();
  if (top == s_notes_window) {
    start_note_append_dictation();
  } else if (top == s_habits_window && s_habits_menu_layer) {
    MenuIndex idx = menu_layer_get_selected_index(s_habits_menu_layer);
    habits_menu_select_long_click(s_habits_menu_layer, &idx, NULL);
  } else if (top == s_main_window) {
    MenuIndex idx = menu_layer_get_selected_index(s_menu_layer);
    menu_select_long_click(s_menu_layer, &idx, NULL);
  }
}

// Left-swipe test: a decisive leftward drag with a shallow vertical component,
// on the main list only.
static bool touch_is_left_swipe(int dx, int dy) {
  int adx = dx < 0 ? -dx : dx;
  int ady = dy < 0 ? -dy : dy;
  return dx <= -TOUCH_SWIPE_PX && ady * 2 <= adx;
}

// Right-swipe test (mirror of the above) - used only on windows built from a
// bare custom Layer (no MenuLayer/ScrollLayer), where the system bridge's own
// swipe-right-Back doesn't reach (confirmed on hardware for the live tracking
// window; the calendar month grid is the same shape - a raw Layer, not a
// ScrollLayer/MenuLayer - so it gets the same treatment). Everywhere else
// (MenuLayer and ScrollLayer windows) the bridge already handles it, and a
// handler of our own there double-popped (see the touch-nav memory) - this
// stays scoped to those two windows to avoid repeating that.
static bool touch_is_right_swipe(int dx, int dy) {
  int adx = dx < 0 ? -dx : dx;
  int ady = dy < 0 ? -dy : dy;
  return dx >= TOUCH_SWIPE_PX && ady * 2 <= adx;
}

// Defined with the rest of the live window, further down the file.
static void live_window_back_click_handler(ClickRecognizerRef recognizer, void *context);

// Pops whichever of the two bridge-unreachable windows is on top, if either
// is; returns whether it did, so a right-swipe on any other window is a
// no-op (the bridge already owns Back there).
static bool touch_swipe_back(void) {
  Window *top = window_stack_get_top_window();
  if (top == s_live_window) {
    live_window_back_click_handler(NULL, NULL);
    return true;
  }
#ifdef PBL_PLATFORM_EMERY
  if (top == s_calendar_window) {
    window_stack_pop(true);
    return true;
  }
#endif
  return false;
}

static void touch_handler(const TouchEvent *event, void *context) {
  switch (event->type) {
  case TouchEvent_Touchdown:
    touch_longpress_timer_cancel();
    s_touch_moved = false;
    s_touch_swipe_fired = false;
    s_tap_moved_sel = false;
    s_touch_longpress_fired = false;
#ifdef PBL_PLATFORM_EMERY
    s_touch_pip_scrub = false;
#endif
    // non_navigational: contact without the watch being woken first - don't arm.
    s_touch_armed = !event->non_navigational;
    if (!s_touch_armed) {
      clear_tap_select_guard();
      break;
    }
    s_touch_down_point = GPoint(event->x, event->y);
    s_touch_longpress_timer = app_timer_register(TOUCH_LONGPRESS_MS, touch_longpress_fire, NULL);
    // Arm now - the bridge can synthesise its tap SELECT click before the raw
    // Liftoff even reaches us. Re-armed on Liftoff to cover a late one.
    arm_tap_select_guard();
#ifdef PBL_PLATFORM_EMERY
    // A touch on the right-edge scrollbar zone (long lists only) starts a scrub.
    if (window_stack_get_top_window() == s_main_window && s_scroll_pip_layer &&
        pip_total_rows() >= PIP_MIN_ROWS &&
        event->x >= layer_get_bounds(s_scroll_pip_layer).size.w - PIP_TOUCH_ZONE_PX) {
      s_touch_pip_scrub = true;
      s_tap_moved_sel = true;  // its trailing SELECT must not activate a row
      touch_longpress_timer_cancel();
      pip_scrub_to(event->y);
    }
#endif
    break;

  case TouchEvent_PositionUpdate: {
#ifdef PBL_PLATFORM_EMERY
    if (s_touch_pip_scrub) {
      pip_scrub_to(event->y);
      break;
    }
#endif
    if (!s_touch_armed || s_touch_swipe_fired) {
      break;
    }
    int dx = event->x - s_touch_down_point.x;
    int dy = event->y - s_touch_down_point.y;
    int adx = dx < 0 ? -dx : dx;
    int ady = dy < 0 ? -dy : dy;
    if (adx > TOUCH_SLOP_PX || ady > TOUCH_SLOP_PX) {
      s_touch_moved = true;
      touch_longpress_timer_cancel();  // a moving finger isn't a long-press
    }
    // Keep evaluating dx for the whole drag - the old code disarmed at the slop,
    // so a swipe that started even slightly vertical could never be recognised.
    // s_tap_moved_sel too: a swipe's trailing synthesised SELECT must not
    // land on the tap-to-activate branch in menu_select_click.
    if (touch_is_left_swipe(dx, dy) && window_stack_get_top_window() == s_main_window) {
      s_touch_swipe_fired = true;
      s_tap_moved_sel = true;
      begin_pending_reschedule(RESCHEDULE_TOMORROW);
    }
    // Right-swipe Back is the system bridge's job everywhere else - a handler
    // of our own double-popped elsewhere (see the touch-nav memory). The live
    // tracking and calendar windows are the bare-Layer places the bridge's
    // swipe-back doesn't reach (confirmed on hardware for live tracking), so
    // they alone get a swipe-right of our own.
    if (touch_is_right_swipe(dx, dy) && touch_swipe_back()) {
      s_touch_swipe_fired = true;
    }
    break;
  }

  case TouchEvent_Liftoff: {
    touch_longpress_timer_cancel();
#ifdef PBL_PLATFORM_EMERY
    if (s_touch_pip_scrub) {
      s_touch_pip_scrub = false;
      arm_tap_select_guard();  // swallow the bridge's release SELECT
      s_touch_armed = false;
      break;
    }
#endif
    if (s_touch_armed && !s_touch_swipe_fired) {
      int dx = event->x - s_touch_down_point.x;
      int dy = event->y - s_touch_down_point.y;
      // A fast flick can arrive as Touchdown -> Liftoff with no PositionUpdate
      // between - catch it from the endpoint delta.
      if (touch_is_left_swipe(dx, dy) && window_stack_get_top_window() == s_main_window) {
        begin_pending_reschedule(RESCHEDULE_TOMORROW);
        s_touch_swipe_fired = true;
        s_tap_moved_sel = true;
      } else if (touch_is_right_swipe(dx, dy) && touch_swipe_back()) {
        s_touch_swipe_fired = true;
      }
    }
    // Re-arm the tap guard so a SELECT the bridge emits just after the release
    // is swallowed too (tap = select-only; and after a swipe, a stray SELECT
    // must not reach the pending-reschedule cancel path).
    arm_tap_select_guard();
    s_touch_armed = false;
    s_touch_moved = false;
    s_touch_swipe_fired = false;
    break;
  }
  }
}

static void apply_touch_nav(void) {
  app_touch_navigation_enable(s_touch_nav_enabled);
  if (s_touch_nav_enabled) {
    touch_service_subscribe(touch_handler, NULL);
  } else {
    touch_service_unsubscribe();
    touch_longpress_timer_cancel();
    clear_tap_select_guard();
    s_touch_armed = false;
    s_touch_moved = false;
    s_touch_swipe_fired = false;
    s_tap_moved_sel = false;
    s_touch_longpress_fired = false;
  }
}
#endif  // PBL_TOUCH

// Installed onto the ScrollLayer (not the window) via
// scroll_layer_set_click_config_onto_window, which wires UP/DOWN to scrolling
// then calls this for SELECT. On touch builds the bridge also scrolls by finger.
// Estimate / deadline / tags live in the task action menu (long-Select a task
// row), not on hidden gestures here.
static void notes_window_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, notes_window_select_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 0, notes_window_select_long_click_handler, NULL);
}

// Fills the layer with NOTES_TAGS_BG_COLOR and draws the bold "Tags:" label
// then the tag names below it. Two graphics_draw_text calls since one can't mix
// font weights. A no-op when the frame is zero-height (a project subject).
static void notes_tags_layer_draw(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  fill_bg(ctx, bounds, NOTES_TAGS_BG_COLOR);
  graphics_context_set_text_color(ctx, GColorBlack);

  int16_t content_w = bounds.size.w - NOTES_TAGS_PADDING_X * 2;
  int16_t label_h, names_h;
  measure_notes_tags_parts(content_w, &label_h, &names_h);

  GRect label_rect = GRect(NOTES_TAGS_PADDING_X, NOTES_TAGS_PADDING_Y, content_w, label_h);
  draw_text(ctx, NOTES_TAGS_LABEL, NOTES_LABEL_FONT_KEY, label_rect, GTextOverflowModeWordWrap, GTextAlignmentLeft);

  GRect names_rect = GRect(NOTES_TAGS_PADDING_X, NOTES_TAGS_PADDING_Y + label_h, content_w, names_h);
  draw_text(ctx, notes_tags_display_line(), NOTES_BODY_FONT_KEY, names_rect, GTextOverflowModeWordWrap, GTextAlignmentLeft);
}

static void notes_window_load(Window *window) {
  Layer *window_layer;
  GRect content_bounds = window_chrome(window, &s_notes_status_bar, &window_layer);
  s_notes_content_bounds = content_bounds;

  // Fixed header added to window_layer, not the ScrollLayer, so it stays put
  // while the body scrolls. Provisional zero-height frame -
  // render_notes_overlay_content sizes it and slices the ScrollLayer below it.
  s_notes_tags_layer = layer_create(GRect(content_bounds.origin.x, content_bounds.origin.y,
                                           content_bounds.size.w, 0));
  layer_set_update_proc(s_notes_tags_layer, notes_tags_layer_draw);
  layer_add_child(window_layer, s_notes_tags_layer);

  // Provisional full-content frame - render_notes_overlay_content shrinks it
  // to make room for the tags header.
  s_notes_scroll_layer = scroll_layer_create(content_bounds);
  scroll_layer_set_content_size(s_notes_scroll_layer, content_bounds.size);
  scroll_layer_set_click_config_onto_window(s_notes_scroll_layer, window);
  scroll_layer_set_callbacks(s_notes_scroll_layer, (ScrollLayerCallbacks) {
    .click_config_provider = notes_window_click_config_provider,
  });

  // Left-aligned, top-anchored body text (unlike s_error_layer's centered
  // status text), smaller font. Created at the viewport size, resized to the
  // real content height by render_notes_overlay_content.
  s_notes_layer = text_layer_create(GRect(0, 0, content_bounds.size.w, content_bounds.size.h));
  text_layer_set_text_alignment(s_notes_layer, GTextAlignmentLeft);
  text_layer_set_font(s_notes_layer, fonts_get_system_font(NOTES_BODY_FONT_KEY));
  text_layer_set_background_color(s_notes_layer, GColorWhite);
  text_layer_set_text_color(s_notes_layer, GColorBlack);
  text_layer_set_overflow_mode(s_notes_layer, GTextOverflowModeWordWrap);
  scroll_layer_add_child(s_notes_scroll_layer, text_layer_get_layer(s_notes_layer));

  layer_add_child(window_layer, scroll_layer_get_layer(s_notes_scroll_layer));

  // s_notes_display_text was set by show_notes_overlay() before the push
  // (usually the loading placeholder - the fetch is still in flight).
  render_notes_overlay_content();
}

static void notes_window_unload(Window *window) {
  layer_destroy(s_notes_tags_layer);
  s_notes_tags_layer = NULL;
  text_layer_destroy(s_notes_layer);
  s_notes_layer = NULL;
  scroll_layer_destroy(s_notes_scroll_layer);
  s_notes_scroll_layer = NULL;
  status_bar_layer_destroy(s_notes_status_bar);
  reset_notes_full_buffer();
  cancel_notes_load_timeout();
  // Cleared here, not in hide_notes_overlay(), so a Back-triggered dismissal
  // (which never calls hide_notes_overlay) clears it the same way.
  s_notes_overlay_active = false;
}

// Created once and reused, like push_habits_window - only its layers are
// rebuilt each visit.
static void push_notes_window(void) {
  if (!s_notes_window) {
    s_notes_window = window_create();
    window_set_window_handlers(s_notes_window, (WindowHandlers) {
      .load = notes_window_load,
      .unload = notes_window_unload,
    });
  }
  window_stack_push(s_notes_window, true);
}
