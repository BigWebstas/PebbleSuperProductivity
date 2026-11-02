// Per-task action menu (track, notes, tags, estimate, priority, delete...).
// Textually #included by main.c (one translation unit) - not compiled alone.

// ---------- per-task action menu ----------
// Long-Select on a task row - today list OR a Projects/Tags browser task view -
// opens this. The one home for per-task actions. Each row pops this menu, then
// launches its target, so Back from the target returns to the list.
enum {
  ACT_DONE, ACT_TRACK, ACT_TODAY, ACT_TOMORROW, ACT_AT, ACT_UNSCHEDULE,
  ACT_NOTES, ACT_TAGS, ACT_MOVE, ACT_ESTIMATE, ACT_DEADLINE, ACT_BACKLOG,
  ACT_PRIORITY,
  ACT_DELETE, // always last - destructive, see the arm/confirm state below
  ACT_REPEAT, // appended by act_rows() only for a recurring task
  ACT_COLLAPSE, // inserted before ACT_DELETE by act_rows() for a task with subtasks
};
// The visible rows, in order, per context. ACT_DONE is first (the commonest
// action, so Select-Select completes a task). Tags / Move open the browse window
// as a picker, so they're only offered from the today list (from the browser
// that window is already on the stack). Backlog is a project-task concept.
// ACT_DELETE is last in every context - no round-trip on the watch (see
// CLAUDE.md's "no delete-from-watch" note, no longer true as of this row),
// so it needs its own arm/confirm step, unlike everything else here.
static const int s_act_rows_today[] = {
  ACT_DONE, ACT_TRACK, ACT_TODAY, ACT_TOMORROW, ACT_AT, ACT_UNSCHEDULE,
  ACT_NOTES, ACT_TAGS, ACT_MOVE, ACT_ESTIMATE, ACT_DEADLINE, ACT_PRIORITY, ACT_DELETE };
static const int s_act_rows_project[] = {
  ACT_DONE, ACT_TRACK, ACT_TODAY, ACT_TOMORROW, ACT_AT, ACT_UNSCHEDULE,
  ACT_NOTES, ACT_ESTIMATE, ACT_DEADLINE, ACT_PRIORITY, ACT_BACKLOG, ACT_DELETE };
static const int s_act_rows_tag[] = {
  ACT_DONE, ACT_TRACK, ACT_TODAY, ACT_TOMORROW, ACT_AT, ACT_UNSCHEDULE,
  ACT_NOTES, ACT_ESTIMATE, ACT_DEADLINE, ACT_PRIORITY, ACT_DELETE };

// ActionCtx declared with the forward decls.
static Window *s_action_window = NULL;
static MenuLayer *s_action_menu = NULL;
static StatusBarLayer *s_action_status_bar = NULL;
static char s_action_task_id[MAX_ID_LEN] = "";
static ActionCtx s_action_ctx = ACTX_TODAY;
static bool s_action_in_backlog = false;
// "Repeat" row: pattern text (MSG_TASK_REPEAT_DATA) + pause state, per open.
static char s_action_repeat_text[24] = "";
static bool s_action_repeat_paused = false;
// ACT_DELETE's own two-press guard: the first press arms it (label flips to
// "Confirm delete?", menu stays open, like ACT_REPEAT's in-place toggle) and
// starts DELETE_ARM_MS; a second press within that window actually sends
// MSG_TASK_DELETE, anything else (Back, the timer, opening a different task)
// disarms it back to plain "Delete". Reset whenever the menu opens fresh
// (push_action_menu) and on close (action_window_unload).
#define DELETE_ARM_MS 4000
static bool s_delete_armed = false;
static AppTimer *s_delete_arm_timer = NULL;
static int s_act_rows_buf[15];

static Task *resolve_action_task(void);

#ifdef PBL_PLATFORM_EMERY
// How many subtasks directly follow `t` in the today list (s_tasks keeps a
// parent's subtasks right after it); 0 for a task outside that list.
static int task_subtask_count(const Task *t) {
  if (!t || t < s_tasks || t >= s_tasks + s_task_count) {
    return 0;
  }
  int n = 0;
  while (t + n + 1 < s_tasks + s_task_count && t[n + 1].is_subtask) {
    n++;
  }
  return n;
}
#endif

static const int *act_rows(int *count) {
  const int *base;
  int n;
  switch (s_action_ctx) {
    case ACTX_PROJECT: base = s_act_rows_project; n = ARRLEN(s_act_rows_project); break;
    case ACTX_TAG:     base = s_act_rows_tag;     n = ARRLEN(s_act_rows_tag);     break;
    default:           base = s_act_rows_today;   n = ARRLEN(s_act_rows_today);   break;
  }
  memcpy(s_act_rows_buf, base, (size_t)n * sizeof(int));
  Task *t = resolve_action_task();
#ifdef PBL_PLATFORM_EMERY
  if (s_action_ctx == ACTX_TODAY && task_subtask_count(t) > 0) {
    s_act_rows_buf[n] = s_act_rows_buf[n - 1]; // ACT_DELETE stays last
    s_act_rows_buf[n - 1] = ACT_COLLAPSE;
    n++;
  }
#endif
  if (t && t->recurs && n < (int)ARRLEN(s_act_rows_buf)) {
    s_act_rows_buf[n++] = ACT_REPEAT;
  }
  *count = n;
  return s_act_rows_buf;
}

static bool action_task_is_tracked(void) {
  return s_tracking_task_id[0] != '\0' &&
         strncmp(s_tracking_task_id, s_action_task_id, MAX_ID_LEN) == 0;
}

// MSG_TASK_REPEAT_DATA: the "Repeat" row's pattern + pause state, for the task
// whose action menu is open. A stale reply for a different task is ignored.
static void handle_repeat_data(DictionaryIterator *it) {
  if (strncmp(tuple_str(it, KEY_TASK_ID, ""), s_action_task_id, MAX_ID_LEN) != 0) {
    return;
  }
  str_copy(s_action_repeat_text, tuple_str(it, KEY_TASK_REPEAT_TEXT, ""), sizeof(s_action_repeat_text));
  s_action_repeat_paused = tuple_int(it, KEY_TASK_REPEAT_PAUSED, 0) != 0;
  if (s_action_menu) {
    menu_layer_reload_data(s_action_menu);
  }
}

// The Task* for s_action_task_id: the today list, else a browsed task.
static Task *resolve_action_task(void) {
  Task *t = find_task_by_id(s_action_task_id);
#if PROJECTS_BROWSER
  if (!t && s_browse_tasks) {
    for (int i = 0; i < s_browse_task_count; i++) {
      if (strncmp(s_browse_tasks[i].id, s_action_task_id, MAX_ID_LEN) == 0) {
        return &s_browse_tasks[i];
      }
    }
  }
#endif
  return t;
}

static uint16_t action_get_num_rows(MenuLayer *ml, uint16_t section, void *ctx) {
  int n;
  act_rows(&n);
  return (uint16_t)n;
}

static void action_draw_row(GContext *ctx, const Layer *cell, MenuIndex *idx, void *c) {
  int n;
  const int *rows = act_rows(&n);
  if ((int)idx->row >= n) {
    return;
  }
  const char *label = "";
  const char *sub = NULL;
  switch (rows[idx->row]) {
    case ACT_DONE: {
      Task *dt = resolve_action_task();
      label = (dt && dt->done) ? "Mark not done" : "Mark done";
      break;
    }
    case ACT_TRACK:      label = action_task_is_tracked() ? "Stop tracking" : "Start tracking"; break;
    case ACT_TODAY:      label = "Schedule today"; break;
    case ACT_TOMORROW:   label = "Schedule tomorrow"; break;
    case ACT_AT:         label = "Schedule Time"; break;
    case ACT_UNSCHEDULE: label = "Unschedule"; break;
    case ACT_NOTES:      label = "Notes"; break;
    case ACT_TAGS:       label = "Edit tags"; break;
    case ACT_MOVE:       label = "Move to project"; break;
    case ACT_ESTIMATE:   label = "Set estimate"; break;
    case ACT_DEADLINE:   label = "Set deadline"; break;
    case ACT_PRIORITY: {
      static const char *const names[] = { "None", "Low", "Medium", "High" };
      Task *pt = resolve_action_task();
      label = "Priority";
      sub = names[pt ? pt->priority & 3 : 0];
      break;
    }
    case ACT_BACKLOG:    label = s_action_in_backlog ? "Move to list" : "Move to backlog"; break;
#ifdef PBL_PLATFORM_EMERY
    case ACT_COLLAPSE: {
      Task *ct = resolve_action_task();
      label = (ct && ct->collapsed) ? "Expand" : "Collapse";
      break;
    }
#endif
    case ACT_DELETE:     label = s_delete_armed ? "Confirm delete?" : "Delete"; break;
    case ACT_REPEAT:
      label = s_action_repeat_paused ? "Resume repeat" : "Pause repeat";
      sub = s_action_repeat_text[0] ? s_action_repeat_text : "Loading...";
      break;
  }
  menu_cell_basic_draw(ctx, cell, label, sub, NULL);
}

// DELETE_ARM_MS elapsed with no second press - disarm back to plain "Delete".
static void delete_disarm_callback(void *data) {
  s_delete_arm_timer = NULL;
  s_delete_armed = false;
  if (s_action_menu) {
    menu_layer_reload_data(s_action_menu);
  }
}

static void action_select(MenuLayer *ml, MenuIndex *idx, void *c) {
  backlight_touch();
  int n;
  const int *rows = act_rows(&n);
  if ((int)idx->row >= n) {
    return;
  }
  int row = rows[idx->row];
  Task *t = resolve_action_task();
  if (row == ACT_REPEAT) {
    // Toggle pause in place - keep the menu open so the label flips.
    s_action_repeat_paused = !s_action_repeat_paused;
    begin_send(MSG_TASK_REPEAT_PAUSE, s_action_task_id, NULL, s_action_repeat_paused ? 1 : 0);
    menu_layer_reload_data(s_action_menu);
    return;
  }
  if (row == ACT_PRIORITY) {
    // Cycle in place (none -> high -> medium -> low), like the repeat toggle.
    if (t) {
      t->priority = t->priority ? t->priority - 1 : PRIO_HIGH; // none>high>medium>low>none
      save_tasks();
      begin_send(MSG_TASK_SET_PRIORITY, s_action_task_id, NULL, t->priority);
      menu_layer_reload_data(s_action_menu);
      menu_layer_reload_data(s_menu_layer);
    }
    return;
  }
  if (row == ACT_DELETE && !s_delete_armed) {
    // First press - arm it and stay open, same shape as ACT_REPEAT above.
    s_delete_armed = true;
    if (s_delete_arm_timer) {
      app_timer_cancel(s_delete_arm_timer);
    }
    s_delete_arm_timer = app_timer_register(DELETE_ARM_MS, delete_disarm_callback, NULL);
    vibes_short_pulse();
    menu_layer_reload_data(s_action_menu);
    return;
  }
  if (s_delete_arm_timer) {
    app_timer_cancel(s_delete_arm_timer);
    s_delete_arm_timer = NULL;
  }
  s_delete_armed = false;
  window_stack_pop(true); // close the menu; targets push onto the list below it
  switch (row) {
    case ACT_DONE:
      if (t) {
#ifdef PBL_PLATFORM_EMERY
        // Marking done rides the today list's 5s "Marking done..." cancel window
        // (a Select undoes it) when the task is really on that list.
        if (!t->done && s_action_ctx == ACTX_TODAY && find_task_by_id(t->id)) {
          begin_pending_done(t->id);
          break;
        }
#endif
        t->done = !t->done;
        save_tasks();
        send_task_toggle(t);
        menu_layer_reload_data(s_menu_layer);
#if PROJECTS_BROWSER
        if (s_browse_menu) {
          menu_layer_reload_data(s_browse_menu);
        }
#endif
      }
      break;
#ifdef PBL_PLATFORM_EMERY
    case ACT_COLLAPSE:
      if (t) {
        bool collapse = !t->collapsed;
        t->collapsed = collapse;
        for (int i = task_subtask_count(t); i > 0; i--) {
          t[i].collapsed = collapse;
        }
        menu_layer_reload_data(s_menu_layer);
      }
      break;
#endif
    case ACT_TRACK:
      if (!t) {
        break;
      }
      {
        bool was = action_task_is_tracked();
        stop_tracking_and_report();
        if (!was) {
          start_tracking(t);
        }
        menu_layer_reload_data(s_menu_layer);
#ifndef PBL_PLATFORM_APLITE
        if (!was) {
          push_live_window();
        }
#endif
      }
      break;
    case ACT_TODAY:
      begin_pending_reschedule(RESCHEDULE_TODAY);
      break;
    case ACT_TOMORROW:
      begin_pending_reschedule(RESCHEDULE_TOMORROW);
      break;
    case ACT_AT: {
      int hour = 9;
      if (t && t->due_min >= 0) {
        hour = t->due_min / 60;
      } else {
        time_t now = time(NULL);
        struct tm *lt = localtime(&now);
        hour = lt->tm_hour;
      }
      push_value_picker(PICK_TIME, s_action_task_id, hour);
      break;
    }
    case ACT_UNSCHEDULE:
      begin_pending_reschedule(RESCHEDULE_UNSCHEDULE);
      break;
    case ACT_BACKLOG:
      begin_pending_reschedule(s_action_in_backlog ? RESCHEDULE_FROM_BACKLOG : RESCHEDULE_TO_BACKLOG);
      break;
    case ACT_NOTES:
      if (t) {
        show_notes_overlay(t);
      }
      break;
    case ACT_TAGS:
#if PROJECTS_BROWSER
      s_browse_mode = BROWSE_TAG_EDIT;
      str_copy(s_browse_edit_task_id, s_action_task_id, sizeof(s_browse_edit_task_id));
      push_browse_window(NULL);
#endif
      break;
    case ACT_MOVE:
#if PROJECTS_BROWSER
      s_browse_mode = BROWSE_MOVE;
      str_copy(s_browse_edit_task_id, s_action_task_id, sizeof(s_browse_edit_task_id));
      push_browse_window(NULL);
#endif
      break;
    case ACT_ESTIMATE:
      push_value_picker(PICK_ESTIMATE, s_action_task_id, t ? t->time_estimate_ms : 0);
      break;
    case ACT_DEADLINE:
      push_value_picker(PICK_DEADLINE, s_action_task_id, t ? t->deadline_days : DEADLINE_NONE);
      break;
    case ACT_DELETE:
      // Confirmed already (the arm/re-press dance above) - fire the send and
      // re-fetch whichever list is now showing underneath so the deleted row
      // doesn't linger until the next natural sync.
      if (t) {
        send_task_delete(s_action_task_id);
#if PROJECTS_BROWSER
        if (s_action_ctx != ACTX_TODAY && s_browse_menu) {
          browse_descend(s_browse_project_id);
        } else
#endif
        {
          request_sync();
        }
      }
      break;
  }
}

static void action_window_load(Window *window) {
  Layer *wl;
  GRect content = window_chrome(window, &s_action_status_bar, &wl);
  s_action_menu = menu_layer_create(content);
  menu_layer_set_callbacks(s_action_menu, NULL, (MenuLayerCallbacks) {
    .get_num_rows = action_get_num_rows,
    .draw_row = action_draw_row,
    .select_click = action_select,
  });
  menu_layer_set_click_config_onto_window(s_action_menu, window);
  layer_add_child(wl, menu_layer_get_layer(s_action_menu));
}

static void action_window_unload(Window *window) {
  menu_layer_destroy(s_action_menu);
  s_action_menu = NULL;
  status_bar_layer_destroy(s_action_status_bar);
  s_action_status_bar = NULL;
  if (s_delete_arm_timer) {
    app_timer_cancel(s_delete_arm_timer);
    s_delete_arm_timer = NULL;
  }
  s_delete_armed = false;
}

static void push_action_menu(const char *task_id, ActionCtx ctx, bool in_backlog) {
  if (!task_id || task_id[0] == '\0') {
    return;
  }
  str_copy(s_action_task_id, task_id, sizeof(s_action_task_id));
  s_action_ctx = ctx;
  s_action_in_backlog = in_backlog;
  // "Repeat" row only exists for a recurring task; its pattern text is fetched.
  s_action_repeat_text[0] = '\0';
  s_action_repeat_paused = false;
  if (s_delete_arm_timer) {
    app_timer_cancel(s_delete_arm_timer);
    s_delete_arm_timer = NULL;
  }
  s_delete_armed = false;
  {
    Task *rt = resolve_action_task();
    if (rt && rt->recurs) {
      begin_send(MSG_TASK_REPEAT_REQUEST, s_action_task_id, NULL, 0);
    }
  }
  if (!s_action_window) {
    s_action_window = window_create();
    window_set_window_handlers(s_action_window, (WindowHandlers) {
      .load = action_window_load,
      .unload = action_window_unload,
    });
  }
  window_stack_push(s_action_window, true);
}
