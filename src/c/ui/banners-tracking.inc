// Top banners and tracking control (start/stop, over-estimate, break, idle, due).
// Textually #included by main.c (one translation unit) - not compiled alone.

#ifndef PBL_PLATFORM_APLITE
// Shows or hides the top banner strip - instant, no slide (dropped 2026-09-15
// to reclaim ~330B of emery code-space; it's cosmetic only, every platform
// behaves the same as basalt/chalk/diorite always did).
static void banner_slide(bool in) {
  if (s_overtime_banner_layer) {
    layer_set_hidden(text_layer_get_layer(s_overtime_banner_layer), !in);
  }
}

static void overtime_banner_timeout_callback(void *data) {
  s_overtime_banner_timer = NULL;
  banner_slide(false);
}

// Hides the over-estimate banner and cancels its auto-dismiss timer - safe to
// call whether or not it's showing.
static void hide_overtime_banner(void) {
  if (s_overtime_banner_timer) {
    app_timer_cancel(s_overtime_banner_timer);
    s_overtime_banner_timer = NULL;
  }
  if (s_overtime_banner_layer) {
    layer_set_hidden(text_layer_get_layer(s_overtime_banner_layer), true);
  }
}

// Short rising two-note chime played with any banner when s_audible_notify is
// on. Only compiled on speaker hardware (Pebble Time 2); a no-op call otherwise.
// Skipped when the user muted the watch system-wide.
static void banner_ping(void) {
#ifdef PBL_SPEAKER
  if (speaker_is_muted()) {
    return;
  }
  static const SpeakerNote notes[] = {
    { .midi_note = 84, .waveform = SpeakerWaveformSine, .duration_ms = 90, .velocity = 0 },
    { .midi_note = 88, .waveform = SpeakerWaveformSine, .duration_ms = 110, .velocity = 0 },
  };
  speaker_play_notes(notes, ARRAY_LENGTH(notes),
                     s_audible_volume < 0 ? 0 : (s_audible_volume > 100 ? 100 : s_audible_volume));
#endif
}

// Shows `text` in the top banner strip with a double vibe (and a ping when
// audible notifications are on), and (re)arms the auto-dismiss timer. `text`
// must stay valid until the banner hides - s_overtime_banner_text or a string
// literal. Shared by the over-estimate / break / idle / task-due banners; only
// one shows at a time (last writer wins).
static void show_top_banner(const char *text) {
  if (!s_overtime_banner_layer) {
    return;
  }
#ifndef PBL_PLATFORM_APLITE
  // Reparent onto whatever window is currently on top - live tracking, focus,
  // habits, notes, ... - so a banner always reaches the user instead of
  // staying hidden under the main list it was built as a child of.
  // layer_add_child removes it from its old parent first.
  layer_add_child(window_get_root_layer(window_stack_get_top_window()),
                   text_layer_get_layer(s_overtime_banner_layer));
#endif
  text_layer_set_text(s_overtime_banner_layer, text);
  layer_set_hidden(text_layer_get_layer(s_overtime_banner_layer), false);
  banner_slide(true); // slides down from the top, then rests
  vibes_double_pulse();
  if (s_audible_notify) {
    banner_ping();
  }
  if (s_overtime_banner_timer) {
    app_timer_cancel(s_overtime_banner_timer);
  }
  s_overtime_banner_timer = app_timer_register(OVERTIME_BANNER_MS, overtime_banner_timeout_callback, NULL);
}

static void show_overtime_banner(const char *task_title) {
  snprintf(s_overtime_banner_text, sizeof(s_overtime_banner_text),
            "Over estimate\n%s", task_title);
  show_top_banner(s_overtime_banner_text);
}

// Called once per tracking tick: fires the over-estimate banner the first time
// effective time (synced spent + the running session's elapsed) reaches the
// estimate. Covers both a task tracked ON this watch and one tracked on another
// device (remote presence, state 1) when the phone has sent that task's synced
// spent + estimate. Latched via s_overtime_notified (re-armed if effective time
// drops back under). With the "repeat every 5 minutes" sub-option, re-fires
// every OVERTIME_REPEAT_INTERVAL_S while the task stays over.
static void maybe_notify_overtime(void) {
  if (!s_overtime_notify_enabled) {
    return;
  }

  const char *over_title;
  int effective_ms;
  int estimate_ms;
  int elapsed_s;

  if (s_tracking_task_id[0] != '\0') {
    Task *task = find_task_by_id(s_tracking_task_id);
    if (!task || task->time_estimate_ms <= 0) {
      return;
    }
    effective_ms = task->time_spent_ms;
    elapsed_s = (int)(time(NULL) - s_tracking_start_epoch);
    estimate_ms = task->time_estimate_ms;
    over_title = task->title;
  } else if (s_presence_state == 1 && s_presence_estimate_ms > 0) {
    // Another device is tracking; s_presence_spent_ms is that task's synced
    // time-spent AS OF s_presence_spent_received_epoch (this push's receipt
    // time), NOT s_presence_elapsed_base (the session's start) - see
    // s_presence_spent_received_epoch's own comment for why using the
    // session's start here double-counted and could fire this notification
    // early.
    effective_ms = s_presence_spent_ms;
    elapsed_s = (int)(time(NULL) - s_presence_spent_received_epoch);
    estimate_ms = s_presence_estimate_ms;
    over_title = s_presence_task[0] != '\0' ? s_presence_task : "Live tracking";
  } else {
    return;
  }
  if (elapsed_s > 0) {
    effective_ms += elapsed_s * 1000;
  }

  if (effective_ms < estimate_ms) {
    s_overtime_notified = false; // re-arm for a later crossing
    return;
  }
  if (s_error_overlay_active) {
    return;
  }
  if (!s_overtime_notified) {
    s_overtime_notified = true;
    s_overtime_last_notify_epoch = time(NULL);
    show_overtime_banner(over_title);
    return;
  }
  // Already notified this crossing - "repeat every 5 minutes" re-fires it.
  if (s_overtime_repeat_enabled &&
      time(NULL) - s_overtime_last_notify_epoch >= OVERTIME_REPEAT_INTERVAL_S) {
    s_overtime_last_notify_epoch = time(NULL);
    show_overtime_banner(over_title);
  }
}

#ifdef BREAK_REMINDER
// Called once per tracking tick, next to maybe_notify_overtime: fires the
// "time for a break" banner the first time this watch's banked tracked time
// since the last break (s_break_accum_s + the running session's elapsed)
// reaches s_break_reminder_min. Latched via s_break_notified until start_tracking
// sees a real gap. Local task tracking only.
static void maybe_notify_break(void) {
  if (s_tracking_task_id[0] == '\0' || s_break_reminder_min <= 0 || s_break_notified) {
    return;
  }
  int elapsed_s = (int)(time(NULL) - s_tracking_start_epoch);
  if (elapsed_s < 0) {
    elapsed_s = 0;
  }
  if (s_break_accum_s + elapsed_s < s_break_reminder_min * 60) {
    return;
  }
  if (s_error_overlay_active) {
    return;
  }
  s_break_notified = true;
  show_top_banner("Time for a break");
}
#endif

// Re-lays-out the task list after the pinned "TRACKING" section appears or
// disappears - the section and row counts change, so a full reload_data plus a
// scroll-state refresh.
static void refresh_pinned_section(void) {
  if (!s_menu_layer) {
    return;
  }
  menu_layer_reload_data(s_menu_layer);
  refresh_scroll_state(true);
}

static void unpin_timer_callback(void *data) {
  s_unpin_timer = NULL;
  s_pinned_task_id[0] = '\0';
  refresh_pinned_section();
}

// Cancels a pending unpin-grace timer - the task is about to be re-pinned, or
// the app is shutting down.
static void cancel_unpin_timer(void) {
  if (s_unpin_timer) {
    app_timer_cancel(s_unpin_timer);
    s_unpin_timer = NULL;
  }
}
#endif

// The epoch the current live session is measured from: local tracking's own
// start, or (non-aplite) another device's presence session start. No active
// session falls through to "now" - the loop is about to stop anyway, so the
// interval this yields is moot.
static time_t current_tracking_anchor_epoch(void) {
  if (s_tracking_task_id[0] != '\0') {
    return s_tracking_start_epoch;
  }
#ifndef PBL_PLATFORM_APLITE
  if (s_presence_state != 0) {
    return s_presence_elapsed_base;
  }
#endif
  return time(NULL);
}

// See TRACKING_TICK_SLOW_MS. Keyed off THIS session's own elapsed time, not
// the task's total spent+session (which would need a Task lookup at every
// call site) - a task that already had 55 minutes logged still ticks fast
// for its first hour here, which only under-optimizes, never shows a stale
// number.
static uint32_t tracking_tick_interval_ms(void) {
  return (time(NULL) - current_tracking_anchor_epoch()) >= 3600
      ? TRACKING_TICK_SLOW_MS : TRACKING_TICK_INTERVAL_MS;
}

static void tracking_tick_callback(void *data) {
  // Only the elapsed-time text changes each tick - mark_dirty (repaint), not
  // reload_data (which also re-asks for section/row counts).
  layer_mark_dirty(menu_layer_get_layer(s_menu_layer));
#ifndef PBL_PLATFORM_APLITE
  maybe_notify_overtime();
#endif
#ifdef BREAK_REMINDER
  maybe_notify_break();
#endif
  s_tracking_tick_timer = app_timer_register(tracking_tick_interval_ms(), tracking_tick_callback, NULL);
}

static void start_tracking_tick(void) {
  if (!s_tracking_tick_timer) {
    s_tracking_tick_timer = app_timer_register(tracking_tick_interval_ms(), tracking_tick_callback, NULL);
  }
}

static void stop_tracking_tick(void) {
  if (s_tracking_tick_timer) {
    app_timer_cancel(s_tracking_tick_timer);
    s_tracking_tick_timer = NULL;
  }
}

static void start_tracking(Task *task) {
  str_copy(s_tracking_task_id, task->id, MAX_ID_LEN);
  s_tracking_start_epoch = time(NULL);
  save_tracking();
#ifndef PBL_PLATFORM_APLITE
  // Fresh session - re-arm the over-estimate banner and clear any stale one.
  s_overtime_notified = false;
  s_overtime_last_notify_epoch = 0;
  hide_overtime_banner();
#ifdef BREAK_REMINDER
  // Break reminder: a long enough pause since the last session counts as a real
  // break - zero the running tally and re-arm the banner. A shorter gap just
  // carries the tally forward into this session.
  if (s_break_last_stop_epoch != 0 &&
      time(NULL) - s_break_last_stop_epoch >= BREAK_RESET_GAP_S) {
    s_break_accum_s = 0;
    s_break_notified = false;
    // This gap was a real break - record it for the Stats page.
    break_roll_day();
    int gap = (int)(time(NULL) - s_break_last_stop_epoch);
    s_break_count_today++;
    s_break_total_s_today += gap > BREAK_MAX_S ? BREAK_MAX_S : gap;
  }
  save_break_state();
  // Fresh "not tracking" window for the next gap.
  s_untracked_notify_count = 0;
#endif
  // Pin this task to the top (if enabled); cancel any grace timer from a
  // just-stopped task and re-lay-out the list.
  cancel_unpin_timer();
  str_copy(s_pinned_task_id, task->id, MAX_ID_LEN);
#endif
  start_tracking_tick();
#ifndef PBL_PLATFORM_APLITE
  if (has_pinned_row()) {
    refresh_pinned_section();
    // Highlight the freshly-pinned row (section 1, row 0).
    menu_layer_set_selected_index(s_menu_layer, MenuIndex(1, 0), MenuRowAlignCenter, false);
  }
  // Let the phone broadcast this as "Tracking on Pebble" to other devices.
  send_track_time_start(task->id, 0);
#endif
}

// Stops whatever's being tracked (a no-op if nothing is) and reports the
// session up to `end_epoch` for upload (handleTrackTimeStop in index.js).
// stop_tracking_and_report() passes now; the midnight auto-stop passes the
// day boundary so the overnight run isn't logged.
static void stop_tracking_at(time_t end_epoch) {
  if (s_tracking_task_id[0] == '\0') {
    return;
  }
  time_t elapsed_s = end_epoch - s_tracking_start_epoch;
#ifndef PBL_PLATFORM_APLITE
  // Always sent (even a 0ms session) so the phone can end the "Tracking on
  // Pebble" presence broadcast; the phone ignores a 0 delta for the op upload.
  int32_t elapsed_ms = elapsed_s > 0 ? (int32_t)elapsed_s * 1000 : 0;
  send_track_time_stop(s_tracking_task_id, elapsed_ms);
  if (elapsed_ms > 0) {
    Task *tracked_task = find_task_by_id(s_tracking_task_id);
    if (tracked_task) {
      tracked_task->time_spent_ms += elapsed_ms;
      save_tasks();
    }
  }
#else
  if (elapsed_s > 0) {
    int32_t elapsed_ms = (int32_t)elapsed_s * 1000;
    send_track_time_stop(s_tracking_task_id, elapsed_ms);
    // Optimistic local bump so the subtitle doesn't revert to the pre-session
    // total until the next full sync reports the real merged total.
    Task *tracked_task = find_task_by_id(s_tracking_task_id);
    if (tracked_task) {
      tracked_task->time_spent_ms += elapsed_ms;
      save_tasks();
    }
  }
#endif
  s_tracking_task_id[0] = '\0';
  s_tracking_start_epoch = 0;
  save_tracking();
#ifndef PBL_PLATFORM_APLITE
  // A focus session only wraps a local track - stopping the timer ends it too
  // (drops the keepalive timer, releases the backlight). Silent: the stop
  // itself is the user's cue. Covers Flowtime too (end==0, start!=0).
  if (s_focus_end_epoch != 0 || s_focus_start_epoch != 0) {
    focus_end(false);
  }
  s_overtime_notified = false;
  s_overtime_last_notify_epoch = 0;
  hide_overtime_banner();
#ifdef BREAK_REMINDER
  // Bank this session's tracked time and stamp the stop, so the next start can
  // tell a real break from a brief pause. s_break_notified is deliberately kept
  // - a quick stop/restart shouldn't clear an already-shown break banner.
  s_break_accum_s += elapsed_s > 0 ? (int)elapsed_s : 0;
  s_break_last_stop_epoch = time(NULL);
  save_break_state();
#endif
  // Keep the just-stopped task pinned for a short grace period so it slides
  // back into its group smoothly. A new start_tracking() cancels this.
  if (s_pinned_task_id[0] != '\0') {
    cancel_unpin_timer();
    s_unpin_timer = app_timer_register(UNPIN_GRACE_MS, unpin_timer_callback, NULL);
  }
  live_window_refresh(); // pop the detail screen if it was open on this session
#endif
  stop_tracking_tick();
}

static void stop_tracking_and_report(void) {
  stop_tracking_at(time(NULL));
}

static void menu_select_long_click(MenuLayer *menu_layer, MenuIndex *cell_index, void *context);

static void menu_select_click(MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  backlight_touch();
#if defined(PBL_TOUCH)
  // A touch tap. The bridge has already moved the highlight to the tapped row.
  //  - moved to a NEW row (s_tap_moved_sel), or a long-press already fired:
  //    select-only, this synthesised SELECT is a no-op.
  //  - tap on the row that was ALREADY selected: activate it. A real task row
  //    opens its action menu (not toggle-done); every other row (the pinned
  //    "TRACKING" row, the nav rows, a project row) falls through to do exactly
  //    what a physical Select would.
  if (consume_tap_select_guard()) {
    if (s_tap_moved_sel || s_touch_longpress_fired) {
      return;
    }
    // While a pending reschedule / "Marking done..." window is up, a tap cancels
    // it (like a physical Select) - fall through to the checks just below.
    bool pending_window = s_pending_reschedule_kind != RESCHEDULE_NONE;
#ifdef PBL_PLATFORM_EMERY
    pending_window = pending_window || s_pending_done_task_id[0] != '\0';
#endif
    if (!pending_window) {
      bool pinned_row = has_pinned_row() && cell_index->section == 1;
      if (!pinned_row && resolve_task_at(*cell_index)) {
        menu_select_long_click(s_menu_layer, cell_index, NULL);
        return;
      }
    }
    // fall through: pinned / nav / project / Finish Day / a pending window
  }
#endif
#ifndef PBL_PLATFORM_APLITE
  // A pending move-to-tomorrow / unschedule / "Marking done..." is cancelled by
  // a physical Select during its window - checked before the normal row handling
  // so the press only cancels (no toggle, no notes).
  if (s_pending_reschedule_kind != RESCHEDULE_NONE) {
    cancel_pending_reschedule();
    return;
  }
#ifdef PBL_PLATFORM_EMERY
  if (s_pending_done_task_id[0] != '\0') {
    Task *pd_sel = resolve_selected_task();
    if (pd_sel && strncmp(pd_sel->id, s_pending_done_task_id, MAX_ID_LEN) == 0) {
      cancel_pending_done(); // Select on the same row within the window = undo
      return;
    }
    // Selection has moved on - commit the pending one now and let this press
    // fall through to start the newly-selected row's own toggle.
    if (s_pending_done_timer) {
      app_timer_cancel(s_pending_done_timer);
      pending_done_commit_callback(NULL);
    }
  }
#endif
  // The over-estimate banner is a plain layer on the menu - a Select while
  // it's up just dismisses it, like the error overlay.
  if (s_overtime_banner_layer &&
      !layer_get_hidden(text_layer_get_layer(s_overtime_banner_layer))) {
    hide_overtime_banner();
    return;
  }
#endif
  // No s_notes_overlay_active check - the notes overlay is a separate pushed
  // Window, so this callback never fires while it's on top.
  if (s_error_overlay_active) {
    // Click routing goes through MenuLayer's config even while its layer is
    // hidden. Retry immediately after dismissing ("Select to retry") rather
    // than making the user find Resync - the usual cause is a transient send
    // failure where "try again" is the fix.
    hide_error_overlay();
    request_sync();
    return;
  }
  if (s_task_count == 0 && !ACTIONABLE_EMPTY_ACTIVE()) {
    // The empty/error screen's phantom row 0 - this makes "Select to retry"
    // retry. The actionable empty state falls through to normal section-0
    // routing below.
    request_sync();
    return;
  }
  if (cell_index->section == 0) {
    Section0RowKind kind = section0_row_kind((int)cell_index->row);
    if (kind == SECTION0_ROW_HABITS) {
      push_habits_window();
#if PROJECTS_BROWSER
    } else if (kind == SECTION0_ROW_PROJECTS) {
      s_browse_mode = BROWSE_PROJECTS;
      push_browse_window(NULL);
    } else if (kind == SECTION0_ROW_TAGS) {
      s_browse_mode = BROWSE_TAGS;
      push_browse_window(NULL);
#endif
#ifndef PBL_PLATFORM_APLITE
    } else if (kind == SECTION0_ROW_STATS) {
      push_stats_window();
    } else if (kind == SECTION0_ROW_SCHEDULE) {
      push_schedule_window();
#ifdef PBL_PLATFORM_EMERY
    } else if (kind == SECTION0_ROW_CALENDAR) {
      push_calendar_window();
#endif
    } else if (kind == SECTION0_ROW_UPCOMING) {
      push_page_window(PAGE_UPCOMING);
    } else if (kind == SECTION0_ROW_NOTESPAGE) {
      push_page_window(PAGE_NOTES);
    } else if (kind == SECTION0_ROW_SEARCH) {
      start_search_dictation();
    } else if (kind == SECTION0_ROW_ADD_TASK) {
      start_add_task_dictation();
#endif
    } else {
      request_sync(); // the "Resync" row
    }
    return;
  }
#ifndef PBL_PLATFORM_APLITE
  // The pinned "TRACKING" row (section 1) - Select opens the full-screen
  // tracking detail: the local session, or a remote device's (same screen the
  // dark-blue LIVE row opens). Select there stops the timer.
  // s_task_count != 0 keeps this from firing in the empty-list state, where
  // section 1 is the Finish Day row instead (a remote presence session can
  // hold remote_in_pinned_section() true independent of s_task_count).
  if (s_task_count != 0 && cell_index->section == 1 &&
      (s_tracking_task_id[0] != '\0' || remote_in_pinned_section())) {
    push_live_window();
    return;
  }
  // The project row - Select toggles its tasks hidden/shown in place when
  // s_collapse_groups is on, else opens that project's tasks in the browser.
  // Long-Select opens its notes either way (menu_select_long_click).
  TaskGroup *project_row = resolve_project_row_at(*cell_index);
  if (project_row) {
    if (s_collapse_groups) {
      project_row->user_collapsed = !project_row->user_collapsed;
      menu_layer_reload_data(s_menu_layer);
      return;
    }
#if PROJECTS_BROWSER
    // A non-project group (Later Today, or a tag/deadline/planned-date
    // group) has no project to browse into - swallow the click rather than
    // dropping into the unrelated Projects list.
    if (project_row->project_id[0] != '\0') {
      s_browse_mode = BROWSE_PROJECTS;
      push_browse_window(project_row->project_id);
    }
#endif
    return;
  }
  // The Finish Day row: Select opens the Reflect energy check-in when it's
  // enabled (long-Select still archives - menu_select_long_click). The row
  // resolves to no task, so this must come before resolve_task_at's NULL.
  if (s_reflect_enabled && is_finish_day_cell(cell_index)) {
    push_reflect_window();
    return;
  }
#endif
  Task *task = resolve_task_at(*cell_index);
  if (!task) {
    return;
  }
#ifndef PBL_PLATFORM_APLITE
  // Select on a task row opens its action menu (Mark done / track / schedule /
  // notes / ...) - it used to be the toggle-done and a long-Select opened the
  // menu; that swapped so the menu is one press, not a hold.
  push_action_menu(task->id, ACTX_TODAY, false);
#else
  task->done = !task->done;
  save_tasks();
  menu_layer_reload_data(s_menu_layer);
  send_task_toggle(task);
#endif
}

// Long-select on a task row opens its action menu (non-aplite) - track, notes,
// tags, estimate, deadline. aplite has no menu window budget, so there it's the
// plain track toggle it always was.
static void menu_select_long_click(MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  backlight_touch();
  // No s_notes_overlay_active check - note-append is wired on s_notes_window's
  // own click config.
  // s_task_count == 0 only bails when the empty state isn't actionable - the
  // actionable one still needs long-Select to reach the Finish Day row below.
  if (s_error_overlay_active || cell_index->section == 0 ||
      (s_task_count == 0 && !ACTIONABLE_EMPTY_ACTIVE())) {
    return;
  }
#ifndef PBL_PLATFORM_APLITE
  if (is_finish_day_cell(cell_index)) {
    // Finish Day row. No optimistic local change - archiving needs the phone's
    // full state.task cache; this is fire-and-forget and the phone pushes an
    // updated list back.
#ifdef PBL_PLATFORM_EMERY
    // The moment window owns the close (after its animation, or a button press);
    // a send failure still routes to the error overlay, which it leaves up.
    send_finish_day();
    s_close_after_finish_day_sent = false;
    push_finishday_window();
#else
    // Closes the app once the send confirms, not eagerly.
    s_close_after_finish_day_sent = true;
    send_finish_day();
#endif
    return;
  }
  // The project row - long-Select opens its notes (Select opens its tasks).
  // A non-project group (Later Today, or a tag/deadline/planned-date group)
  // has no project to open notes for - swallow the long-Select instead.
  TaskGroup *project_row = resolve_project_row_at(*cell_index);
  if (project_row) {
    if (project_row->project_id[0] != '\0') {
      show_project_notes_overlay(project_row);
    }
    return;
  }
#endif
  Task *task = resolve_task_at(*cell_index);
  if (!task) {
    return;
  }
#ifndef PBL_PLATFORM_APLITE
  push_action_menu(task->id, ACTX_TODAY, false);
#else
  if (task->done) {
    return; // tracking a completed task isn't a real scenario
  }
  bool already_tracking_this = s_tracking_task_id[0] != '\0' &&
                                strncmp(s_tracking_task_id, task->id, MAX_ID_LEN) == 0;
  stop_tracking_and_report();
  if (!already_tracking_this) {
    start_tracking(task);
  }
  menu_layer_reload_data(s_menu_layer);
#endif
}
