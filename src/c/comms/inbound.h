// Phone -> watch: AppMessage inbox handler and per-message parsers.
// Textually #included by main.c (one translation unit) - not compiled alone.

// ---------- AppMessage ----------

static void request_sync(void) {
#ifdef PBL_PLATFORM_APLITE
  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) != APP_MSG_OK) {
    return;
  }
  dict_write_int32(iter, KEY_MSG_TYPE, MSG_REQUEST_SYNC);
  app_message_outbox_send();
#else
  begin_send(MSG_REQUEST_SYNC, NULL, NULL, 0);
#endif
}

#ifndef PBL_PLATFORM_APLITE
// Asks the phone to stop the live-tracking session it's currently showing us.
// No keys - the phone holds the session id (CAS-guarded on its side).
static void send_presence_stop(void) {
  begin_send(MSG_PRESENCE_STOP, NULL, NULL, 0);
}
#endif

#if PROJECTS_BROWSER
// Projects browser: ask the phone for the project list / one project's task
// list. Both replies are chunked (START / ITEM* / END) - see the MSG_PROJECT_*
// handlers in inbox_received_handler.
// int_val carries IS_TAGS (1 for any tag-flavoured mode) - see send_pending_retry
// and the phone's handleProjectListRequest / handleProjectTasksRequest. In
// BROWSE_TAG_EDIT the str_val is the task id whose tags are being edited (the
// phone then marks each tag assigned/not in PROJECT_TASK_COUNT).
static void request_project_list(void) {
  const char *edit_id = (s_browse_mode == BROWSE_TAG_EDIT && s_browse_edit_task_id[0] != '\0')
                            ? s_browse_edit_task_id : NULL;
  begin_send(MSG_PROJECT_LIST_REQUEST, edit_id, NULL, browse_wants_tags() ? 1 : 0);
}

static void request_project_tasks(const char *project_id) {
  begin_send(MSG_PROJECT_TASKS_REQUEST, project_id, NULL, browse_wants_tags() ? 1 : 0);
}
#endif

// The single place s_status_code is assigned (from MSG_SYNC_STATUS and from
// outbox_failed_handler's local STATUS_ERROR).
//
// Forcing the backlight on during a sync is disabled (#if 0, not //, because
// the block contains its own #ifndef/#else the preprocessor would still see).
// Re-enable by flipping the 0 to 1.
#ifndef PBL_PLATFORM_APLITE
// Background sync: each settings push re-arms one wakeup N minutes out; the
// wakeup relaunches the app, its JS syncs, and set_status_code() exits once that
// finishes - which re-arms the next one via that launch's own settings push.
static bool s_wakeup_launch = false;
static void schedule_next_wakeup(int min) {
  wakeup_cancel_all();
  if (min > 0) {
    wakeup_schedule(time(NULL) + min * 60, 0, false);
  }
}
#endif

static void set_status_code(int32_t new_status_code) {
#if 0
  bool was_syncing = (s_status_code == STATUS_SYNCING);
  bool now_syncing = (new_status_code == STATUS_SYNCING);
  if (now_syncing && !was_syncing) {
    light_enable(true);
  } else if (!now_syncing && was_syncing) {
#ifndef PBL_PLATFORM_APLITE
    if (s_backlight_mode != BACKLIGHT_MODE_ALWAYS_ON) {
      light_enable(false);
    }
#else
    light_enable(false);
#endif
  }
#endif
  // A sync just finished cleanly - stamp the time (shown as "Synced Nm ago" on
  // the Resync row) and, on emery, flash that row green.
  if (s_status_code == STATUS_SYNCING && new_status_code == STATUS_OK) {
#ifndef PBL_PLATFORM_APLITE
    s_last_sync_epoch = time(NULL);
    persist_write_int(PERSIST_KEY_LAST_SYNC, (int)s_last_sync_epoch);
#endif
#ifdef PBL_PLATFORM_EMERY
    s_sync_check_active = true;
    s_sync_check_tick = 0;
    if (s_menu_layer) {
      menu_layer_reload_data(s_menu_layer);
      refresh_scroll_state(false);
    }
#endif
  }
  s_status_code = new_status_code;
#ifndef PBL_PLATFORM_APLITE
  if (s_wakeup_launch && new_status_code != STATUS_SYNCING) {
    window_stack_pop_all(true);
  }
#endif
}

// The TASK_* fields shared by MSG_TASK_ITEM and MSG_PROJECT_TASKS_ITEM. The
// caller has already validated id_tuple / title_tuple non-NULL and bounds-
// checked idx; MSG_TASK_ITEM fills in project/tags/colour separately.
static void parse_common_task_fields(DictionaryIterator *it, Task *dst,
                                     Tuple *id_tuple, Tuple *title_tuple) {
  str_copy(dst->id, id_tuple->value->cstring, MAX_ID_LEN);
  str_copy(dst->title, title_tuple->value->cstring, MAX_TITLE_LEN);
  dst->done = tuple_int(it, KEY_TASK_DONE, 0) != 0;
  dst->due_min = tuple_int(it, KEY_TASK_DUE_MIN, -1);
  dst->time_spent_ms = tuple_int(it, KEY_TASK_TIME_SPENT_MS, 0);
  dst->time_estimate_ms = tuple_int(it, KEY_TASK_TIME_ESTIMATE_MS, 0);
  dst->deadline_days = tuple_int(it, KEY_TASK_DEADLINE_DAYS, DEADLINE_NONE);
  dst->recurs = tuple_int(it, KEY_TASK_RECURS, 0) != 0;
  dst->is_subtask = tuple_int(it, KEY_TASK_IS_SUBTASK, 0) != 0;
#ifndef PBL_PLATFORM_APLITE
  dst->remind_min = tuple_int(it, KEY_TASK_REMIND_MIN, -1);
  dst->remind_fired = false;
  str_copy(dst->issue_key, tuple_str(it, KEY_TASK_ISSUE_KEY, ""), sizeof(dst->issue_key));
  dst->issue_src = (uint8_t)tuple_int(it, KEY_TASK_ISSUE_SRC, ISSUE_SRC_NONE);
  dst->priority = (uint8_t)tuple_int(it, KEY_TASK_PRIORITY, PRIO_NONE);
#endif
}

static void inbox_received_handler(DictionaryIterator *iterator, void *context) {
  Tuple *type_tuple = dict_find(iterator, KEY_MSG_TYPE);
  if (!type_tuple) {
    return;
  }

  switch (type_tuple->value->int32) {
    case MSG_TASK_SYNC_START: {
      s_incoming_total = tuple_int(iterator, KEY_TASK_TOTAL, 0);
      if (s_incoming_total > MAX_TASKS) {
        s_incoming_total = MAX_TASKS;
      }
#if INCOMING_MALLOCED
      // A previous batch that never reached SYNC_END would have leaked it.
      if (s_incoming && s_incoming != s_tasks) {
        free(s_incoming);
      }
      s_incoming = malloc(sizeof(Task) * MAX_TASKS);
      if (!s_incoming) {
        s_incoming = s_tasks; // OOM: parse in place, accept a brief torn list
      }
#endif
      set_status_code(STATUS_SYNCING);
      break;
    }
    case MSG_TASK_ITEM: {
      Tuple *idx_tuple = dict_find(iterator, KEY_TASK_INDEX);
      Tuple *id_tuple = dict_find(iterator, KEY_TASK_ID);
      Tuple *title_tuple = dict_find(iterator, KEY_TASK_TITLE);
      if (!idx_tuple || !id_tuple || !title_tuple) {
        break;
      }
#if INCOMING_MALLOCED
      if (!s_incoming) {
        break; // an ITEM with no preceding SYNC_START (stale batch)
      }
#endif
      int idx = idx_tuple->value->int32;
      if (idx < 0 || idx >= MAX_TASKS) {
        break;
      }
      // Key absent (not 0, a valid 12:00am) means "no dueWithTime" - the phone
      // only sends a field when the task has it. parse_common_task_fields
      // handles id/title/done/due/spent/estimate; project/tags/colour here.
      parse_common_task_fields(iterator, &s_incoming[idx], id_tuple, title_tuple);
      str_copy(s_incoming[idx].project, tuple_str(iterator, KEY_TASK_PROJECT, ""), MAX_PROJECT_LEN);
#ifndef PBL_PLATFORM_APLITE
      str_copy(s_incoming[idx].project_id, tuple_str(iterator, KEY_TASK_PROJECT_ID, ""), MAX_PROJECT_ID_LEN);
      str_copy(s_incoming[idx].tags, tuple_str(iterator, KEY_TASK_TAGS, ""), MAX_TASK_TAGS_LEN);
#if TODAY_PROJECT_SWATCH
      s_incoming[idx].project_color = (uint8_t)tuple_int(iterator, KEY_TASK_PROJECT_COLOR, 0);
#endif
#endif
      break;
    }
    case MSG_TASK_SYNC_END: {
      int count = s_incoming_total < MAX_TASKS ? s_incoming_total : MAX_TASKS;
#ifndef PBL_PLATFORM_APLITE
      // Carry each task's "reminder already fired" and collapsed flags across
      // the rebuild - parse_common_task_fields zeroes them, and with the phone
      // re-pushing the list often (a tracked-task time update, presence) that
      // re-fired the banner every push. s_tasks still holds the old list here;
      // s_incoming the new one (unless the OOM path parsed in place).
      if (s_incoming && s_incoming != s_tasks) {
        for (int i = 0; i < count; i++) {
          for (int j = 0; j < s_task_count; j++) {
            if (strncmp(s_tasks[j].id, s_incoming[i].id, MAX_ID_LEN) == 0) {
              s_incoming[i].remind_fired = s_tasks[j].remind_fired;
#ifdef PBL_PLATFORM_EMERY
              s_incoming[i].collapsed = s_tasks[j].collapsed;
#endif
              break;
            }
          }
        }
      }
#endif
#if INCOMING_MALLOCED
      bool committed = (s_incoming == s_tasks); // OOM path parsed in place
      if (s_incoming && s_incoming != s_tasks) {
        memcpy(s_tasks, s_incoming, sizeof(Task) * (size_t)count);
        free(s_incoming);
        committed = true;
      }
      s_incoming = NULL;
      if (committed) {
        s_task_count = count; // a stale END with no active batch leaves the list alone
      }
#else
      memcpy(s_tasks, s_incoming, sizeof(Task) * (size_t)count);
      s_task_count = count;
#endif
      set_status_code(STATUS_OK);
      recompute_groups();
      save_tasks();
#ifdef PBL_PLATFORM_EMERY
      if (!s_group_headers_have_data && s_task_count > 0) {
        header_begin_reveal(); // first data since open - wipe the group bars in
      }
      s_group_headers_have_data = true;
#endif
#ifndef PBL_PLATFORM_APLITE
      // A local tracking session whose task is gone from the synced list - even
      // though the phone force-includes any real tracked task (watchTaskList /
      // handleTrackStart). The task was deleted elsewhere, or an old build left
      // a session that never got a matching task. Clear the dead session: while
      // s_tracking_task_id is set the watch counts as "tracking locally", which
      // keeps a remote presence session off the pinned "TRACKING" section and
      // on the old dark-blue LIVE row (see remote_in_pinned_section). 0-delta
      // stop - no time to report, just end the "Tracking on Pebble" broadcast.
      if (s_tracking_task_id[0] != '\0' && find_task_by_id(s_tracking_task_id) == NULL) {
        send_track_time_stop(s_tracking_task_id, 0);
        s_tracking_task_id[0] = '\0';
        s_tracking_start_epoch = 0;
        save_tracking();
        s_overtime_notified = false;
        stop_tracking_tick();
        if (s_presence_state == 1) {
          start_tracking_tick(); // now drives the remote row's live elapsed
        }
      }
      // If the pinned task is gone from the list and nothing's being tracked,
      // drop the stale pin so it can't spuriously re-appear.
      if (s_pinned_task_id[0] != '\0' && s_tracking_task_id[0] == '\0' &&
          find_task_by_id(s_pinned_task_id) == NULL) {
        cancel_unpin_timer();
        s_pinned_task_id[0] = '\0';
      }
#endif
      menu_layer_reload_data(s_menu_layer);
      update_empty_layer();
      refresh_scroll_state(true); // the selected row may now be different
#ifndef PBL_PLATFORM_APLITE
      schedule_refresh_if_open(); // rebuild the Schedule sub-page off the new list
#endif
      break;
    }
    case MSG_HABIT_SYNC_START: {
      s_habit_incoming_total = tuple_int(iterator, KEY_HABIT_TOTAL, 0);
      if (s_habit_incoming_total > MAX_HABITS) {
        s_habit_incoming_total = MAX_HABITS;
      }
      break;
    }
    case MSG_HABIT_ITEM: {
      Tuple *idx_tuple = dict_find(iterator, KEY_HABIT_INDEX);
      Tuple *id_tuple = dict_find(iterator, KEY_HABIT_ID);
      Tuple *title_tuple = dict_find(iterator, KEY_HABIT_TITLE);
      if (!idx_tuple || !id_tuple || !title_tuple) {
        break;
      }
      int idx = idx_tuple->value->int32;
      if (idx < 0 || idx >= MAX_HABITS) {
        break;
      }
      // Written directly into s_habits (no separate incoming buffer - safe, see
      // the Habit struct comment).
      str_copy(s_habits[idx].id, id_tuple->value->cstring, MAX_HABIT_ID_LEN);
      str_copy(s_habits[idx].title, title_tuple->value->cstring, MAX_TITLE_LEN);
      s_habits[idx].done = tuple_int(iterator, KEY_HABIT_DONE, 0) != 0;
      s_habits[idx].value = tuple_int(iterator, KEY_HABIT_VALUE, 0);
      s_habits[idx].goal = tuple_int(iterator, KEY_HABIT_GOAL, 0);
      // habit type: 0 = ClickCounter, 1 = StopWatch, 2 = RepeatedCountdownReminder.
      int habit_type = tuple_int(iterator, KEY_HABIT_TYPE, 0);
#ifdef PBL_PLATFORM_APLITE
      // aplite has no code budget for the countdown path, and both timer types
      // are excluded from its visible list identically, so collapse both into
      // is_stopwatch (is_countdown is never read there).
      s_habits[idx].is_stopwatch = habit_type != 0;
#else
      s_habits[idx].is_stopwatch = habit_type == 1;
      s_habits[idx].is_countdown = habit_type == 2;
      s_habits[idx].countdown_ms = tuple_int(iterator, KEY_HABIT_COUNTDOWN_MS, 0);
#ifdef PBL_PLATFORM_EMERY
      {
        int old_streak = s_habits[idx].streak;
        int new_streak = tuple_int(iterator, KEY_HABIT_STREAK, 0);
        s_habits[idx].streak = new_streak;
        // Exact day-N crossing only, so a cold load (old 0 -> N) never pops.
        if ((old_streak == 6 && new_streak == 7) ||
            (old_streak == 29 && new_streak == 30) ||
            (old_streak == 99 && new_streak == 100)) {
          str_copy(s_habit_milestone_id, s_habits[idx].id, MAX_HABIT_ID_LEN);
        } else if (new_streak == old_streak + 1 && old_streak >= 2) {
          // An ordinary +1 (not a milestone crossing) - roll the label up one.
          str_copy(s_streak_roll_id, s_habits[idx].id, MAX_HABIT_ID_LEN);
          snprintf(s_streak_roll_prev, sizeof(s_streak_roll_prev), "%d streak", old_streak);
        }
      }
#else
      s_habits[idx].streak = tuple_int(iterator, KEY_HABIT_STREAK, 0);
#endif
      s_habits[idx].best_streak = tuple_int(iterator, KEY_HABIT_BEST_STREAK, 0);
#endif
      break;
    }
    case MSG_HABIT_SYNC_END: {
      s_habit_count = s_habit_incoming_total;
      save_habits();
      if (s_habits_menu_layer) {
        menu_layer_reload_data(s_habits_menu_layer);
      }
      if (s_habits_empty_layer) {
        update_habits_empty_layer();
      }
#ifdef PBL_PLATFORM_EMERY
      // A streak just hit 7 / 30 / 100 in this batch - gold-pop that row, but
      // only if the list is actually on screen to see it.
      if (s_habit_milestone_id[0] != '\0') {
        if (s_habits_menu_layer) {
          begin_habit_flash(s_habit_milestone_id, true);
        }
        s_habit_milestone_id[0] = '\0';
      }
      // An ordinary streak +1 landed - roll its label now that the row data is
      // in place (only worthwhile if the list is on screen).
      if (s_streak_roll_id[0] != '\0') {
        if (s_habits_menu_layer) {
          begin_streak_roll();
        } else {
          s_streak_roll_id[0] = '\0';
        }
      }
      if (s_habits_menu_layer) {
        habits_marquee_refresh(false); // the selected title may now overflow
      }
#endif
      break;
    }
#if PROJECTS_BROWSER
    case MSG_PROJECT_LIST_START: {
      if (!s_browse_projects) {
        break;
      }
      // Drop a reply for the other mode (the user switched Projects<->Tags
      // while this list was in flight).
      if ((tuple_int(iterator, KEY_IS_TAGS, 0) != 0) != browse_wants_tags()) {
        break;
      }
      s_browse_project_incoming = tuple_int(iterator, KEY_PROJECT_TOTAL, 0);
      if (s_browse_project_incoming > MAX_BROWSE_PROJECTS) {
        s_browse_project_incoming = MAX_BROWSE_PROJECTS;
      }
      break;
    }
    case MSG_PROJECT_LIST_ITEM: {
      if (!s_browse_projects) {
        break;
      }
      Tuple *idx_tuple = dict_find(iterator, KEY_PROJECT_INDEX);
      Tuple *id_tuple = dict_find(iterator, KEY_PROJECT_ID);
      Tuple *title_tuple = dict_find(iterator, KEY_PROJECT_TITLE);
      if (!idx_tuple || !id_tuple || !title_tuple) {
        break;
      }
      int idx = idx_tuple->value->int32;
      if (idx < 0 || idx >= MAX_BROWSE_PROJECTS) {
        break;
      }
      str_copy(s_browse_projects[idx].id, id_tuple->value->cstring, MAX_PROJECT_ID_LEN);
      str_copy(s_browse_projects[idx].title, title_tuple->value->cstring, MAX_TITLE_LEN);
      s_browse_projects[idx].color = tuple_int(iterator, KEY_PROJECT_COLOR, 0);
      s_browse_projects[idx].task_count = tuple_int(iterator, KEY_PROJECT_TASK_COUNT, 0);
      break;
    }
    case MSG_PROJECT_LIST_END: {
      if (!s_browse_projects) {
        break;
      }
      s_browse_project_count = s_browse_project_incoming;
      s_browse_projects_loading = false;
#if PROJECTS_CACHE
      if (s_browse_mode == BROWSE_PROJECTS) {
        save_browse_projects(); // the tag list isn't cached - always fetched
      }
#endif
      if (s_browse_menu && s_browse_level == 0) {
        menu_layer_reload_data(s_browse_menu);
        browse_update_empty();
      }
      break;
    }
    case MSG_PROJECT_TASKS_START: {
      Tuple *pid_tuple = dict_find(iterator, KEY_PROJECT_ID);
      // A reply for a project the user has already navigated away from - drop it.
      if (!s_browse_tasks || !pid_tuple ||
          strncmp(pid_tuple->value->cstring, s_browse_project_id, MAX_PROJECT_ID_LEN) != 0) {
        break;
      }
      s_browse_task_incoming = tuple_int(iterator, KEY_TASK_TOTAL, 0);
      if (s_browse_task_incoming > MAX_BROWSE_TASKS) {
        s_browse_task_incoming = MAX_BROWSE_TASKS;
      }
      // Regular list first, then backlog - the boundary is the lowest index
      // carrying PROJECT_TASK_BACKLOG=1 (see the ITEM handler). Start it past
      // the end so "no backlog" leaves every row in the regular section.
      s_browse_backlog_start = s_browse_task_incoming;
      break;
    }
    case MSG_PROJECT_TASKS_ITEM: {
      Tuple *pid_tuple = dict_find(iterator, KEY_PROJECT_ID);
      if (!s_browse_tasks || !pid_tuple ||
          strncmp(pid_tuple->value->cstring, s_browse_project_id, MAX_PROJECT_ID_LEN) != 0) {
        break;
      }
      Tuple *idx_tuple = dict_find(iterator, KEY_TASK_INDEX);
      Tuple *id_tuple = dict_find(iterator, KEY_TASK_ID);
      Tuple *title_tuple = dict_find(iterator, KEY_TASK_TITLE);
      if (!idx_tuple || !id_tuple || !title_tuple) {
        break;
      }
      int idx = idx_tuple->value->int32;
      if (idx < 0 || idx >= MAX_BROWSE_TASKS) {
        break;
      }
      Task *bt = &s_browse_tasks[idx];
      memset(bt, 0, sizeof(Task));
      parse_common_task_fields(iterator, bt, id_tuple, title_tuple);
      // Project display name - shown right-aligned in the Tags view (a tag's
      // tasks span projects); redundant but harmless in the Projects view.
      str_copy(bt->project, tuple_str(iterator, KEY_TASK_PROJECT, ""), MAX_PROJECT_LEN);
      if (tuple_int(iterator, KEY_PROJECT_TASK_BACKLOG, 0) != 0 && idx < s_browse_backlog_start) {
        s_browse_backlog_start = idx;
      }
      break;
    }
    case MSG_PROJECT_TASKS_END: {
      Tuple *pid_tuple = dict_find(iterator, KEY_PROJECT_ID);
      if (!s_browse_tasks || !pid_tuple ||
          strncmp(pid_tuple->value->cstring, s_browse_project_id, MAX_PROJECT_ID_LEN) != 0) {
        break;
      }
      s_browse_task_count = s_browse_task_incoming;
      if (s_browse_backlog_start > s_browse_task_count) {
        s_browse_backlog_start = s_browse_task_count;
      }
      s_browse_tasks_loading = false;
      if (s_browse_menu && s_browse_level == 1) {
        menu_layer_reload_data(s_browse_menu);
        browse_update_empty();
      }
      break;
    }
#endif
#ifndef PBL_PLATFORM_APLITE
    case MSG_STATS_DATA: {
      s_stats_est_remaining_ms = tuple_int(iterator, KEY_STATS_EST_REMAINING_MS, 0);
      s_stats_worked_today_ms = tuple_int(iterator, KEY_STATS_WORKED_TODAY_MS, 0);
      s_stats_done_today = tuple_int(iterator, KEY_STATS_DONE_TODAY, 0);
      s_stats_worked_yesterday_ms = tuple_int(iterator, KEY_STATS_WORKED_YESTERDAY_MS, 0);
      s_stats_done_yesterday = tuple_int(iterator, KEY_STATS_DONE_YESTERDAY, 0);
      if (s_stats_projects) { // NULL = window closed; it'll re-request on open
        str_copy(s_stats_projects, tuple_str(iterator, KEY_STATS_TEXT, ""), STATS_TEXT_CAP);
        if (!s_stats_have_data) {
          header_begin_reveal(); // first data since open - wipe the bars in
        }
        s_stats_have_data = true;
        stats_render();
      }
      break;
    }
    case MSG_UPCOMING_DATA: {
      if (s_upcoming_text && s_page_mode == PAGE_UPCOMING) {
        str_copy(s_upcoming_text, tuple_str(iterator, KEY_UPCOMING_TEXT, ""), PAGE_TEXT_CAP);
        if (!s_upcoming_have_data) {
          header_begin_reveal(); // first data since open - wipe the day headers in
        }
        s_upcoming_have_data = true;
        upcoming_render(); // no-op if the window was closed before the reply landed
      }
      break;
    }
    case MSG_NOTESPAGE_DATA: {
      if (s_upcoming_text && s_page_mode == PAGE_NOTES) {
        str_copy(s_upcoming_text, tuple_str(iterator, KEY_NOTESPAGE_TEXT, ""), PAGE_TEXT_CAP);
        if (!s_upcoming_have_data) {
          header_begin_reveal();
        }
        s_upcoming_have_data = true;
        upcoming_render();
      }
      break;
    }
    case MSG_SEARCH_DATA: {
      if (s_upcoming_text && s_page_mode == PAGE_SEARCH) {
        str_copy(s_upcoming_text, tuple_str(iterator, KEY_UPCOMING_TEXT, ""), PAGE_TEXT_CAP);
        if (!s_upcoming_have_data) {
          header_begin_reveal();
        }
        s_upcoming_have_data = true;
        upcoming_render();
      }
      break;
    }
#ifdef PBL_PLATFORM_EMERY
    case MSG_CAL_MONTH_DATA: {
      handle_calendar_month_data(iterator);
      break;
    }
#endif
    case MSG_TASK_REPEAT_DATA: {
      handle_repeat_data(iterator);
      break;
    }
    case MSG_POMODORO_CFG: {
      int w = tuple_int(iterator, KEY_POMODORO_WORK_MIN, 0);
      int b = tuple_int(iterator, KEY_POMODORO_BREAK_MIN, 0);
      if (w > 0 && w < 600) {
        s_pomodoro_work_min = w;
      }
      if (b > 0 && b < 600) {
        s_pomodoro_break_min = b;
      }
      break;
    }
#endif
    case MSG_SYNC_STATUS: {
      Tuple *status_tuple = dict_find(iterator, KEY_STATUS_CODE);
      if (status_tuple) {
        set_status_code(status_tuple->value->int32);
      }
      str_copy(s_status_msg, tuple_str(iterator, KEY_STATUS_MSG, ""), MAX_STATUS_MSG_LEN);
      // Feature toggles from the phone's pairing settings - optional fields,
      // absent-means-unchanged (pass the current value as the fallback) so a
      // version mismatch can't reset a flag. Read before reload_data so a
      // change shows in the same redraw.
      s_habits_enabled = tuple_int(iterator, KEY_HABITS_ENABLED, s_habits_enabled) != 0;
      s_add_task_enabled = tuple_int(iterator, KEY_ADD_TASK_ENABLED, s_add_task_enabled) != 0;
      s_projects_enabled = tuple_int(iterator, KEY_PROJECTS_ENABLED, s_projects_enabled) != 0;
#ifndef PBL_PLATFORM_APLITE
      s_stats_enabled = tuple_int(iterator, KEY_STATS_ENABLED, s_stats_enabled) != 0;
      s_schedule_enabled = tuple_int(iterator, KEY_SCHEDULE_ENABLED, s_schedule_enabled) != 0;
#ifdef PBL_PLATFORM_EMERY
      s_calendar_enabled = tuple_int(iterator, KEY_CALENDAR_ENABLED, s_calendar_enabled) != 0;
#endif
      s_upcoming_enabled = tuple_int(iterator, KEY_UPCOMING_ENABLED, s_upcoming_enabled) != 0;
      s_notespage_enabled = tuple_int(iterator, KEY_NOTESPAGE_ENABLED, s_notespage_enabled) != 0;
      s_search_enabled = tuple_int(iterator, KEY_SEARCH_ENABLED, s_search_enabled) != 0;
      s_tags_enabled = tuple_int(iterator, KEY_TAGS_ENABLED, s_tags_enabled) != 0;
      s_yesterday_stats_enabled = tuple_int(iterator, KEY_YESTERDAY_STATS_ENABLED, s_yesterday_stats_enabled) != 0;
      s_later_today_enabled = tuple_int(iterator, KEY_LATER_TODAY_ENABLED, s_later_today_enabled) != 0;
      s_collapse_groups = tuple_int(iterator, KEY_COLLAPSE_GROUPS, s_collapse_groups) != 0;
#endif
      // Only re-applied when the value actually changed - this field is sent on
      // every status push (including routine background syncs), and re-triggering
      // the backlight each time would defeat a custom timeout.
#ifndef PBL_PLATFORM_APLITE
      Tuple *backlight_mode_tuple = dict_find(iterator, KEY_BACKLIGHT_MODE);
      if (backlight_mode_tuple && backlight_mode_tuple->value->int32 != s_backlight_mode) {
        s_backlight_mode = backlight_mode_tuple->value->int32;
        apply_backlight_mode();
      }
#endif
#if defined(PBL_TOUCH)
      // Only acted on when the value changed - a redundant touch_service_(un)subscribe
      // on every routine status push is wasteful.
      Tuple *touch_nav_tuple = dict_find(iterator, KEY_TOUCH_NAV_ENABLED);
      if (touch_nav_tuple && (touch_nav_tuple->value->int32 != 0) != s_touch_nav_enabled) {
        s_touch_nav_enabled = touch_nav_tuple->value->int32 != 0;
        apply_touch_nav();
      }
#endif
#ifndef PBL_PLATFORM_APLITE
      Tuple *overtime_notify_tuple = dict_find(iterator, KEY_OVERTIME_NOTIFY_ENABLED);
      if (overtime_notify_tuple) {
        s_overtime_notify_enabled = overtime_notify_tuple->value->int32 != 0;
        if (!s_overtime_notify_enabled) {
          hide_overtime_banner();
        }
      }
      // Sub-option of the above. If switched on while a task is already over but
      // the banner has already fired (s_overtime_last_notify_epoch unset), start
      // the 5-minute clock now rather than firing on the next tick.
      Tuple *overtime_repeat_tuple = dict_find(iterator, KEY_OVERTIME_REPEAT_ENABLED);
      if (overtime_repeat_tuple) {
        bool was = s_overtime_repeat_enabled;
        s_overtime_repeat_enabled = overtime_repeat_tuple->value->int32 != 0;
        if (!was && s_overtime_repeat_enabled && s_overtime_notified &&
            s_overtime_last_notify_epoch == 0) {
          s_overtime_last_notify_epoch = time(NULL);
        }
      }
      s_audible_notify = tuple_int(iterator, KEY_AUDIBLE_NOTIFICATIONS, s_audible_notify) != 0;
      s_audible_volume = tuple_int(iterator, KEY_AUDIBLE_VOLUME, s_audible_volume);
#ifdef BREAK_REMINDER
      // "Remind me to take a break" interval in minutes (0 = off). Re-sent every
      // sync like the flags above; the tally itself lives in persist.
      s_break_reminder_min = tuple_int(iterator, KEY_BREAK_REMINDER_MIN, s_break_reminder_min);
      s_idle_reminder_min = tuple_int(iterator, KEY_IDLE_REMINDER_MIN, s_idle_reminder_min);
#endif
      // "Notify before a task is due" - minutes ahead, 0 = off.
      s_due_reminder_min = tuple_int(iterator, KEY_DUE_REMINDER_MIN, s_due_reminder_min);
#ifndef PBL_PLATFORM_APLITE
      if (dict_find(iterator, KEY_SYNC_INTERVAL_MIN)) {
        schedule_next_wakeup(tuple_int(iterator, KEY_SYNC_INTERVAL_MIN, 0));
      }
#endif
      // Focus-mode session length in minutes. Only the NEXT session picks up a
      // change; a running one keeps its already-computed end time.
      s_focus_len_min = tuple_int(iterator, KEY_FOCUS_LEN_MIN, s_focus_len_min);
      {
        int ft = tuple_int(iterator, KEY_USE_POMODORO_CFG, (int)s_focus_type);
        s_focus_type = (ft < 0 || ft > FOCUS_FLOWTIME) ? FOCUS_COUNTDOWN : (FocusType)ft;
      }
      // "Stop tracking at midnight" - absent-means-unchanged.
      s_stop_at_midnight = tuple_int(iterator, KEY_STOP_AT_MIDNIGHT, s_stop_at_midnight) != 0;
      // "Nudge me about unfinished streaks" - absent-means-unchanged.
      s_habit_streak_nudge = tuple_int(iterator, KEY_HABIT_STREAK_NUDGE, s_habit_streak_nudge) != 0;
      // "Log energy on Finish Day" - absent-means-unchanged.
      s_reflect_enabled = tuple_int(iterator, KEY_REFLECT_ENABLED, s_reflect_enabled) != 0;
#endif
#ifndef PBL_PLATFORM_APLITE
      // Regroup so a Later Today toggle takes effect immediately rather than
      // waiting for the next full task sync.
      if (s_task_count > 0) {
        recompute_groups();
      }
#endif
      // reload_data refreshes the Resync row's status subtitle;
      // update_empty_layer() handles the empty screen. Both no-op while the
      // error overlay is up.
      menu_layer_reload_data(s_menu_layer);
      update_empty_layer();
      if (s_status_code == STATUS_ERROR) {
        show_error_overlay();
      }
      break;
    }
    case MSG_WIPE_CACHE: {
      // Pairing page "Wipe watch cache": forget the persisted lists, show the
      // now-empty list, and pull a fresh copy from the phone.
      clear_persisted_caches();
      if (s_menu_layer) {
        menu_layer_reload_data(s_menu_layer);
      }
      update_empty_layer();
      request_sync();
      break;
    }
#ifndef PBL_PLATFORM_APLITE
    case MSG_PRESENCE_UPDATE: {
      s_presence_state = tuple_int(iterator, KEY_PRESENCE_STATE, 0);
      if (s_presence_state == 0) {
        s_presence_task[0] = '\0';
        s_presence_device[0] = '\0';
        s_presence_can_stop = false;
        s_presence_stopping = false;
      } else {
        // A remote device just started (or switched) a session while this watch
        // is tracking locally - yield: stop and report, so the remote session
        // takes the pinned row instead of two timers running at once.
        if (s_presence_state == 1 && s_tracking_task_id[0] != '\0' &&
            tuple_int(iterator, KEY_PRESENCE_NEW_SESSION, 0) != 0) {
          stop_tracking_and_report();
        }
        str_copy(s_presence_task, tuple_str(iterator, KEY_PRESENCE_TASK_TITLE, ""), sizeof(s_presence_task));
        str_copy(s_presence_device, tuple_str(iterator, KEY_PRESENCE_DEVICE, ""), sizeof(s_presence_device));
        s_presence_elapsed_base = time(NULL) - (time_t)tuple_int(iterator, KEY_PRESENCE_ELAPSED_S, 0);
        s_presence_can_stop = tuple_int(iterator, KEY_PRESENCE_CAN_STOP, 0) != 0;
        s_presence_spent_ms = tuple_int(iterator, KEY_PRESENCE_SPENT_MS, 0);
        s_presence_estimate_ms = tuple_int(iterator, KEY_PRESENCE_ESTIMATE_MS, 0);
        s_presence_spent_received_epoch = time(NULL);
        // The phone confirms a stop by clearing (state 0), never by another
        // still-live update - so any fresh state drops the "Stopping..." latch.
        s_presence_stopping = false;
      }
      // A remote session that just started or switched task begins its own
      // over-estimate crossing - re-arm the latch and drop any banner it left
      // up. The phone flags this (PRESENCE_NEW_SESSION, a new sessionId); a
      // same-session heartbeat clears it so the repeat interval isn't reset
      // every 60s. Skipped while tracking locally - that session owns the latch.
      if (s_tracking_task_id[0] == '\0' && tuple_int(iterator, KEY_PRESENCE_NEW_SESSION, 0) != 0) {
        s_overtime_notified = false;
        s_overtime_last_notify_epoch = 0;
        hide_overtime_banner();
      }
      // reload + scroll refresh covers both the dark-blue section-0 row count
      // changing and the pinned "TRACKING" section appearing/disappearing.
      refresh_pinned_section();
      // The pinned-section remote row needs a per-second redraw for its live
      // elapsed. tracking_tick_callback just marks the menu dirty; it's a
      // no-op for the over-estimate banner when nothing is tracked locally.
      if (s_tracking_task_id[0] == '\0') {
        if (remote_in_pinned_section() && s_presence_state == 1) {
          start_tracking_tick();
        } else {
          stop_tracking_tick();
        }
      }
      live_window_refresh(); // updates or pops the detail window if it's open
      break;
    }
    case MSG_PRESENCE_STOP_LOCAL: {
      // A remote device stopped the timer this watch is running. Stop it the
      // same as a long-press would - stop_tracking_and_report() sends the
      // MSG_TRACK_TIME_STOP the phone turns into the presence "stopped" ack.
      if (s_tracking_task_id[0] != '\0') {
        stop_tracking_and_report();
        menu_layer_reload_data(s_menu_layer);
        refresh_scroll_state(true);
      }
      break;
    }
    case MSG_NOTE_SYNC_START: {
      Tuple *id_tuple = dict_find(iterator, KEY_TASK_ID);
      Tuple *total_tuple = dict_find(iterator, KEY_NOTE_TOTAL_LEN);
      if (!id_tuple || !total_tuple) {
        break;
      }
      handle_notes_sync_start(id_tuple->value->cstring, false, total_tuple->value->int32);
      break;
    }
    case MSG_NOTE_CHUNK: {
      Tuple *id_tuple = dict_find(iterator, KEY_TASK_ID);
      Tuple *chunk_tuple = dict_find(iterator, KEY_NOTE_CHUNK_TEXT);
      if (!id_tuple || !chunk_tuple) {
        break;
      }
      handle_notes_chunk(id_tuple->value->cstring, false, chunk_tuple->value->cstring);
      break;
    }
    case MSG_NOTE_SYNC_END: {
      Tuple *id_tuple = dict_find(iterator, KEY_TASK_ID);
      if (!id_tuple) {
        break;
      }
      handle_notes_sync_end(id_tuple->value->cstring, false);
      break;
    }
    case MSG_PROJECT_NOTE_SYNC_START: {
      Tuple *id_tuple = dict_find(iterator, KEY_PROJECT_ID);
      Tuple *total_tuple = dict_find(iterator, KEY_NOTE_TOTAL_LEN);
      if (!id_tuple || !total_tuple) {
        break;
      }
      handle_notes_sync_start(id_tuple->value->cstring, true, total_tuple->value->int32);
      break;
    }
    case MSG_PROJECT_NOTE_CHUNK: {
      Tuple *id_tuple = dict_find(iterator, KEY_PROJECT_ID);
      Tuple *chunk_tuple = dict_find(iterator, KEY_NOTE_CHUNK_TEXT);
      if (!id_tuple || !chunk_tuple) {
        break;
      }
      handle_notes_chunk(id_tuple->value->cstring, true, chunk_tuple->value->cstring);
      break;
    }
    case MSG_PROJECT_NOTE_SYNC_END: {
      Tuple *id_tuple = dict_find(iterator, KEY_PROJECT_ID);
      if (!id_tuple) {
        break;
      }
      handle_notes_sync_end(id_tuple->value->cstring, true);
      break;
    }
#endif
    default:
      break;
  }
}

static void inbox_dropped_handler(AppMessageResult reason, void *context) {
  (void)reason; // no log: the string cost ~30B of code space
}

#ifndef PBL_PLATFORM_APLITE
// Fires once the phone confirms delivery (not just "queued") of any outbound
// message - closes the app only when the Finish Day send just succeeded, and
// clears the retry state so a later failure can't re-send a message that went
// through.
static void outbox_sent_handler(DictionaryIterator *iterator, void *context) {
  clear_pending_retry();
  if (s_close_after_finish_day_sent) {
    s_close_after_finish_day_sent = false;
    window_stack_pop_all(true);
  }
}

// The AppMessageResult reasons that are transient (a busy/backgrounded phone or
// BT link) and worth a short retry. Excludes structural problems with the
// call/message itself, which would just fail identically.
static bool is_retryable_failure(AppMessageResult reason) {
  switch (reason) {
    case APP_MSG_SEND_TIMEOUT:
    case APP_MSG_SEND_REJECTED:
    case APP_MSG_NOT_CONNECTED:
    case APP_MSG_APP_NOT_RUNNING:
    case APP_MSG_BUSY:
    case APP_MSG_BUFFER_OVERFLOW:
      return true;
    default:
      return false;
  }
}

static void retry_timer_callback(void *data) {
  s_retry_timer = NULL;
  send_pending_retry();
}
#endif

static void outbox_failed_handler(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
#ifndef PBL_PLATFORM_APLITE
  s_close_after_finish_day_sent = false; // don't let a later send close the app
  // Retry the same message (short backoff) before the error overlay - transient
  // failures only, up to MAX_SEND_RETRIES. A truly unreachable phone still ends
  // at the same error, just seconds later.
  if (s_retry_msg_type != 0 && is_retryable_failure(reason) && s_retry_count < MAX_SEND_RETRIES) {
    s_retry_count++;
    uint32_t delay_ms = (uint32_t)RETRY_BACKOFF_BASE_MS << (s_retry_count - 1); // 1s, 2s, 4s
    s_retry_timer = app_timer_register(delay_ms, retry_timer_callback, NULL);
    return;
  }
  clear_pending_retry();
#endif
  // Route the failure through the same fullscreen overlay as a phone->watch
  // sync error so every send failure is visible, not silently swallowed.
  // Reached immediately on aplite (no retry) or once the retries above run out.
  set_status_code(STATUS_ERROR);
  str_copy(s_status_msg, "Couldn't reach phone app", MAX_STATUS_MSG_LEN);
  show_error_overlay();
}
