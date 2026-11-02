// Persistent storage: cached tasks/habits/projects, tracking and focus state.
// Textually #included by main.c (one translation unit) - not compiled alone.

// ---------- persistence (so the list survives a watchapp relaunch) ----------

// Task caching is aplite-excluded: its ~24 KB RAM has no margin (the app runs
// within a few hundred bytes of the heap) and its list never persisted anyway
// (see below), so nothing is lost. Everywhere else the cached list is what an
// offline open shows instead of the sync-error screen.
// persist_write_data / persist_read_data silently cap each key at
// PERSIST_DATA_MAX_LENGTH (256 B) - PebbleOS applib/persist.c does
// MIN(buffer_size, PERSIST_DATA_MAX_LENGTH). The old caches wrote a whole
// record array (many KB) to one key, so it was truncated to 256 B on write
// and the exact-length check failed on read - the list never cached at all.
// These helpers split the array across consecutive keys instead.
#ifndef PBL_PLATFORM_APLITE
// Copies `total` bytes between `buf` and keys `base_key`, `base_key+1`, ... in
// 256 B slices. Returns true only if every slice round-tripped its full length.
static bool persist_blob_rw(bool writing, uint32_t base_key, uint8_t *buf, size_t total) {
  size_t off = 0;
  uint32_t key = base_key;
  while (off < total) {
    size_t len = total - off;
    if (len > PERSIST_DATA_MAX_LENGTH) {
      len = PERSIST_DATA_MAX_LENGTH;
    }
    int n = writing ? persist_write_data(key, buf + off, len)
                    : persist_read_data(key, buf + off, len);
    if (n != (int)len) {
      return false;
    }
    off += len;
    key++;
  }
  return true;
}

// A cache of up to `max_count` fixed-size records (`stride` bytes each):
// `count_key` holds the live count, `count_key+1` a stride guard (so a struct
// layout change discards a stale cache), and `chunk_base..` the raw bytes.
// `max_bytes` (>= max_count*stride) bounds the delete sweep.
static void save_blob_cache(uint32_t count_key, uint32_t chunk_base, uint8_t *buf,
                            int count, size_t stride, size_t max_bytes) {
  for (size_t i = 0; i <= max_bytes / PERSIST_DATA_MAX_LENGTH; i++) {
    persist_delete(chunk_base + i);
  }
  if (count <= 0) {
    persist_delete(count_key);
    persist_delete(count_key + 1);
    return;
  }
  if (persist_blob_rw(true, chunk_base, buf, (size_t)count * stride)) {
    persist_write_int(count_key + 1, (int32_t)stride);
    persist_write_int(count_key, count);
  } else {
    persist_delete(count_key);
    persist_delete(count_key + 1);
  }
}

// Returns the loaded record count (0 if absent, stale-layout, or incomplete).
static int load_blob_cache(uint32_t count_key, uint32_t chunk_base, uint8_t *buf,
                           int max_count, size_t stride) {
  if (!persist_exists(count_key) || !persist_exists(count_key + 1) ||
      persist_read_int(count_key + 1) != (int32_t)stride) {
    return 0;
  }
  int count = persist_read_int(count_key);
  if (count <= 0 || count > max_count) {
    return 0;
  }
  return persist_blob_rw(false, chunk_base, buf, (size_t)count * stride) ? count : 0;
}
#endif // !PBL_PLATFORM_APLITE

// ---------- persistence (so the list survives a watchapp relaunch) ----------

// Task caching is aplite-excluded: its ~24 KB RAM has no margin (the app runs
// within a few hundred bytes of the heap) and its list never persisted anyway,
// so nothing is lost. Everywhere else the cached list is what an offline open
// shows instead of the sync-error screen.
#ifdef PBL_PLATFORM_APLITE
static void save_tasks(void) {}
static void load_tasks(void) {}
#else
static void save_tasks(void) {
  persist_delete(100); // legacy single-blob key (pre-chunk builds)
  save_blob_cache(101, 200, (uint8_t *)s_tasks, s_task_count, sizeof(Task),
                  (size_t)MAX_TASKS * sizeof(Task));
}

static void load_tasks(void) {
  s_task_count = load_blob_cache(101, 200, (uint8_t *)s_tasks, MAX_TASKS, sizeof(Task));
}
#endif

static const uint32_t PERSIST_KEY_HABITS = 120;

// aplite fits its 2-habit array in one 256 B key, so it keeps the plain
// single-blob form; everywhere else MAX_HABITS * sizeof(Habit) overflows a
// key and has to be chunked (see save_tasks / save_blob_cache).
#ifdef PBL_PLATFORM_APLITE
static void save_habits(void) {
  if (s_habit_count > 0) {
    persist_write_data(PERSIST_KEY_HABITS, s_habits, sizeof(Habit) * (size_t)s_habit_count);
    persist_write_int(PERSIST_KEY_HABITS + 1, s_habit_count);
  } else {
    persist_delete(PERSIST_KEY_HABITS);
    persist_delete(PERSIST_KEY_HABITS + 1);
  }
}

static void load_habits(void) {
  if (persist_exists(PERSIST_KEY_HABITS + 1)) {
    int count = persist_read_int(PERSIST_KEY_HABITS + 1);
    if (count > 0 && count <= MAX_HABITS) {
      int bytes = persist_read_data(PERSIST_KEY_HABITS, s_habits, sizeof(Habit) * (size_t)count);
      if (bytes == (int)(sizeof(Habit) * (size_t)count)) {
        s_habit_count = count;
      }
    }
  }
}
#else
static void save_habits(void) {
  persist_delete(PERSIST_KEY_HABITS); // legacy single-blob key (pre-chunk builds)
  save_blob_cache(121, 260, (uint8_t *)s_habits, s_habit_count, sizeof(Habit),
                  (size_t)MAX_HABITS * sizeof(Habit));
}

static void load_habits(void) {
  s_habit_count = load_blob_cache(121, 260, (uint8_t *)s_habits, MAX_HABITS, sizeof(Habit));
}
#endif

// Danger-zone "Wipe watch cache" (pairing page -> MSG_WIPE_CACHE): drop every
// persisted list blob - tasks, habits, and the project list - so the next
// offline open starts empty instead of from a stale snapshot. In-memory counts
// are zeroed too; the caller pulls a fresh list from the phone right after.
// Credentials, feature toggles, and tracking/focus state are left alone - those
// aren't "the cache".
static void clear_persisted_caches(void) {
#ifdef PBL_PLATFORM_APLITE
  // aplite only ever persists the small single-blob habit list.
  persist_delete(PERSIST_KEY_HABITS);
  persist_delete(PERSIST_KEY_HABITS + 1);
#else
  persist_delete(100); // legacy single-blob task key (pre-chunk builds)
  save_blob_cache(101, 200, NULL, 0, sizeof(Task), (size_t)MAX_TASKS * sizeof(Task));
  persist_delete(PERSIST_KEY_HABITS); // legacy single-blob habit key
  save_blob_cache(121, 260, NULL, 0, sizeof(Habit), (size_t)MAX_HABITS * sizeof(Habit));
#if PROJECTS_CACHE
  persist_delete(PERSIST_KEY_BROWSE_PROJECTS); // legacy single-blob key
  save_blob_cache(PERSIST_KEY_BROWSE_PROJECTS + 1, 280, NULL, 0, sizeof(BrowseProject),
                  (size_t)MAX_BROWSE_PROJECTS * sizeof(BrowseProject));
  s_browse_project_count = 0;
#endif
#endif
  s_task_count = 0;
  s_habit_count = 0;
}

static const uint32_t PERSIST_KEY_TRACKING_ID = 110;
static const uint32_t PERSIST_KEY_TRACKING_START = 111;

#ifndef PBL_PLATFORM_APLITE
static const uint32_t PERSIST_KEY_LAST_SYNC = 122;
// Epoch of the last SYNCING -> OK transition; drives the Resync row's
// "Synced Nm ago" subtitle. 0 = never synced this install. aplite has no RAM
// headroom for it and keeps a plain "Synced".
static time_t s_last_sync_epoch = 0;
#endif

#ifndef PBL_PLATFORM_APLITE
// Focus mode: a watch-local pomodoro wrapped around the current LOCAL
// tracking session (the desktop's own focus mode is not in the SuperSync
// op-log, so it can't be mirrored - this is the watch's own). s_focus_end_epoch
// is the wall-clock time the session ends; 0 = not focusing. Only meaningful
// while s_tracking_task_id is set. s_focus_len_min is the configured length,
// pushed from the pairing page (FOCUS_LEN_MIN). Persisted so the focus screen
// comes straight back if the firmware's inactivity timeout closed the app.
static const uint32_t PERSIST_KEY_FOCUS_END = 114;
static const uint32_t PERSIST_KEY_FOCUS_ON_BREAK = 121;
static const uint32_t PERSIST_KEY_FOCUS_START = 123;
static time_t s_focus_end_epoch = 0;
// Start of the current focus session - drives Flowtime's count-up and is the
// "session is live" marker there (where s_focus_end_epoch stays 0).
static time_t s_focus_start_epoch = 0;
static int s_focus_len_min = 25;
// The focus timer style, from the pairing page (config.focusType, sent over the
// KEY_USE_POMODORO_CFG slot as 0/1/2 to avoid a new message key):
//   COUNTDOWN - one session of s_focus_len_min, buzz at zero, done.
//   POMODORO  - s_pomodoro_work_min work, then a s_pomodoro_break_min break,
//               looping work/break until a long-hold ends it.
//   FLOWTIME  - counts up, no end; long-hold ends it. MSG_POMODORO_CFG pushes
//               the work/break minutes used by POMODORO.
typedef enum { FOCUS_COUNTDOWN, FOCUS_POMODORO, FOCUS_FLOWTIME } FocusType;
static FocusType s_focus_type = FOCUS_COUNTDOWN;
static int s_pomodoro_work_min = 25;
static int s_pomodoro_break_min = 5;
static bool s_focus_on_break = false;
// Anti-inactivity-close lever: pulse the backlight (light_enable_interaction -
// a fading flash, like a button press, not a latch) this often while a focus
// screen is up. There is no real API for this - see the focus-mode comment.
#define FOCUS_LIGHT_POKE_S 300
static time_t s_focus_last_poke_epoch = 0;

// Count of focus sessions that ran ALL the way to 0:00 today (not ones ended
// early). Watch-local - SP has no syncable focus/pomodoro entity, so this
// can't reach the desktop's end-of-day review; it only feeds the Stats page,
// same as "Current session" and "Without a break". s_focus_done_day is a
// local-calendar day id (tm_year*400 + tm_yday) for the rollover check.
static const uint32_t PERSIST_KEY_FOCUS_DONE_COUNT = 115;
static const uint32_t PERSIST_KEY_FOCUS_DONE_DAY = 116;
static int s_focus_completed_today = 0;
static int s_focus_done_day = 0;

static bool focus_active(void) {
  if (s_tracking_task_id[0] == '\0') {
    return false;
  }
  return s_focus_end_epoch != 0 ||
         (s_focus_type == FOCUS_FLOWTIME && s_focus_start_epoch != 0);
}

// A local-calendar day id for "resets at midnight" counters (focus sessions,
// break tally). Shared by focus_roll_day and break_roll_day.
static int local_day_id(void) {
  time_t now = time(NULL);
  struct tm *lt = localtime(&now);
  return lt->tm_year * 400 + lt->tm_yday;
}

// Zeroes the completed count when the local day has rolled over. Called before
// every read and before an increment.
static void focus_roll_day(void) {
  int d = local_day_id();
  if (d != s_focus_done_day) {
    s_focus_done_day = d;
    s_focus_completed_today = 0;
  }
}

static void save_focus(void) {
  if (s_focus_end_epoch != 0 || s_focus_start_epoch != 0) {
    persist_write_int(PERSIST_KEY_FOCUS_END, (int)s_focus_end_epoch);
    persist_write_int(PERSIST_KEY_FOCUS_START, (int)s_focus_start_epoch);
    persist_write_int(PERSIST_KEY_FOCUS_ON_BREAK, s_focus_on_break ? 1 : 0);
  } else {
    persist_delete(PERSIST_KEY_FOCUS_END);
    persist_delete(PERSIST_KEY_FOCUS_START);
    persist_delete(PERSIST_KEY_FOCUS_ON_BREAK);
  }
  persist_write_int(PERSIST_KEY_FOCUS_DONE_COUNT, s_focus_completed_today);
  persist_write_int(PERSIST_KEY_FOCUS_DONE_DAY, s_focus_done_day);
}

static void load_focus(void) {
  if (persist_exists(PERSIST_KEY_FOCUS_END)) {
    s_focus_end_epoch = (time_t)persist_read_int(PERSIST_KEY_FOCUS_END);
    s_focus_start_epoch = (time_t)persist_read_int(PERSIST_KEY_FOCUS_START);
    s_focus_on_break = persist_exists(PERSIST_KEY_FOCUS_ON_BREAK) &&
                       persist_read_int(PERSIST_KEY_FOCUS_ON_BREAK) != 0;
  }
  s_focus_completed_today = persist_read_int(PERSIST_KEY_FOCUS_DONE_COUNT);
  s_focus_done_day = persist_read_int(PERSIST_KEY_FOCUS_DONE_DAY);
  focus_roll_day();
}

// Records one fully-completed focus session for today's Stats count.
static void focus_bump_completed(void) {
  focus_roll_day();
  s_focus_completed_today++;
  save_focus();
}
#endif

// Its own key pair, independent of save_tasks(), so a tracked session survives
// a resync that replaces s_tasks wholesale.
static void save_tracking(void) {
  if (s_tracking_task_id[0] != '\0') {
    persist_write_string(PERSIST_KEY_TRACKING_ID, s_tracking_task_id);
    persist_write_int(PERSIST_KEY_TRACKING_START, (int)s_tracking_start_epoch);
  } else {
    persist_delete(PERSIST_KEY_TRACKING_ID);
    persist_delete(PERSIST_KEY_TRACKING_START);
  }
}

static void load_tracking(void) {
  if (persist_exists(PERSIST_KEY_TRACKING_ID)) {
    persist_read_string(PERSIST_KEY_TRACKING_ID, s_tracking_task_id, sizeof(s_tracking_task_id));
    s_tracking_start_epoch = (time_t)persist_read_int(PERSIST_KEY_TRACKING_START);
  }
}

#ifdef BREAK_REMINDER
static const uint32_t PERSIST_KEY_BREAK_ACCUM_S = 112;
static const uint32_t PERSIST_KEY_BREAK_LAST_STOP = 113;
static const uint32_t PERSIST_KEY_BREAK_COUNT_TODAY = 117;
static const uint32_t PERSIST_KEY_BREAK_TOTAL_S_TODAY = 118;
static const uint32_t PERSIST_KEY_BREAK_DAY = 119;

// Zeroes the Stats break tally when the local day has rolled over.
static void break_roll_day(void) {
  int d = local_day_id();
  if (d != s_break_day) {
    s_break_day = d;
    s_break_count_today = 0;
    s_break_total_s_today = 0;
  }
}

// Persists the break-reminder tally so it survives the app closing between a
// stop and the next start. persist_read_int returns 0 for a missing key, which
// is the right default for all of them.
static void save_break_state(void) {
  persist_write_int(PERSIST_KEY_BREAK_ACCUM_S, s_break_accum_s);
  persist_write_int(PERSIST_KEY_BREAK_LAST_STOP, (int)s_break_last_stop_epoch);
  persist_write_int(PERSIST_KEY_BREAK_COUNT_TODAY, s_break_count_today);
  persist_write_int(PERSIST_KEY_BREAK_TOTAL_S_TODAY, s_break_total_s_today);
  persist_write_int(PERSIST_KEY_BREAK_DAY, s_break_day);
}

static void load_break_state(void) {
  s_break_accum_s = persist_read_int(PERSIST_KEY_BREAK_ACCUM_S);
  s_break_last_stop_epoch = (time_t)persist_read_int(PERSIST_KEY_BREAK_LAST_STOP);
  s_break_count_today = persist_read_int(PERSIST_KEY_BREAK_COUNT_TODAY);
  s_break_total_s_today = persist_read_int(PERSIST_KEY_BREAK_TOTAL_S_TODAY);
  s_break_day = persist_read_int(PERSIST_KEY_BREAK_DAY);
  break_roll_day();
}
#endif

#ifndef PBL_PLATFORM_APLITE
static const uint32_t PERSIST_KEY_HABIT_TRACKING_ID = 130;
static const uint32_t PERSIST_KEY_HABIT_TRACKING_START = 131;
static const uint32_t PERSIST_KEY_HABIT_COUNTDOWN_PAUSED = 132;
static const uint32_t PERSIST_KEY_HABIT_COUNTDOWN_FROZEN_MS = 133;

// Mirrors save_tracking()/load_tracking() for a tracked StopWatch/countdown
// habit. The pause fields only matter for an is_countdown session but persist
// unconditionally (a StopWatch just always saves paused: false).
static void save_habit_tracking(void) {
  if (s_tracking_habit_id[0] != '\0') {
    persist_write_string(PERSIST_KEY_HABIT_TRACKING_ID, s_tracking_habit_id);
    persist_write_int(PERSIST_KEY_HABIT_TRACKING_START, (int)s_tracking_habit_start_epoch);
    persist_write_int(PERSIST_KEY_HABIT_COUNTDOWN_PAUSED, s_habit_countdown_paused ? 1 : 0);
    persist_write_int(PERSIST_KEY_HABIT_COUNTDOWN_FROZEN_MS, s_habit_countdown_frozen_elapsed_ms);
  } else {
    persist_delete(PERSIST_KEY_HABIT_TRACKING_ID);
    persist_delete(PERSIST_KEY_HABIT_TRACKING_START);
    persist_delete(PERSIST_KEY_HABIT_COUNTDOWN_PAUSED);
    persist_delete(PERSIST_KEY_HABIT_COUNTDOWN_FROZEN_MS);
  }
}

static void load_habit_tracking(void) {
  if (persist_exists(PERSIST_KEY_HABIT_TRACKING_ID)) {
    persist_read_string(PERSIST_KEY_HABIT_TRACKING_ID, s_tracking_habit_id, sizeof(s_tracking_habit_id));
    s_tracking_habit_start_epoch = (time_t)persist_read_int(PERSIST_KEY_HABIT_TRACKING_START);
    s_habit_countdown_paused = persist_exists(PERSIST_KEY_HABIT_COUNTDOWN_PAUSED) &&
                                persist_read_int(PERSIST_KEY_HABIT_COUNTDOWN_PAUSED) != 0;
    s_habit_countdown_frozen_elapsed_ms = persist_exists(PERSIST_KEY_HABIT_COUNTDOWN_FROZEN_MS) ?
                                           persist_read_int(PERSIST_KEY_HABIT_COUNTDOWN_FROZEN_MS) : 0;
  }
}
#endif

static void request_sync(void);
static void hide_error_overlay(void);
static void update_empty_layer(void);
