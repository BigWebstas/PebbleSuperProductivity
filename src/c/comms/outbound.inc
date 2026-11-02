// Watch -> phone: send retry queue and the commands the watch sends.
// Textually #included by main.c (one translation unit) - not compiled alone.

// ---------- outbound send retry ----------
// A watch->phone send can fail transiently - APP_MSG_SEND_TIMEOUT (no ack in
// time) is the most common cause, from a briefly congested BT link or a
// backgrounded phone app, not a real pairing problem. No API extends the
// ack-wait, so this retries the same message with a short backoff before
// surfacing the error overlay. Every send funnels through begin_send() so one
// place knows how to rebuild the last message (AppMessage has no "resend").
//
// aplite-excluded: s_retry_str needs MAX_ID_LEN (96 bytes) to cover every
// message type, well past aplite's ~10-byte margin. aplite keeps the immediate
// error - a real resilience regression there, but honest: the user sees the
// same error and can Resync manually.
#ifndef PBL_PLATFORM_APLITE
#define MAX_SEND_RETRIES 3
#define RETRY_BACKOFF_BASE_MS 1000
static int s_retry_msg_type = 0; // 0 = no message to retry if this send fails
// Sized for the largest payload: a task id (MAX_ID_LEN), habit id, or dictated
// title. MSG_REQUEST_SYNC/MSG_FINISH_DAY leave it empty.
static char s_retry_str[MAX_ID_LEN];
// Second string, only for MSG_NOTE_APPEND (TASK_ID in s_retry_str + the note
// text here). Sized to MAX_TITLE_LEN - the dictation session's own buffer size.
static char s_retry_str2[MAX_TITLE_LEN];
static int32_t s_retry_int = 0; // TASK_DONE / TRACKED_MS / HABIT_DELTA, whichever s_retry_msg_type needs
static int s_retry_count = 0;
static AppTimer *s_retry_timer = NULL;

static void clear_pending_retry(void) {
  s_retry_msg_type = 0;
  s_retry_count = 0;
  if (s_retry_timer) {
    app_timer_cancel(s_retry_timer);
    s_retry_timer = NULL;
  }
}

// Rebuilds and (re)sends the message s_retry_msg_type/str/int describe - the
// single source of truth for every send's wire format, used for both the first
// attempt (via begin_send) and any retry.
static void send_pending_retry(void) {
  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) != APP_MSG_OK) {
    return;
  }
  dict_write_int32(iter, KEY_MSG_TYPE, s_retry_msg_type);
  switch (s_retry_msg_type) {
    case MSG_TASK_TOGGLE:
      dict_write_cstring(iter, KEY_TASK_ID, s_retry_str);
      dict_write_int32(iter, KEY_TASK_DONE, s_retry_int);
      break;
    case MSG_TRACK_TIME_STOP:
    case MSG_TRACK_TIME_START:
      dict_write_cstring(iter, KEY_TASK_ID, s_retry_str);
      dict_write_int32(iter, KEY_TRACKED_MS, s_retry_int);
      break;
    case MSG_HABIT_ADJUST:
      dict_write_cstring(iter, KEY_HABIT_ID, s_retry_str);
      dict_write_int32(iter, KEY_HABIT_DELTA, s_retry_int);
      break;
    case MSG_TASK_ADD:
    case MSG_SEARCH_REQUEST:
      dict_write_cstring(iter, KEY_TASK_TITLE, s_retry_str);
      break;
    case MSG_NOTESPAGE_APPEND:
      dict_write_cstring(iter, KEY_NOTE_TEXT, s_retry_str);
      break;
    case MSG_CAL_MONTH_REQUEST:
      dict_write_int32(iter, KEY_CAL_MONTH_OFFSET, s_retry_int);
      break;
    case MSG_CALDAY_REQUEST:
      dict_write_cstring(iter, KEY_CALDAY_DATE, s_retry_str);
      break;
    case MSG_TASK_DELETE:
      dict_write_cstring(iter, KEY_TASK_ID, s_retry_str);
      break;
    case MSG_TASK_SET_PRIORITY:
      dict_write_cstring(iter, KEY_TASK_ID, s_retry_str);
      dict_write_int32(iter, KEY_TASK_PRIORITY, s_retry_int);
      break;
    case MSG_HABIT_TRACK_STOP:
      dict_write_cstring(iter, KEY_HABIT_ID, s_retry_str);
      dict_write_int32(iter, KEY_TRACKED_MS, s_retry_int);
      break;
    case MSG_NOTE_APPEND:
      dict_write_cstring(iter, KEY_TASK_ID, s_retry_str);
      dict_write_cstring(iter, KEY_NOTE_TEXT, s_retry_str2);
      break;
    case MSG_NOTE_REQUEST:
      dict_write_cstring(iter, KEY_TASK_ID, s_retry_str);
      break;
    case MSG_TASK_PLAN_TOMORROW:
    case MSG_TASK_PLAN_TODAY:
    case MSG_TASK_UNSCHEDULE:
      dict_write_cstring(iter, KEY_TASK_ID, s_retry_str);
      if (s_retry_str2[0] != '\0') {
        dict_write_cstring(iter, KEY_PROJECT_ID, s_retry_str2);
      }
      break;
    case MSG_TASK_SET_BACKLOG:
      dict_write_cstring(iter, KEY_TASK_ID, s_retry_str);
      if (s_retry_str2[0] != '\0') {
        dict_write_cstring(iter, KEY_PROJECT_ID, s_retry_str2);
      }
      dict_write_int32(iter, KEY_PROJECT_TASK_BACKLOG, s_retry_int);
      break;
    case MSG_TASK_SET_ESTIMATE:
      dict_write_cstring(iter, KEY_TASK_ID, s_retry_str);
      dict_write_int32(iter, KEY_TASK_TIME_ESTIMATE_MS, s_retry_int);
      break;
    case MSG_TASK_SET_DEADLINE:
      dict_write_cstring(iter, KEY_TASK_ID, s_retry_str);
      dict_write_int32(iter, KEY_TASK_DEADLINE_DAYS, s_retry_int);
      break;
    case MSG_TASK_SET_DUE_TIME:
      dict_write_cstring(iter, KEY_TASK_ID, s_retry_str);
      dict_write_int32(iter, KEY_TASK_DUE_MIN, s_retry_int);
      break;
    case MSG_TASK_MOVE_PROJECT:
      dict_write_cstring(iter, KEY_TASK_ID, s_retry_str);
      dict_write_cstring(iter, KEY_PROJECT_ID, s_retry_str2); // target project
      break;
    case MSG_TASK_TOGGLE_CHECK:
      dict_write_cstring(iter, KEY_TASK_ID, s_retry_str);
      dict_write_int32(iter, KEY_CHECK_INDEX, s_retry_int >> 1); // index<<1 | checked
      dict_write_int32(iter, KEY_CHECK_VALUE, s_retry_int & 1);
      break;
    case MSG_METRIC_ENERGY:
      dict_write_int32(iter, KEY_METRIC_ENERGY, s_retry_int);
      break;
    case MSG_TASK_REPEAT_REQUEST:
      dict_write_cstring(iter, KEY_TASK_ID, s_retry_str);
      break;
    case MSG_TASK_REPEAT_PAUSE:
      dict_write_cstring(iter, KEY_TASK_ID, s_retry_str);
      dict_write_int32(iter, KEY_TASK_REPEAT_PAUSED, s_retry_int);
      break;
    case MSG_METRIC_RATING:
      dict_write_int32(iter, KEY_METRIC_RATING, s_retry_int);
      break;
    case MSG_METRIC_REFLECT:
      dict_write_cstring(iter, KEY_METRIC_REFLECT_TEXT, s_retry_str);
      break;
    case MSG_TASK_TOGGLE_TAG:
      dict_write_cstring(iter, KEY_TASK_ID, s_retry_str);
      dict_write_cstring(iter, KEY_PROJECT_ID, s_retry_str2); // the tag id
      dict_write_int32(iter, KEY_PROJECT_TASK_BACKLOG, s_retry_int); // 1 = assign
      break;
    case MSG_PROJECT_NOTE_APPEND:
      dict_write_cstring(iter, KEY_PROJECT_ID, s_retry_str);
      dict_write_cstring(iter, KEY_NOTE_TEXT, s_retry_str2);
      break;
    case MSG_PROJECT_NOTE_REQUEST:
      dict_write_cstring(iter, KEY_PROJECT_ID, s_retry_str);
      break;
#if PROJECTS_BROWSER
    case MSG_PROJECT_TASKS_REQUEST:
      dict_write_cstring(iter, KEY_PROJECT_ID, s_retry_str);
      dict_write_int32(iter, KEY_IS_TAGS, s_retry_int);
      break;
    case MSG_PROJECT_LIST_REQUEST:
      dict_write_int32(iter, KEY_IS_TAGS, s_retry_int);
      if (s_retry_str[0] != '\0') {
        dict_write_cstring(iter, KEY_TASK_ID, s_retry_str); // BROWSE_TAG_EDIT
      }
      break;
#endif
    case MSG_FINISH_DAY:
    case MSG_REQUEST_SYNC:
    case MSG_PRESENCE_STOP:
    default:
      break; // no extra keys
  }
  app_message_outbox_send();
}

// Every watch-initiated send starts here: stash what it takes to rebuild this
// message for a retry, reset the retry count, and cancel any older pending
// retry timer so a stale retry can't resend outdated data. str_val2 is non-NULL
// only for MSG_NOTE_APPEND.
static void begin_send(int msg_type, const char *str_val, const char *str_val2, int32_t int_val) {
  if (s_retry_timer) {
    app_timer_cancel(s_retry_timer);
    s_retry_timer = NULL;
  }
  s_retry_msg_type = msg_type;
  if (str_val) {
    str_copy(s_retry_str, str_val, sizeof(s_retry_str));
  } else {
    s_retry_str[0] = '\0';
  }
  if (str_val2) {
    str_copy(s_retry_str2, str_val2, sizeof(s_retry_str2));
  } else {
    s_retry_str2[0] = '\0';
  }
  s_retry_int = int_val;
  s_retry_count = 0;
  send_pending_retry();
}
#endif

static void send_task_toggle(Task *task) {
#ifdef PBL_PLATFORM_APLITE
  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) != APP_MSG_OK) {
    return;
  }
  dict_write_int32(iter, KEY_MSG_TYPE, MSG_TASK_TOGGLE);
  dict_write_cstring(iter, KEY_TASK_ID, task->id);
  dict_write_int32(iter, KEY_TASK_DONE, task->done ? 1 : 0);
  app_message_outbox_send();
#else
  begin_send(MSG_TASK_TOGGLE, task->id, NULL, task->done ? 1 : 0);
#endif
}

#ifndef PBL_PLATFORM_APLITE
// The action menu's "Delete" row - only reachable there, which is itself
// non-aplite (no RAM budget for the menu window), so no aplite branch needed.
static void send_task_delete(const char *task_id) {
  begin_send(MSG_TASK_DELETE, task_id, NULL, 0);
}
#endif

static void send_track_time_stop(const char *task_id, int32_t tracked_ms) {
#ifdef PBL_PLATFORM_APLITE
  DictionaryIterator *iter;
  if (app_message_outbox_begin(&iter) != APP_MSG_OK) {
    return;
  }
  dict_write_int32(iter, KEY_MSG_TYPE, MSG_TRACK_TIME_STOP);
  dict_write_cstring(iter, KEY_TASK_ID, task_id);
  dict_write_int32(iter, KEY_TRACKED_MS, tracked_ms);
  app_message_outbox_send();
#else
  begin_send(MSG_TRACK_TIME_STOP, task_id, NULL, tracked_ms);
#endif
}

#ifndef PBL_PLATFORM_APLITE
// Phase 2 live-tracking presence: tells the phone this watch just started (or
// resumed, elapsed_ms > 0) tracking a task, so it can broadcast "Tracking on
// Pebble" to the account's other devices. aplite-excluded with the rest of
// the presence feature.
static void send_track_time_start(const char *task_id, int32_t elapsed_ms) {
  begin_send(MSG_TRACK_TIME_START, task_id, NULL, elapsed_ms);
}
#endif

#ifndef PBL_PLATFORM_APLITE
// Schedule the task for today / tomorrow, or clear its scheduling. The phone
// turns this into an updateTask op and pushes a fresh list back - the task may
// leave or join a Today-only view. aplite-excluded with the gesture.
static void send_task_reschedule(const char *task_id, RescheduleKind kind, const char *project_id) {
  const char *pid = (project_id && project_id[0] != '\0') ? project_id : NULL;
  if (kind == RESCHEDULE_TO_BACKLOG || kind == RESCHEDULE_FROM_BACKLOG) {
    // int_val carries the direction (1 = into the backlog); see send_pending_retry.
    begin_send(MSG_TASK_SET_BACKLOG, task_id, pid, kind == RESCHEDULE_TO_BACKLOG ? 1 : 0);
    return;
  }
  int msg_type = kind == RESCHEDULE_TODAY ? MSG_TASK_PLAN_TODAY
                 : kind == RESCHEDULE_TOMORROW ? MSG_TASK_PLAN_TOMORROW
                 : MSG_TASK_UNSCHEDULE;
  begin_send(msg_type, task_id, pid, 0);
}

// Set (or clear, ms == 0) a task's time estimate. The phone turns it into a
// plain updateTask op and pushes a fresh list back. int_val carries the ms.
static void send_task_set_estimate(const char *task_id, int32_t ms) {
  begin_send(MSG_TASK_SET_ESTIMATE, task_id, NULL, ms);
}

// Set a task's deadline to `days` from today, or clear it (days < 0).
static void send_task_set_deadline(const char *task_id, int32_t days) {
  begin_send(MSG_TASK_SET_DEADLINE, task_id, NULL, days);
}

// Add (assign != 0) or remove a tag from a task.
static void send_toggle_tag(const char *task_id, const char *tag_id, int32_t assign) {
  begin_send(MSG_TASK_TOGGLE_TAG, task_id, tag_id, assign);
}

// Schedule a task at `hour`:00 (the phone picks the day).
static void send_task_set_due_time(const char *task_id, int hour) {
  begin_send(MSG_TASK_SET_DUE_TIME, task_id, NULL, hour * 60);
}

static void send_move_to_project(const char *task_id, const char *project_id) {
  begin_send(MSG_TASK_MOVE_PROJECT, task_id, project_id, 0);
}
#endif

#ifndef PBL_PLATFORM_APLITE
// Set right before send_finish_day() queues its message; outbox_sent_handler
// checks it to close the app only when the Finish Day send is what just
// CONFIRMED (not just queued - an eager close would hide a later send failure).
// outbox_failed_handler clears it too.
static bool s_close_after_finish_day_sent = false;

// No extra keys - the watch's Task struct is a trimmed display projection, so it
// can't build a full archive payload; the phone's state.task cache has
// everything (handleFinishDay in index.js). aplite-excluded with the row.
static void send_finish_day(void) {
  begin_send(MSG_FINISH_DAY, NULL, NULL, 0);
}
#endif

#ifndef PBL_PLATFORM_APLITE
static void send_task_add(const char *title) {
  begin_send(MSG_TASK_ADD, title, NULL, 0);
}

static void send_note_append(const char *id, const char *note_text, bool is_project) {
  begin_send(is_project ? MSG_PROJECT_NOTE_APPEND : MSG_NOTE_APPEND, id, note_text, 0);
}

// Fires when dictation finishes (success, cancel, or failure). Mic platforms
// only. Shared by Add Task and note-append (s_dictation_is_note_append routes).
static void dictation_status_callback(DictationSession *session, DictationSessionStatus status,
                                       char *transcription, void *context) {
  s_dictation_pending = false;
  if (status == DictationSessionStatusSuccess) {
    if (s_dictation_target == DICT_NOTE_APPEND) {
      send_note_append(s_notes_overlay_subject_id, transcription, s_notes_overlay_is_project);
    } else if (s_dictation_target == DICT_SEARCH) {
      str_copy(s_search_query, transcription, sizeof(s_search_query));
      push_page_window(PAGE_SEARCH); // its window_load fires MSG_SEARCH_REQUEST
    } else if (s_dictation_target == DICT_REFLECT) {
      begin_send(MSG_METRIC_REFLECT, transcription, NULL, 0);
      s_reflect_note_set = true;
      if (s_reflect_menu) {
        menu_layer_reload_data(s_reflect_menu);
      }
    } else if (s_dictation_target == DICT_NOTESPAGE_APPEND) {
      begin_send(MSG_NOTESPAGE_APPEND, transcription, NULL, 0);
    } else {
      send_task_add(transcription);
#ifdef PBL_PLATFORM_EMERY
      s_addtask_flash_active = true;
      s_addtask_flash_tick = 0;
      if (s_menu_layer) {
        menu_layer_reload_data(s_menu_layer);
        refresh_scroll_state(false);
      }
#endif
    }
    return;
  }
  if (status == DictationSessionStatusFailureTranscriptionRejected) {
    // User declined the transcription on the confirmation screen - a cancel,
    // not a failure, so a silent no-op.
    return;
  }
  // Every other failure already gets a dialog from the OS's own dictation UI,
  // so no error overlay here. Logged only.
  APP_LOG(APP_LOG_LEVEL_INFO, "dictation failed, status=%d", (int)status);
}

static void start_add_task_dictation(void) {
  if (s_dictation_pending || !s_dictation_session) {
    // Ignore a rapid double-press, and a call before window_load created the
    // session (shouldn't happen).
    return;
  }
  s_dictation_target = DICT_ADD_TASK;
  s_dictation_pending = true;
  dictation_session_start(s_dictation_session);
}

// Long-select on the notes overlay - dictates text to append to the shown
// task's or project's notes. Same session/guard as start_add_task_dictation,
// just tagged for the callback to route differently.
static void start_note_append_dictation(void) {
  if (s_dictation_pending || !s_dictation_session) {
    return;
  }
  s_dictation_target = DICT_NOTE_APPEND;
  s_dictation_pending = true;
  dictation_session_start(s_dictation_session);
}

// Long-Select on the Notes page - dictates text appended to (or creating) the
// pinned-to-today note the page shows. Same guard/session as the above.
static void start_notespage_append_dictation(void) {
  if (s_dictation_pending || !s_dictation_session) {
    return;
  }
  s_dictation_target = DICT_NOTESPAGE_APPEND;
  s_dictation_pending = true;
  dictation_session_start(s_dictation_session);
}

// Section-0 "Search" row - dictate a query; the callback stashes it in
// s_search_query and opens the shared page window in PAGE_SEARCH, whose load
// fires MSG_SEARCH_REQUEST.
static void start_search_dictation(void) {
  if (s_dictation_pending || !s_dictation_session) {
    return;
  }
  s_dictation_target = DICT_SEARCH;
  s_dictation_pending = true;
  dictation_session_start(s_dictation_session);
}

// Reflect window "improvement" row - dictate one thing to improve; the callback
// routes it to MSG_METRIC_REFLECT (today's metric.reflections[0]).
static void start_reflect_dictation(void) {
  if (s_dictation_pending || !s_dictation_session) {
    return;
  }
  s_dictation_target = DICT_REFLECT;
  s_dictation_pending = true;
  dictation_session_start(s_dictation_session);
}
#endif
