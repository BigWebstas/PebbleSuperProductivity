// Forward declarations for functions defined in the later sections.
// Textually #included by main.c (one translation unit) - not compiled alone.

static void push_habits_window(void);
static void update_habits_empty_layer(void);
static Task *find_task_by_id(const char *id);
#ifndef PBL_PLATFORM_APLITE
static void push_stats_window(void);
static void stats_render(void);
static void header_begin_reveal(void);
static void header_cancel_reveal(void);
static int16_t header_bar_w(int16_t w);
static void push_schedule_window(void);
static void schedule_refresh_if_open(void);
static void push_page_window(PageMode mode);
static void upcoming_render(void);
#ifdef PBL_PLATFORM_EMERY
static void push_calendar_window(void);
#endif
static void handle_repeat_data(DictionaryIterator *it);
#ifdef PBL_PLATFORM_EMERY
static void handle_calendar_month_data(DictionaryIterator *it);
#endif
#endif
#if PROJECTS_BROWSER
static void push_browse_window(const char *jump_to_project);
static void browse_update_empty(void);
static void request_project_list(void);
static void request_project_tasks(const char *project_id);
static Task *resolve_browse_task_at(MenuIndex index);
#endif
#if PROJECTS_CACHE
static void save_browse_projects(void);
#endif
#ifndef PBL_PLATFORM_APLITE
static void hide_overtime_banner(void);
static void maybe_notify_overtime(void);
static bool has_pinned_row(void);
static int pinned_task_index(void);
static void refresh_pinned_section(void);
static void push_live_window(void);
static void live_window_refresh(void);
static void stop_live_tick(void);
static void focus_end(bool notify);
static const char *presence_state_phrase(void);
static void send_presence_stop(void);
#endif
#ifndef PBL_PLATFORM_APLITE
static void backlight_touch(void);
#else
#define backlight_touch() ((void)0)
#endif

// The shared head of the sub-window loads: root layer, backlight, a status bar
// (into *status), and the content rect below it (the return value). *root is
// the window's root layer, for the caller's own layer_add_child calls.
static GRect window_chrome(Window *window, StatusBarLayer **status, Layer **root) {
  *root = window_get_root_layer(window);
  GRect b = layer_get_bounds(*root);
  backlight_touch();
  *status = add_status_bar(*root);
  return GRect(b.origin.x, b.origin.y + STATUS_BAR_LAYER_HEIGHT,
               b.size.w, b.size.h - STATUS_BAR_LAYER_HEIGHT);
}
#ifndef PBL_PLATFORM_APLITE
#ifdef PBL_PLATFORM_EMERY
static void begin_habit_flash(const char *habit_id, bool milestone);
static void begin_streak_roll(void);
static void habits_marquee_refresh(bool reset_offset);
#endif
static void show_notes_overlay(Task *task);
static void show_project_notes_overlay(TaskGroup *group);
static void hide_notes_overlay(void);
static void push_notes_window(void);
static bool try_open_checklist(void);
static void push_reflect_window(void);
#ifdef PBL_PLATFORM_EMERY
static void push_finishday_window(void);
#endif
typedef enum { PICK_ESTIMATE, PICK_DEADLINE, PICK_HABIT, PICK_TIME } PickKind;
// task_id is a habit id for PICK_HABIT. current: ms (estimate) / days-from-today
// or DEADLINE_NONE (deadline) / the counter's value (habit) / hour 0-23 (time).
static void push_value_picker(PickKind kind, const char *task_id, int current);
static void send_task_set_due_time(const char *task_id, int hour);
typedef enum { ACTX_TODAY, ACTX_PROJECT, ACTX_TAG } ActionCtx;
static void push_action_menu(const char *task_id, ActionCtx ctx, bool in_backlog);
static void pending_reschedule_timer_callback(void *data);
static void cancel_pending_reschedule(void);
static void begin_pending_reschedule(RescheduleKind kind);
#ifdef PBL_PLATFORM_EMERY
static void pending_done_commit_callback(void *data);
static void begin_pending_done(const char *task_id);
static void cancel_pending_done(void);
#endif
static void send_task_reschedule(const char *task_id, RescheduleKind kind, const char *project_id);
static TaskGroup *resolve_project_row_at(MenuIndex index);
#endif
#ifndef PBL_PLATFORM_APLITE
static void start_add_task_dictation(void);
static void start_note_append_dictation(void);
#endif

#if defined(PBL_TOUCH)
// Applies s_touch_nav_enabled: opts into the system touch-nav bridge and
// arms/disarms the raw long-press handler below. Called from init() and when a
// sync reports the setting changed.
static void apply_touch_nav(void);
#endif
