// Status/error overlays, notes overlay, and the pending-reschedule flow.
// Textually #included by main.c (one translation unit) - not compiled alone.

// ---------- empty / status placeholder ----------

static void stop_syncing_animation(void) {
  if (s_syncing_timer) {
    app_timer_cancel(s_syncing_timer);
    s_syncing_timer = NULL;
  }
#ifndef PBL_PLATFORM_APLITE
  // Restore s_empty_layer's plain font and hide the percent/hint subtitle -
  // both only used while syncing.
  text_layer_set_font(s_empty_layer, fonts_get_system_font(EMPTY_MSG_FONT_KEY));
  layer_set_hidden(text_layer_get_layer(s_sync_progress_layer), true);
#endif
}

#ifndef PBL_PLATFORM_APLITE
// Refreshes s_sync_progress_layer from s_status_msg - "Decrypting NN%" while a
// page of ops decrypts, or the "may take a few minutes" fallback before a
// percentage is available. The percent gets a bigger font (short enough not to
// wrap; the fallback sentence would).
static void update_sync_progress_text(void) {
  if (s_status_msg[0] != '\0') {
    text_layer_set_font(s_sync_progress_layer, fonts_get_system_font(EMPTY_MSG_FONT_KEY));
    text_layer_set_text(s_sync_progress_layer, s_status_msg);
  } else {
    text_layer_set_font(s_sync_progress_layer, fonts_get_system_font(CHROME_FONT_KEY));
    text_layer_set_text(s_sync_progress_layer, "This may take a few minutes");
  }
}
#endif

static void syncing_timer_callback(void *data) {
  s_syncing_dots = (s_syncing_dots + 1) % 4;
#ifdef PBL_PLATFORM_APLITE
  // No spare TextLayer on aplite - the percentage rides the same line/font.
  static char s_syncing_text[MAX_STATUS_MSG_LEN + 16];
  if (s_status_msg[0] != '\0') {
    snprintf(s_syncing_text, sizeof(s_syncing_text), "Syncing %s%.*s", s_status_msg, s_syncing_dots, "...");
  } else {
    snprintf(s_syncing_text, sizeof(s_syncing_text), "Syncing%.*s\n\nThis may take a few minutes", s_syncing_dots, "...");
  }
  text_layer_set_text(s_empty_layer, s_syncing_text);
#else
  static char s_syncing_text[16];
  snprintf(s_syncing_text, sizeof(s_syncing_text), "Syncing%.*s", s_syncing_dots, "...");
  text_layer_set_text(s_empty_layer, s_syncing_text);
  update_sync_progress_text();
#endif
  s_syncing_timer = app_timer_register(SYNCING_ANIM_INTERVAL_MS, syncing_timer_callback, NULL);
}

static void start_syncing_animation(void) {
#ifndef PBL_PLATFORM_APLITE
  // Bigger than the other empty-state messages - safe because "Syncing..." is
  // always short, unlike the not-paired/error text sharing this layer.
  text_layer_set_font(s_empty_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  layer_set_hidden(text_layer_get_layer(s_sync_progress_layer), false);
  if (s_syncing_timer) {
    update_sync_progress_text(); // new percent mid-sync; timer already running
    return;
  }
#else
  if (s_syncing_timer) {
    return;
  }
#endif
  s_syncing_dots = 0;
#ifdef PBL_PLATFORM_APLITE
  text_layer_set_text(s_empty_layer, "Syncing\n\nThis may take a few minutes");
#else
  text_layer_set_text(s_empty_layer, "Syncing");
  update_sync_progress_text();
#endif
  s_syncing_timer = app_timer_register(SYNCING_ANIM_INTERVAL_MS, syncing_timer_callback, NULL);
}

// Shows (or re-affirms) the fullscreen error overlay, hiding the menu and
// empty-state layers under it.
//
// With a cached list to fall back on, a sync failure doesn't blank the
// screen: the tasks stay visible and the red Resync row ("Sync failed" /
// "Failed: ...", Select on it to retry) carries the error - see
// update_empty_layer's own "resync with a populated list" path. The
// fullscreen overlay is only for when there's nothing else to show.
static void show_error_overlay(void) {
  if (s_task_count > 0) {
    s_error_overlay_active = false;
    layer_set_hidden(text_layer_get_layer(s_error_layer), true);
    menu_layer_reload_data(s_menu_layer); // refresh the Resync row subtitle
    update_empty_layer();
    return;
  }
  s_error_overlay_active = true;
  static char s_error_overlay_text[MAX_STATUS_MSG_LEN + 48];
  if (s_status_msg[0] != '\0') {
    snprintf(s_error_overlay_text, sizeof(s_error_overlay_text),
              "Sync Error\n\n%s\n\nSelect to retry", s_status_msg);
  } else {
    snprintf(s_error_overlay_text, sizeof(s_error_overlay_text), "Sync Error\n\nSelect to retry");
  }
  text_layer_set_text(s_error_layer, s_error_overlay_text);
  layer_set_hidden(text_layer_get_layer(s_error_layer), false);
  layer_set_hidden(menu_layer_get_layer(s_menu_layer), true);
  layer_set_hidden(text_layer_get_layer(s_empty_layer), true);
  layer_set_hidden(bitmap_layer_get_layer(s_logo_layer), true);
#ifndef PBL_PLATFORM_APLITE
  // Drop the over-estimate banner so it doesn't half-cover the error.
  hide_overtime_banner();
#endif
  stop_syncing_animation();
}

static void update_empty_layer(void) {
  // The error overlay owns menu/empty-layer visibility while it's up - without
  // this guard a background status update (e.g. a retry's TASK_SYNC_END) would
  // un-hide the menu under the overlay. The notes overlay needs no such guard
  // (it's a separate pushed Window).
  if (s_error_overlay_active) {
    return;
  }
  bool show_empty = (s_task_count == 0);
  // STATUS_OK with zero tasks is an "actionable" empty state: the menu stays
  // visible with "No tasks for today." as section 0's header and the real
  // rows reachable, rather than the standalone s_empty_layer/s_logo_layer.
  bool show_actionable_empty = show_empty && ACTIONABLE_EMPTY_ACTIVE();
  layer_set_hidden(text_layer_get_layer(s_empty_layer), !show_empty || show_actionable_empty);
  layer_set_hidden(bitmap_layer_get_layer(s_logo_layer), !show_empty || show_actionable_empty);
  layer_set_hidden(menu_layer_get_layer(s_menu_layer), show_empty && !show_actionable_empty);

  // Only the first sync (no cached list) gets the animation; a resync with a
  // populated list shows status via the Resync row's subtitle.
  bool is_initial_syncing = show_empty && s_status_code == STATUS_SYNCING;
  if (is_initial_syncing) {
    start_syncing_animation();
  } else {
    stop_syncing_animation();
  }

  if (!show_empty || is_initial_syncing || show_actionable_empty) {
    return;
  }

  static char s_empty_text[MAX_STATUS_MSG_LEN + 32];

  // Only STATUS_NOT_PAIRED and STATUS_ERROR reach here (STATUS_OK and
  // STATUS_SYNCING were intercepted above).
  if (s_status_code == STATUS_NOT_PAIRED) {
    text_layer_set_text(s_empty_layer, "Open the app on\nyour phone to pair\nwith SuperSync.");
  } else {
    if (s_status_msg[0] != '\0') {
      snprintf(s_empty_text, sizeof(s_empty_text), "Sync error:\n%s\nSelect to retry.", s_status_msg);
      text_layer_set_text(s_empty_layer, s_empty_text);
    } else {
      text_layer_set_text(s_empty_layer, "Sync error.\nSelect to retry.");
    }
  }
}

// Dismisses the error overlay (Select) and hands visibility back to
// update_empty_layer() (unguarded now, since the flag flips first) to restore
// the right layer for the current s_task_count/s_status_code.
static void hide_error_overlay(void) {
  s_error_overlay_active = false;
  layer_set_hidden(text_layer_get_layer(s_error_layer), true);
  update_empty_layer();
}

#ifndef PBL_PLATFORM_APLITE
// "Notes:\n\n" is written into s_notes_full_text's buffer ahead of the chunks,
// not composed at render time (there's no bound to size a second buffer to).
#define NOTES_HEADER "Notes:\n\n"

// Height a word-wrapped text block lays out to at the given width. On-device,
// graphics_text_layout_get_content_size runs short of what TextLayer needs and
// the shortfall scales with length, so the margin is proportional (10%) plus a
// two-line floor. A short note just gets harmless extra scroll room.
#define NOTES_TEXT_HEIGHT_MARGIN_FLOOR 48

static int16_t measure_notes_text_height(int16_t width, const char *text) {
  GSize size = graphics_text_layout_get_content_size(
      text, fonts_get_system_font(NOTES_BODY_FONT_KEY), GRect(0, 0, width, 20000),
      GTextOverflowModeWordWrap, GTextAlignmentLeft);
  int16_t margin = size.h / 10;
  if (margin < NOTES_TEXT_HEIGHT_MARGIN_FLOOR) {
    margin = NOTES_TEXT_HEIGHT_MARGIN_FLOOR;
  }
  return size.h + margin;
}

// The names line - the comma-joined tags, or NOTES_TAGS_EMPTY_TEXT for an
// untagged task. Shared by measure_notes_tags_parts and notes_tags_layer_draw
// so measured and drawn never disagree.
static const char *notes_tags_display_line(void) {
  return s_notes_tags_line[0] != '\0' ? s_notes_tags_line : NOTES_TAGS_EMPTY_TEXT;
}

// Measures the bold "Tags:" label and the (wrapped) tag names separately, given
// the padded content width - shared by render_notes_overlay_content (needs the
// total) and notes_tags_layer_draw (needs each height) so they can't disagree.
static void measure_notes_tags_parts(int16_t content_w, int16_t *out_label_h, int16_t *out_names_h) {
  GSize label_size = graphics_text_layout_get_content_size(
      NOTES_TAGS_LABEL, fonts_get_system_font(NOTES_LABEL_FONT_KEY),
      GRect(0, 0, content_w, 2000), GTextOverflowModeWordWrap, GTextAlignmentLeft);
  GSize names_size = graphics_text_layout_get_content_size(
      notes_tags_display_line(), fonts_get_system_font(NOTES_BODY_FONT_KEY),
      GRect(0, 0, content_w, 2000), GTextOverflowModeWordWrap, GTextAlignmentLeft);
  *out_label_h = label_size.h;
  *out_names_h = names_size.h;
}

// Applies s_notes_display_text and s_notes_tags_line to the created
// TextLayers/ScrollLayer and resizes to match. Shared by notes_window_load and
// every later update. A no-op if the window isn't loaded (s_notes_layer NULL).
static void render_notes_overlay_content(void) {
  if (!s_notes_layer) {
    return;
  }
  // Tags line: a fixed header outside the ScrollLayer, staying put while the
  // body scrolls. Shown for any task subject (untagged -> NOTES_TAGS_EMPTY_TEXT),
  // zero-height for a project subject. Measured against the padded width so the
  // wrap point matches what's drawn.
  int16_t tags_height = 0;
  if (!s_notes_overlay_is_project) {
    int16_t label_h, names_h;
    measure_notes_tags_parts(s_notes_content_bounds.size.w - NOTES_TAGS_PADDING_X * 2, &label_h, &names_h);
    tags_height = label_h + names_h + NOTES_TAGS_PADDING_Y * 2;
  }
  layer_set_frame(s_notes_tags_layer,
                   GRect(s_notes_content_bounds.origin.x, s_notes_content_bounds.origin.y,
                         s_notes_content_bounds.size.w, tags_height));
  layer_mark_dirty(s_notes_tags_layer);

  // The scroll area starts right below the tags header (or right at the top
  // when there's no header) and shrinks to make room for it.
  GRect scroll_frame = GRect(s_notes_content_bounds.origin.x, s_notes_content_bounds.origin.y + tags_height,
                              s_notes_content_bounds.size.w, s_notes_content_bounds.size.h - tags_height);
  layer_set_frame(scroll_layer_get_layer(s_notes_scroll_layer), scroll_frame);

  text_layer_set_text(s_notes_layer, s_notes_display_text);
  // The new text can be a very different length - resize the scroll content and
  // snap back to the top rather than leaving the position mid-way through text
  // that may no longer be there.
  int16_t text_height = measure_notes_text_height(scroll_frame.size.w, s_notes_display_text);
  if (text_height < scroll_frame.size.h) {
    text_height = scroll_frame.size.h;
  }
  GRect text_frame = layer_get_frame(text_layer_get_layer(s_notes_layer));
  text_frame.origin.y = 0;
  text_frame.size.h = text_height;
  layer_set_frame(text_layer_get_layer(s_notes_layer), text_frame);
  scroll_layer_set_content_size(s_notes_scroll_layer, GSize(scroll_frame.size.w, text_height));
  scroll_layer_set_content_offset(s_notes_scroll_layer, GPointZero, false);
}

// Frees s_notes_full_text (free(NULL) is fine) and resets the fetch state for a
// new request. Does NOT touch s_notes_display_text - callers set that next.
static void reset_notes_full_buffer(void) {
  free(s_notes_full_text);
  s_notes_full_text = NULL;
  s_notes_full_len = 0;
  s_notes_full_capacity = 0;
  s_notes_fetch_state = NOTES_FETCH_IDLE;
}

static void cancel_notes_load_timeout(void) {
  if (s_notes_load_timeout_timer) {
    app_timer_cancel(s_notes_load_timeout_timer);
    s_notes_load_timeout_timer = NULL;
  }
}

// Fires when a note fetch is unanswered for NOTES_LOAD_TIMEOUT_MS. Only acts if
// the overlay is still open and still waiting.
static void notes_load_timeout_callback(void *data) {
  s_notes_load_timeout_timer = NULL;
  if (s_notes_overlay_active && s_notes_is_loading) {
    s_notes_is_loading = false;
    s_notes_display_text = NOTES_TIMEOUT_TEXT;
    render_notes_overlay_content();
  }
}

static void start_notes_load_timeout(void) {
  cancel_notes_load_timeout();
  s_notes_load_timeout_timer = app_timer_register(NOTES_LOAD_TIMEOUT_MS, notes_load_timeout_callback, NULL);
}

// Asks the phone for this subject's full notes (MSG_NOTE_REQUEST /
// MSG_PROJECT_NOTE_REQUEST) - the SYNC_START/CHUNK/SYNC_END reply is matched
// back to s_notes_overlay_subject_id, not to a stale in-flight request.
static void request_notes_full(const char *id, bool is_project) {
  begin_send(is_project ? MSG_PROJECT_NOTE_REQUEST : MSG_NOTE_REQUEST, id, NULL, 0);
  start_notes_load_timeout();
}

// Shared by show_notes_overlay (task) and show_project_notes_overlay - same
// pushed Window, different subject id/fetch type. Also a live refresh while the
// window is already open (a successful append re-triggers this): with
// s_notes_layer non-NULL it re-renders instead of pushing a second copy.
static void show_notes_overlay_for(const char *id, bool is_project) {
  s_notes_overlay_active = true;
  s_notes_overlay_is_project = is_project;
  str_copy(s_notes_overlay_subject_id, id, MAX_ID_LEN);
  reset_notes_full_buffer();
  s_notes_display_text = NOTES_LOADING_TEXT;
  s_notes_is_loading = true;
  request_notes_full(id, is_project);
  if (s_notes_layer) {
    render_notes_overlay_content();
    return;
  }
  push_notes_window();
}

// Shows a task's notes (double-click Select). s_notes_tags_line is set here
// synchronously (tags are sent with every task) so it stays visible above the
// notes whatever the fetch resolves to.
static void show_notes_overlay(Task *task) {
  str_copy(s_notes_tags_line, task->tags, sizeof(s_notes_tags_line));
  show_notes_overlay_for(task->id, false);
}

// Shows a project's notes (double-click Select on its project row). See
// MSG_PROJECT_NOTE_APPEND for what "a project's notes" means. No tags line -
// no tags-on-project concept.
static void show_project_notes_overlay(TaskGroup *group) {
  s_notes_tags_line[0] = '\0';
  show_notes_overlay_for(group->project_id, true);
}

// Dismisses the notes overlay (Select) - notes_window_unload clears
// s_notes_overlay_active on the pop, same as a Back-triggered dismissal, so
// both paths share the cleanup.
static void hide_notes_overlay(void) {
  window_stack_pop(true);
}

#ifdef PBL_PLATFORM_EMERY
// Opens (or restarts) the "Marking done..." cancel window for a task by id. The
// task is not marked done until pending_done_commit_callback fires; a Select in
// the meantime calls cancel_pending_done.
static void begin_pending_done(const char *task_id) {
  if (s_pending_done_timer) {
    app_timer_cancel(s_pending_done_timer);
  }
  str_copy(s_pending_done_task_id, task_id, MAX_ID_LEN);
  s_pending_done_tick = 0;
  s_pending_done_timer = app_timer_register(DONE_WINDOW_MS, pending_done_commit_callback, NULL);
  vibes_short_pulse();
  if (s_menu_layer) {
    menu_layer_reload_data(s_menu_layer);
  }
  refresh_scroll_state(false);
}

// Cancel window elapsed: mark the task done for real and sync it.
static void pending_done_commit_callback(void *data) {
  s_pending_done_timer = NULL;
  Task *task = find_task_by_id(s_pending_done_task_id);
  s_pending_done_task_id[0] = '\0';
  s_pending_done_tick = 0;
  if (task && !task->done) {
    task->done = true;
    save_tasks();
    send_task_toggle(task);
    vibes_short_pulse(); // the cancel window elapsed silently - confirm the commit
  }
  if (s_menu_layer) {
    menu_layer_reload_data(s_menu_layer);
  }
  refresh_scroll_state(false);
}

// Select pressed inside the window - drop it, the task stays not-done.
static void cancel_pending_done(void) {
  if (s_pending_done_timer) {
    app_timer_cancel(s_pending_done_timer);
    s_pending_done_timer = NULL;
  }
  s_pending_done_task_id[0] = '\0';
  s_pending_done_tick = 0;
  if (s_menu_layer) {
    menu_layer_reload_data(s_menu_layer);
  }
  refresh_scroll_state(false);
}
#endif

// The pending-reschedule subtitle rides draw_task_row, which both the today
// list and the Projects browser task view use - redraw whichever menus exist.
static void reschedule_menus_reload(void) {
  if (s_menu_layer) {
    menu_layer_reload_data(s_menu_layer);
  }
#if PROJECTS_BROWSER
  if (s_browse_menu) {
    menu_layer_reload_data(s_browse_menu);
  }
#endif
}

// Clears a pending schedule-today / tomorrow / unschedule (Select pressed
// within the window, or the window's own task vanished) and redraws so the
// subtitle reverts.
static void cancel_pending_reschedule(void) {
  if (s_pending_reschedule_timer) {
    app_timer_cancel(s_pending_reschedule_timer);
    s_pending_reschedule_timer = NULL;
  }
  s_pending_reschedule_kind = RESCHEDULE_NONE;
  s_pending_reschedule_tick = 0;
  s_pending_reschedule_task_id[0] = '\0';
  s_pending_reschedule_project_id[0] = '\0';
  reschedule_menus_reload();
  refresh_scroll_state(false);
}

// Commits the pending reschedule once its cancel window passes. Sends by the
// stashed id (the phone ignores a since-deleted task) - the task need not be
// in s_tasks, since a browser task usually isn't.
static void pending_reschedule_timer_callback(void *data) {
  s_pending_reschedule_timer = NULL;
  RescheduleKind kind = s_pending_reschedule_kind;
  s_pending_reschedule_kind = RESCHEDULE_NONE;
#ifdef PBL_PLATFORM_EMERY
  if (kind == RESCHEDULE_AT && s_pending_reschedule_task_id[0] != '\0') {
    send_task_set_due_time(s_pending_reschedule_task_id, s_pending_reschedule_at_hour);
  } else
#endif
  if (kind != RESCHEDULE_NONE && s_pending_reschedule_task_id[0] != '\0') {
    send_task_reschedule(s_pending_reschedule_task_id, kind, s_pending_reschedule_project_id);
    vibes_short_pulse(); // the cancel window elapsed silently - confirm the commit
  }
  s_pending_reschedule_task_id[0] = '\0';
  s_pending_reschedule_project_id[0] = '\0';
  s_pending_reschedule_tick = 0;
  // Drop the pending subtitle now; the phone's list push handles the rest.
  reschedule_menus_reload();
  refresh_scroll_state(false);
}

// Starts (or replaces) the pending reschedule for the currently-selected task -
// on the today list or the Projects browser's task view, whichever is on top.
// Long-press Up = schedule today (browser) / unschedule (today list), long-
// press Down = tomorrow on both.
static void begin_pending_reschedule(RescheduleKind kind) {
  if (s_error_overlay_active || kind == RESCHEDULE_NONE) {
    return;
  }
  const char *task_id = NULL;
  s_pending_reschedule_project_id[0] = '\0';
#if PROJECTS_BROWSER
  if (window_stack_get_top_window() == s_browse_window && s_browse_level == 1 && s_browse_menu) {
    Task *bt = resolve_browse_task_at(menu_layer_get_selected_index(s_browse_menu));
    if (bt) {
      task_id = bt->id;
      // Only a real project id round-trips (the phone re-pushes that project's
      // list). In tags mode s_browse_project_id is a tag - leave it empty so the
      // phone re-pushes the today list instead.
      if (!browse_wants_tags()) {
        str_copy(s_pending_reschedule_project_id, s_browse_project_id, MAX_PROJECT_ID_LEN);
      }
    }
  } else
#endif
  {
    if (s_task_count == 0) {
      return;
    }
    Task *task = resolve_selected_task();
    if (task) {
      task_id = task->id; // NULL on a pinned/project/action row
    }
  }
  if (!task_id) {
    return;
  }
#ifdef PBL_PLATFORM_EMERY
  // A pending "Marking done..." window on the same task would otherwise commit
  // mid-window - drop it in favour of this reschedule.
  if (s_pending_done_timer) {
    app_timer_cancel(s_pending_done_timer);
    s_pending_done_timer = NULL;
    s_pending_done_task_id[0] = '\0';
    s_pending_done_tick = 0;
  }
#endif
  if (s_pending_reschedule_timer) {
    app_timer_cancel(s_pending_reschedule_timer);
  }
  str_copy(s_pending_reschedule_task_id, task_id, MAX_ID_LEN);
  s_pending_reschedule_kind = kind;
  s_pending_reschedule_tick = 0;
  s_pending_reschedule_timer =
      app_timer_register(RESCHEDULE_WINDOW_MS, pending_reschedule_timer_callback, NULL);
  vibes_short_pulse();
  reschedule_menus_reload();
  refresh_scroll_state(false);
}

// True if a NOTE_SYNC_* reply for id/is_project is about what the notes overlay
// is currently showing, not a stale reply for a subject the user backed out of.
static bool notes_reply_matches(const char *id, bool is_project) {
  return is_project == s_notes_overlay_is_project &&
         strncmp(id, s_notes_overlay_subject_id, MAX_ID_LEN) == 0;
}

// Shared by the MSG_NOTE_SYNC_START / MSG_PROJECT_NOTE_SYNC_START handlers.
static void handle_notes_sync_start(const char *id, bool is_project, int32_t total_len) {
  if (!notes_reply_matches(id, is_project)) {
    return;
  }
  reset_notes_full_buffer();
  if (total_len <= 0) {
    s_notes_fetch_state = NOTES_FETCH_EMPTY;
    return; // No chunks will follow - a SYNC_END will render the empty-notes text.
  }
  s_notes_full_capacity = (int)strlen(NOTES_HEADER) + (int)total_len + 1;
  s_notes_full_text = malloc((size_t)s_notes_full_capacity);
  if (!s_notes_full_text) {
    s_notes_full_capacity = 0;
    s_notes_fetch_state = NOTES_FETCH_FAILED;
    return;
  }
  memcpy(s_notes_full_text, NOTES_HEADER, strlen(NOTES_HEADER));
  s_notes_full_len = (int)strlen(NOTES_HEADER);
  s_notes_fetch_state = NOTES_FETCH_STARTED;
}

// Shared by MSG_NOTE_CHUNK/MSG_PROJECT_NOTE_CHUNK below.
static void handle_notes_chunk(const char *id, bool is_project, const char *chunk) {
  if (s_notes_fetch_state != NOTES_FETCH_STARTED || !notes_reply_matches(id, is_project)) {
    return;
  }
  int chunk_len = (int)strlen(chunk);
  // Bounds-checked against the capacity SYNC_START malloc'd - a chunk that would
  // overflow (a phone/watch length mismatch) is dropped, not overrun.
  if (s_notes_full_len + chunk_len < s_notes_full_capacity) {
    memcpy(s_notes_full_text + s_notes_full_len, chunk, (size_t)chunk_len);
    s_notes_full_len += chunk_len;
  }
}

// Shared by MSG_NOTE_SYNC_END/MSG_PROJECT_NOTE_SYNC_END below.
static void handle_notes_sync_end(const char *id, bool is_project) {
  if (!notes_reply_matches(id, is_project)) {
    return;
  }
  cancel_notes_load_timeout();
  s_notes_is_loading = false;
  switch (s_notes_fetch_state) {
    case NOTES_FETCH_STARTED:
      s_notes_full_text[s_notes_full_len] = '\0';
      s_notes_display_text = s_notes_full_text;
      break;
    case NOTES_FETCH_EMPTY:
      s_notes_display_text = is_project ? PROJECT_NOTES_EMPTY_TEXT : NOTES_EMPTY_TEXT;
      break;
    case NOTES_FETCH_FAILED:
    case NOTES_FETCH_IDLE:
    default:
      // malloc failed, or SYNC_START never arrived - a real failure, not a
      // legitimately empty note (NOTES_FETCH_EMPTY, above).
      s_notes_display_text = NOTES_TIMEOUT_TEXT;
      break;
  }
  render_notes_overlay_content();
}
#endif
