#include <pebble.h>

// The .h files are pasted into this one file, so __FILE_NAME__ would embed a
// different name per section in .rodata - bytes emery can't spare. Log under
// "main.c" so there's a single shared string.
#undef APP_LOG
#define APP_LOG(level, fmt, args...) app_log(level, "main.c", __LINE__, fmt, ## args)

// Source map. Everything below is ONE translation unit: the .h files are
// pasted in here in order, so static helpers still inline and the 64 KB emery
// ceiling is measured exactly as before. Order matters (later sections call
// earlier ones; forward-decls.h covers the rest) - don't reorder casually.

// Protocol and data
#include "core/protocol.h"
#include "core/model.h"
#include "ui/metrics.h"
#include "core/persistence.h"
#include "core/forward-decls.h"

// Main list, watch -> phone commands, banners, overlays
#include "ui/main-list.h"
#include "comms/outbound.h"
#include "ui/banners-tracking.h"
#include "ui/overlays.h"

// Phone -> watch
#include "comms/inbound.h"

// Pages
#include "pages/habits.h"
#include "pages/projects-browser.h"

#ifndef PBL_PLATFORM_APLITE
#include "ui/notes-touch.h"
#include "ui/checklist.h"
#include "ui/value-picker.h"
#include "pages/reflect.h"
#include "ui/action-menu.h"
#endif

#ifndef PBL_PLATFORM_APLITE
#include "pages/stats.h"
#include "pages/upcoming.h"
#endif

#include "pages/calendar.h"
#include "pages/schedule.h"
#include "pages/live-focus.h"

// App lifecycle: window setup, minute tick, init/deinit, main()

// ---------- window lifecycle ----------

// MenuLayer owns the whole main-window click config now: UP/DOWN scroll, SELECT
// single (toggle / double-click notes) and long (the per-task action menu, where
// scheduling now lives - it used to be long-press UP/DOWN here).

#ifdef PBL_PLATFORM_EMERY
// A scrollbar on the far right of the task list - a faint track with a red
// thumb whose position tracks the MenuLayer's real scroll viewport (via its
// backing ScrollLayer's content offset/size), not the selected row: a touch
// drag scrolls the list without moving the selection, so a selection-index
// thumb sat frozen mid-drag even though menu_draw_row was marking this layer
// dirty on every row repaint. Hidden when the whole list fits (< PIP_MIN_ROWS).
// Its own overlay layer (s_scroll_pip_layer, declared up by s_menu_layer)
// above s_menu_layer. On a touch watch, dragging the right edge scrubs the
// selection - pip_total_rows / pip_scrub_to are defined up by touch_handler.
static void scroll_pip_update_proc(Layer *layer, GContext *ctx) {
  int total = pip_total_rows();
  if (total < PIP_MIN_ROWS) {
    return;
  }
  ScrollLayer *sl = menu_layer_get_scroll_layer(s_menu_layer);
  int16_t viewport_h = layer_get_bounds(scroll_layer_get_layer(sl)).size.h;
  int16_t max_scroll = scroll_layer_get_content_size(sl).h - viewport_h;
  int16_t scroll_y = -scroll_layer_get_content_offset(sl).y;
  if (scroll_y < 0) {
    scroll_y = 0;
  } else if (max_scroll > 0 && scroll_y > max_scroll) {
    scroll_y = max_scroll;
  }

  GRect b = layer_get_bounds(layer);
  int16_t tx = b.origin.x + b.size.w - 4;
  int16_t top = b.origin.y + 4;
  int16_t th = b.size.h - 8;
  graphics_context_set_fill_color(ctx, GColorLightGray);
  graphics_fill_rect(ctx, GRect(tx + 1, top, 2, th), 1, GCornersAll);
  int thumb = th / 5;
  if (thumb < 14) {
    thumb = 14;
  }
  int ty = top + (max_scroll > 0 ? (th - thumb) * scroll_y / max_scroll : 0);
  graphics_context_set_fill_color(ctx, GColorRed);
  graphics_fill_rect(ctx, GRect(tx, ty, 3, thumb), 1, GCornersAll);
}
#endif

static void window_load(Window *window) {
  Layer *window_layer;
  GRect content_bounds = window_chrome(window, &s_status_bar, &window_layer);

  s_menu_layer = menu_layer_create(content_bounds);
  menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks) {
    .get_num_sections = menu_get_num_sections,
    .get_num_rows = menu_get_num_rows,
    .get_header_height = menu_get_header_height,
    .draw_header = menu_draw_header,
    .draw_row = menu_draw_row,
    .select_click = menu_select_click,
    .select_long_click = menu_select_long_click,
    .selection_changed = menu_selection_changed,
  });
  menu_layer_set_click_config_onto_window(s_menu_layer, window);
  layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
#ifdef PBL_PLATFORM_EMERY
  s_scroll_pip_layer = layer_create(content_bounds);
  layer_set_update_proc(s_scroll_pip_layer, scroll_pip_update_proc);
  layer_add_child(window_layer, s_scroll_pip_layer);
#endif
  // A cached list may already have a selection that needs to scroll.
  refresh_scroll_state(true);

  // Logo in a fixed strip at the bottom of the empty-state area; the text layer
  // gets the space above it.
  #define LOGO_SIZE 50
  #define LOGO_STRIP_HEIGHT 58
  GRect empty_text_bounds = GRect(content_bounds.origin.x, content_bounds.origin.y,
                                   content_bounds.size.w, content_bounds.size.h - LOGO_STRIP_HEIGHT);
  s_empty_layer = make_text_layer(window_layer, empty_text_bounds,
                                  EMPTY_MSG_FONT_KEY, GTextAlignmentCenter);

#ifndef PBL_PLATFORM_APLITE
  // Bottom slice of the text area for s_sync_progress_layer, sized off
  // empty_text_bounds. Hidden except during the initial sync.
  #ifdef PBL_PLATFORM_EMERY
  #define SYNC_PROGRESS_HEIGHT 42
  #else
  #define SYNC_PROGRESS_HEIGHT 36
  #endif
  GRect sync_progress_bounds = GRect(empty_text_bounds.origin.x,
                                      empty_text_bounds.origin.y + empty_text_bounds.size.h - SYNC_PROGRESS_HEIGHT,
                                      empty_text_bounds.size.w, SYNC_PROGRESS_HEIGHT);
  s_sync_progress_layer = make_text_layer(window_layer, sync_progress_bounds,
                                          CHROME_FONT_KEY, GTextAlignmentCenter);
  layer_set_hidden(text_layer_get_layer(s_sync_progress_layer), true);
#endif

  GRect logo_bounds = GRect(content_bounds.origin.x + (content_bounds.size.w - LOGO_SIZE) / 2,
                             content_bounds.origin.y + content_bounds.size.h - LOGO_STRIP_HEIGHT,
                             LOGO_SIZE, LOGO_SIZE);
  s_logo_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_LOGO_LARGE);
  s_logo_layer = bitmap_layer_create(logo_bounds);
  bitmap_layer_set_bitmap(s_logo_layer, s_logo_bitmap);
  bitmap_layer_set_compositing_mode(s_logo_layer, GCompOpSet);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_logo_layer));

  // Row icons for the Resync/Habits rows - loaded once, not per-draw.
  s_check_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MENU_ICON);
  s_check_white_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_CHECK_WHITE);
  s_heart_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HEART_CHECK);
  s_heart_white_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HEART_CHECK_WHITE);
#ifndef PBL_PLATFORM_APLITE
  s_project_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PROJECT);
  s_project_white_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_PROJECT_WHITE);
  s_stats_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_STATS);
  s_stats_white_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_STATS_WHITE);
  s_inbox_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_INBOX);
  s_inbox_white_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_INBOX_WHITE);
#endif

  // Add Task row + dictation session - mic platforms only.
#ifndef PBL_PLATFORM_APLITE
  s_mic_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MICROPHONE);
  s_mic_white_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MICROPHONE_WHITE);
  s_dictation_session = dictation_session_create(MAX_TITLE_LEN, dictation_status_callback, NULL);
  // Let the user review/retry the transcription before it's sent.
  dictation_session_enable_confirmation(s_dictation_session, true);
#endif

  // Full content_bounds, added last so it draws on top of every other layer.
  // Hidden by default; only show_error_overlay() reveals it.
  s_error_layer = make_text_layer(window_layer, content_bounds, TITLE_FONT_KEY, GTextAlignmentCenter);
  text_layer_set_background_color(s_error_layer, GColorRed);
  // White, not black - better contrast on red, and on aplite GColorRed may
  // reduce to black, which would make black text invisible.
  text_layer_set_text_color(s_error_layer, GColorWhite);
  text_layer_set_overflow_mode(s_error_layer, GTextOverflowModeWordWrap);
  layer_set_hidden(text_layer_get_layer(s_error_layer), true);

#ifndef PBL_PLATFORM_APLITE
  // Over-estimate banner - a red strip across the top of the list, same
  // GColorRed + white-bold as s_error_layer. Hidden until a tracked task first
  // crosses its estimate.
  GRect overtime_bounds = GRect(content_bounds.origin.x, content_bounds.origin.y,
                                 content_bounds.size.w, OVERTIME_BANNER_HEIGHT);
  s_overtime_banner_layer = make_text_layer(window_layer, overtime_bounds,
                                            TITLE_FONT_KEY, GTextAlignmentCenter);
  text_layer_set_background_color(s_overtime_banner_layer, GColorRed);
  text_layer_set_text_color(s_overtime_banner_layer, GColorWhite);
  text_layer_set_overflow_mode(s_overtime_banner_layer, GTextOverflowModeTrailingEllipsis);
  layer_set_hidden(text_layer_get_layer(s_overtime_banner_layer), true);
#endif

  update_empty_layer();
  request_sync();

  // Resume the live-ticking redraw if a session was running at last close (the
  // elapsed total comes from the persisted start timestamp).
  if (s_tracking_task_id[0] != '\0') {
    start_tracking_tick();
#ifndef PBL_PLATFORM_APLITE
    // Re-announce the resumed session so the phone can broadcast it again.
    time_t resumed_s = time(NULL) - s_tracking_start_epoch;
    send_track_time_start(s_tracking_task_id, resumed_s > 0 ? (int32_t)resumed_s * 1000 : 0);
#endif
  }
}

// Returning to the list from a sub-window (the action menu especially) lands
// here. Sub-windows change the list underneath them - a task gets marked done,
// tracking stops - and reload their target inline, but a reload issued while
// the sub-window is still animating out gets swallowed by the transition and
// the list shows a stale frame until the next button press. Re-issuing it on
// .appear, after the transition settles, is the reliable repaint.
static void window_appear(Window *window) {
  if (!s_menu_layer) {
    return;
  }
  menu_layer_reload_data(s_menu_layer);
  refresh_scroll_state(true);
#ifdef PBL_PLATFORM_EMERY
  if (s_scroll_pip_layer) {
    layer_mark_dirty(s_scroll_pip_layer);
  }
#endif
}

static void window_unload(Window *window) {
  stop_scroll_timer();
  // NOT stopping tracking here - only this window's redraw timer, since
  // s_menu_layer is about to be destroyed.
  stop_tracking_tick();
#ifndef PBL_PLATFORM_APLITE
  hide_overtime_banner(); // cancels its auto-dismiss timer
  cancel_unpin_timer();   // the pinned-section grace timer
#endif
  stop_syncing_animation();
#ifdef PBL_PLATFORM_EMERY
  layer_destroy(s_scroll_pip_layer);
  s_scroll_pip_layer = NULL;
#endif
  menu_layer_destroy(s_menu_layer);
  text_layer_destroy(s_empty_layer);
#ifndef PBL_PLATFORM_APLITE
  text_layer_destroy(s_sync_progress_layer);
  text_layer_destroy(s_overtime_banner_layer);
  s_overtime_banner_layer = NULL;
#endif
  text_layer_destroy(s_error_layer);
#ifndef PBL_PLATFORM_APLITE
  if (s_pending_reschedule_timer) {
    app_timer_cancel(s_pending_reschedule_timer);
    s_pending_reschedule_timer = NULL;
  }
  s_pending_reschedule_kind = RESCHEDULE_NONE;
  s_pending_reschedule_task_id[0] = '\0';
#ifdef PBL_PLATFORM_EMERY
  if (s_pending_done_timer) {
    app_timer_cancel(s_pending_done_timer);
    s_pending_done_timer = NULL;
  }
  s_pending_done_task_id[0] = '\0';
#endif
#endif
#if defined(PBL_TOUCH)
  clear_tap_select_guard();
#endif
  bitmap_layer_destroy(s_logo_layer);
  gbitmap_destroy(s_logo_bitmap);
  gbitmap_destroy(s_check_bitmap);
  gbitmap_destroy(s_check_white_bitmap);
  gbitmap_destroy(s_heart_bitmap);
  gbitmap_destroy(s_heart_white_bitmap);
#ifndef PBL_PLATFORM_APLITE
  gbitmap_destroy(s_project_bitmap);
  gbitmap_destroy(s_project_white_bitmap);
  gbitmap_destroy(s_stats_bitmap);
  gbitmap_destroy(s_stats_white_bitmap);
  gbitmap_destroy(s_inbox_bitmap);
  gbitmap_destroy(s_inbox_white_bitmap);
#endif
#ifndef PBL_PLATFORM_APLITE
  dictation_session_destroy(s_dictation_session);
  gbitmap_destroy(s_mic_bitmap);
  gbitmap_destroy(s_mic_white_bitmap);
#endif
  status_bar_layer_destroy(s_status_bar);
}

#ifdef BREAK_REMINDER
// "Not tracking" nudge - purely time-based (no step count), checked once a
// minute while the app is open and nothing is tracked. Counts minutes since
// the last stop and repeats every s_idle_reminder_min minutes until tracking
// resumes (start_tracking resets the count). Idle-while-tracking has no
// separate behaviour of its own - the break reminder already covers that off
// accumulated tracked time.
static void maybe_notify_idle(void) {
  if (s_idle_reminder_min <= 0 || s_error_overlay_active ||
      s_tracking_task_id[0] != '\0' || s_break_last_stop_epoch == 0) {
    return;
  }
  // A remote device actively tracking counts as "something is being tracked" -
  // no nudge, and hold the idle baseline at now so the count restarts from
  // when that remote session ends, not from this watch's last local stop.
  if (s_presence_state == 1) {
    s_break_last_stop_epoch = time(NULL);
    s_untracked_notify_count = 0;
    return;
  }
  int elapsed_min = (int)((time(NULL) - s_break_last_stop_epoch) / 60);
  int intervals = elapsed_min / s_idle_reminder_min;
  if (intervals <= s_untracked_notify_count) {
    return; // already nudged for the current interval
  }
  // Jump straight to the current interval instead of incrementing by one:
  // s_untracked_notify_count starts at 0 every app open, so if the app was
  // last open (or tracking last stopped) several intervals ago, a plain ++
  // fired the banner every single tick until the count caught up. Now it
  // fires at most once per tick, on a real interval boundary.
  s_untracked_notify_count = intervals;
  snprintf(s_overtime_banner_text, sizeof(s_overtime_banner_text),
           "Not tracking\n%d min", elapsed_min);
  show_top_banner(s_overtime_banner_text);
}
#endif

#ifndef PBL_PLATFORM_APLITE
// config.stopAtMidnight: close out a session still running past the local
// midnight. A local timer is stopped and its time logged only up to 00:00
// (stop_tracking_at); a remote session gets a stop request - the producing
// device owns how its time splits across the day. Runs on the minute tick,
// so a timer left going overnight is caught the next time the app is opened,
// not only if the app happens to be foregrounded at midnight.
static void maybe_stop_at_midnight(struct tm *now_tm) {
  if (!s_stop_at_midnight) {
    return;
  }
  time_t now = time(NULL);
  time_t today_start = now - (now_tm->tm_hour * 3600 + now_tm->tm_min * 60 + now_tm->tm_sec);

  if (s_tracking_task_id[0] != '\0' && s_tracking_start_epoch < today_start) {
    stop_tracking_at(today_start);
    menu_layer_reload_data(s_menu_layer);
    refresh_scroll_state(true);
    return;
  }
  // A StopWatch/countdown habit timer left running overnight (its own slot -
  // can run alongside a task timer). Logs only up to 00:00, same as the task.
  if (s_tracking_habit_id[0] != '\0' && s_tracking_habit_start_epoch != 0 &&
      s_tracking_habit_start_epoch < today_start) {
    stop_habit_tracking_at(today_start);
    if (s_habits_menu_layer) {
      menu_layer_reload_data(s_habits_menu_layer);
    }
    return;
  }
  if (s_presence_state == 1 && s_presence_can_stop && !s_presence_stopping &&
      s_presence_elapsed_base != 0 && s_presence_elapsed_base < today_start) {
    s_presence_stopping = true;
    send_presence_stop();
    live_window_refresh();
  }
}

// From 18:00, once per local day: nudge if a habit with a 2+ day streak still
// isn't done. s_streak_nudge_day latches on tm_yday. Undone means the streak
// is measured behind today (it's still alive right now but breaks at midnight),
// which is exactly what habitStreak sends as `streak` for a not-done habit.
static void maybe_notify_streak_at_risk(struct tm *now_tm) {
  if (!s_habit_streak_nudge || s_error_overlay_active || now_tm->tm_hour < 18 ||
      s_streak_nudge_day == now_tm->tm_yday) {
    return;
  }
  int at_risk = 0;
  const char *one_title = NULL;
  for (int i = 0; i < s_habit_count; i++) {
    if (!s_habits[i].done && s_habits[i].streak >= 2) {
      at_risk++;
      one_title = s_habits[i].title;
    }
  }
  if (at_risk == 0) {
    return;
  }
  s_streak_nudge_day = now_tm->tm_yday;
  if (at_risk == 1) {
    snprintf(s_overtime_banner_text, sizeof(s_overtime_banner_text),
             "Keep your streak\n%s", one_title);
  } else {
    snprintf(s_overtime_banner_text, sizeof(s_overtime_banner_text),
             "Keep your streaks\n%d habits", at_risk);
  }
  show_top_banner(s_overtime_banner_text);
}

// MINUTE_UNIT tick: when s_due_reminder_min is set, fires the banner as the
// soonest upcoming timed task comes within that window; also drives the idle
// check above. App-open only.
static void minute_tick_handler(struct tm *now_tm, TimeUnits units_changed) {
  maybe_stop_at_midnight(now_tm);
#ifdef BREAK_REMINDER
  maybe_notify_idle();
#endif
  maybe_notify_streak_at_risk(now_tm);
  if (s_error_overlay_active) {
    return;
  }
  // Later Today: reclassify every minute so a task rejoins its normal group
  // the instant its scheduled time passes, without waiting for a sync.
  if (s_later_today_enabled && s_task_count > 0) {
    recompute_groups();
    menu_layer_reload_data(s_menu_layer);
  }
  int now_min = now_tm->tm_hour * 60 + now_tm->tm_min;

  // Both this and the global "notify before due" lead below share the same
  // "Notify before a task is due" setting - a task's own reminder time used
  // to always fire regardless of that toggle (it "superseded" the global
  // lead, per the comment that used to be here), which meant turning the
  // setting off didn't actually stop every due-related banner. Folded under
  // the same gate on request so Off really means off.
  if (s_due_reminder_min <= 0) {
    return;
  }
  // Per-task reminders (task.remindAt from the desktop): fire once when the
  // clock reaches the set time. One banner per tick; the rest catch up on
  // later ticks.
  for (int i = 0; i < s_task_count; i++) {
    if (s_tasks[i].remind_min >= 0 && !s_tasks[i].done && !s_tasks[i].remind_fired &&
        now_min >= s_tasks[i].remind_min) {
      s_tasks[i].remind_fired = true;
      snprintf(s_overtime_banner_text, sizeof(s_overtime_banner_text),
               "Reminder\n%s", s_tasks[i].title);
      show_top_banner(s_overtime_banner_text);
      break;
    }
  }
  // Global "notify before due" lead - skips tasks that carry their own
  // reminder (handled above).
  int soonest = -1;
  int soonest_idx = -1;
  for (int i = 0; i < s_task_count; i++) {
    int d = s_tasks[i].due_min;
    if (!s_tasks[i].done && s_tasks[i].remind_min < 0 && d >= now_min &&
        (soonest < 0 || d < soonest)) {
      soonest = d;
      soonest_idx = i;
    }
  }
  if (soonest < 0 || soonest - now_min > s_due_reminder_min || soonest == s_due_notified_min) {
    return;
  }
  s_due_notified_min = soonest;
  char at[16];
  format_due_time(soonest, false, at, sizeof(at)); // "@ 9:41 AM"
  snprintf(s_overtime_banner_text, sizeof(s_overtime_banner_text), "Task due\n%s %s", at,
           s_tasks[soonest_idx].title);
  show_top_banner(s_overtime_banner_text);
}
#endif

#if HEAP_BACKED_LISTS
// The list arrays that are static .bss on every other platform. Heap-allocated
// here so they don't count against emery's 64 KB virtual-size ceiling; emery
// has ~90 KB of free app heap, so this ~50 KB block is safe. Never freed - they
// live for the whole app, and PebbleOS reclaims the heap on exit anyway.
static bool alloc_heap_lists(void) {
  s_tasks    = calloc(MAX_TASKS, sizeof(Task));
  s_incoming = calloc(MAX_TASKS, sizeof(Task));
  s_habits   = calloc(MAX_HABITS, sizeof(Habit));
  s_groups   = calloc(MAX_GROUPS, sizeof(TaskGroup));
  s_row_map  = calloc(MAX_TASKS, sizeof(int));
  return s_tasks && s_incoming && s_habits && s_groups && s_row_map;
}
#endif

static void init(void) {
#ifndef PBL_PLATFORM_APLITE
  s_wakeup_launch = launch_reason() == APP_LAUNCH_WAKEUP;
#endif
  // Set the starting status through set_status_code() (not its static
  // initializer - see s_status_code's comment) so this first "Syncing..."
  // stretch goes through the same chokepoint as every later status.
  set_status_code(STATUS_SYNCING);
#if HEAP_BACKED_LISTS
  if (!alloc_heap_lists()) {
    // Can't realistically happen on emery. If it somehow does, there are no
    // list buffers, so skip the inbox handler (nothing to parse into) and show
    // the sync-error screen instead of dereferencing a NULL array.
    str_copy(s_status_msg, "Out of memory", sizeof(s_status_msg));
    set_status_code(STATUS_ERROR);
    s_main_window = window_create();
    window_set_window_handlers(s_main_window, (WindowHandlers) {
      .load = window_load,
      .unload = window_unload,
    });
    window_stack_push(s_main_window, true);
    return;
  }
#endif
  load_tasks();
  load_habits();
  load_tracking();
#ifndef PBL_PLATFORM_APLITE
  if (persist_exists(PERSIST_KEY_LAST_SYNC)) {
    s_last_sync_epoch = (time_t)persist_read_int(PERSIST_KEY_LAST_SYNC);
  }
#endif
#ifndef PBL_PLATFORM_APLITE
  load_habit_tracking();
  load_focus();
  // Drop a stale session: focus needs a live local track, and a timed session
  // already past its end is over (the app was closed when it expired). A
  // Flowtime session (no end) survives only while the track is still live.
  if ((s_focus_end_epoch != 0 || s_focus_start_epoch != 0) &&
      (s_tracking_task_id[0] == '\0' ||
       (s_focus_end_epoch != 0 && time(NULL) >= s_focus_end_epoch))) {
    s_focus_end_epoch = 0;
    s_focus_start_epoch = 0;
    s_focus_on_break = false;
    save_focus();
  }
#endif
#ifdef BREAK_REMINDER
  load_break_state();
#endif
  recompute_groups();

#ifndef PBL_PLATFORM_APLITE
  // Re-pin a resumed tracking session (the pin setting isn't known until the
  // first sync; the section appears then). A stopped-but-in-grace task is NOT
  // re-pinned - the grace period isn't persisted state.
  if (s_tracking_task_id[0] != '\0') {
    str_copy(s_pinned_task_id, s_tracking_task_id, MAX_ID_LEN);
  }
#endif

#ifndef PBL_PLATFORM_APLITE
  // If a resumed session is already past its estimate at open, latch
  // s_overtime_notified so the first tick doesn't fire the banner - only a
  // crossing while the app is open should notify. maybe_notify_overtime re-arms
  // it if effective time later dips back under.
  if (s_tracking_task_id[0] != '\0') {
    Task *resumed = find_task_by_id(s_tracking_task_id);
    if (resumed && resumed->time_estimate_ms > 0) {
      int effective_ms = resumed->time_spent_ms;
      time_t elapsed_s = time(NULL) - s_tracking_start_epoch;
      if (elapsed_s > 0) {
        effective_ms += (int)elapsed_s * 1000;
      }
      if (effective_ms >= resumed->time_estimate_ms) {
        s_overtime_notified = true;
        // Count any "repeat every 5 minutes" interval from launch, not from the
        // first tick.
        s_overtime_last_notify_epoch = time(NULL);
      }
    }
  }
#endif

  app_message_register_inbox_received(inbox_received_handler);
  app_message_register_inbox_dropped(inbox_dropped_handler);
  app_message_register_outbox_failed(outbox_failed_handler);
#ifndef PBL_PLATFORM_APLITE
  app_message_register_outbox_sent(outbox_sent_handler);
#endif
  // Not app_message_*_size_maximum() (~8 KB each): two maxed buffers eat ~16 KB
  // of the ~25 KB heap, leaving too little for the Habits window's layers.
  // Largest messages are MSG_TASK_ITEM (~500 B) and MSG_NOTE_CHUNK (~1 KB with
  // UTF-8); 2 KB in / 1 KB out clears both.
  app_message_open(2048, 1024);

  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = window_load,
    .appear = window_appear,
    .unload = window_unload,
  });
  window_stack_push(s_main_window, true);

#ifndef PBL_PLATFORM_APLITE
  // A focus session that survived an app close (the inactivity timeout, or
  // the user backing out by mistake) comes straight back to its screen, and
  // re-arms the keepalive. This is the "never loses focus" half of the
  // feature that survives the app actually being killed.
  if (focus_active()) {
    push_live_window();
  } else if (s_add_task_enabled && launch_reason() == APP_LAUNCH_QUICK_LAUNCH) {
#ifdef PBL_PLATFORM_EMERY
    // Time 2 alone reports which button quick-launched it - Down goes
    // straight to dictation, Up (or anything else) opens the list as usual.
    if (launch_button() == BUTTON_ID_DOWN) {
      start_add_task_dictation();
    }
#else
    start_add_task_dictation();
#endif
  }
#endif

#if defined(PBL_TOUCH)
  // Apply the default (off); the first sync turns it on if the phone says so.
  apply_touch_nav();
#endif
#ifndef PBL_PLATFORM_APLITE
  tick_timer_service_subscribe(MINUTE_UNIT, minute_tick_handler);
#endif
}

#ifndef PBL_PLATFORM_APLITE
// Launcher App Glance: one line under the app name in the launcher. The
// tracked task wins ("Now: <title>"); otherwise the soonest still-upcoming
// timed task today ("@ 9:41 AM  <title>"), same pick as minute_tick_handler.
// Set from deinit as the app closes - there's no background worker, so it
// reflects state at the last app close. aplite has no App Glance support.
static void glance_reload_cb(AppGlanceReloadSession *session, size_t limit,
                             void *context) {
  if (limit < 1) {
    return;
  }
  char subtitle[96];
  subtitle[0] = '\0';

  if (s_tracking_task_id[0] != '\0') {
    Task *t = find_task_by_id(s_tracking_task_id);
    if (t) {
      snprintf(subtitle, sizeof(subtitle), "Now: %s", t->title);
    }
  }

  if (subtitle[0] == '\0') {
    time_t now = time(NULL);
    struct tm *lt = localtime(&now);
    int now_min = lt->tm_hour * 60 + lt->tm_min;
    int soonest = -1, soonest_idx = -1;
    for (int i = 0; i < s_task_count; i++) {
      int d = s_tasks[i].due_min;
      if (!s_tasks[i].done && d >= now_min && (soonest < 0 || d < soonest)) {
        soonest = d;
        soonest_idx = i;
      }
    }
    if (soonest_idx >= 0) {
      char at[16];
      format_due_time(soonest, false, at, sizeof(at)); // "@ 9:41 AM"
      snprintf(subtitle, sizeof(subtitle), "%s  %s", at,
               s_tasks[soonest_idx].title);
    }
  }

  if (subtitle[0] == '\0') {
    return; // nothing to show - app_glance_reload already cleared old slices
  }

  // '{' / '}' get parsed as template-string tokens by the launcher and would
  // blank the line; task titles rarely contain them, but strip to be safe.
  for (char *p = subtitle; *p; p++) {
    if (*p == '{' || *p == '}') {
      *p = ' ';
    }
  }

  app_glance_add_slice(session, (AppGlanceSlice) {
    .layout = {
      .icon = APP_GLANCE_SLICE_DEFAULT_ICON,
      .subtitle_template_string = subtitle,
    },
    .expiration_time = APP_GLANCE_SLICE_NO_EXPIRATION,
  });
}
#endif

static void deinit(void) {
#ifndef PBL_PLATFORM_APLITE
  app_glance_reload(glance_reload_cb, NULL);
#endif
#if defined(PBL_TOUCH)
  s_touch_nav_enabled = false;
  apply_touch_nav();  // unsubscribe + cancel the long-press timer
#endif
#ifndef PBL_PLATFORM_APLITE
  // Relinquish the backlight to automatic control before exiting - otherwise an
  // always-on or mid-timeout override persists past the app. Unconditional
  // (not gated on s_backlight_mode) since set_status_code() can also force it
  // on for a mid-sync exit. Harmless if the app never touched the backlight.
  if (s_backlight_timer) {
    app_timer_cancel(s_backlight_timer);
  }
  light_enable(false);
#endif
  window_destroy(s_main_window);
  if (s_habits_window) {
    window_destroy(s_habits_window);
  }
#ifndef PBL_PLATFORM_APLITE
  if (s_notes_window) {
    window_destroy(s_notes_window);
  }
  if (s_schedule_window) {
    window_destroy(s_schedule_window);
  }
  if (s_upcoming_window) {
    window_destroy(s_upcoming_window);
  }
  stop_live_tick();
  if (s_live_window) {
    window_destroy(s_live_window);
  }
#endif
#if PROJECTS_BROWSER
  if (s_browse_window) {
    window_destroy(s_browse_window);
  }
#endif
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
