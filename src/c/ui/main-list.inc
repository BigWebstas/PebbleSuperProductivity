// Main task list: section layout, row drawing, menu callbacks, selection/marquee.
// Textually #included by main.c (one translation unit) - not compiled alone.

// ---------- menu layer callbacks ----------

// Section 0 is "Resync", plus "Habits" and/or "Add Task" when enabled - a
// dynamic 1-3 rows. section0_row_count()/section0_row_kind() are the single
// source of truth; every menu callback defers to them. Add Task stays gated by
// PBL_IF_MICROPHONE_ELSE regardless of the setting.
// With tasks: sections 1..s_group_count are the project groups, then one final
// section (group_idx == s_group_count) with a single "Finish Day" row - always
// last, long-select archives every done task, plain Select is a no-op.
// Empty list: no further sections. For NOT_PAIRED/ERROR/initial-syncing,
// section 0 doubles as the phantom retry row; for STATUS_OK with nothing due
// (ACTIONABLE_EMPTY_ACTIVE()), section 0 shows its normal rows with "No tasks
// for today." as the header.
typedef enum {
  SECTION0_ROW_RESYNC,
  SECTION0_ROW_HABITS,
  SECTION0_ROW_PROJECTS, // projects browser, between Habits and Add Task (non-aplite)
  SECTION0_ROW_TAGS,     // tags page, right after Projects, opt-in / default off (non-aplite)
  SECTION0_ROW_STATS,    // stats page, between Projects and Add Task (non-aplite)
  SECTION0_ROW_SCHEDULE, // schedule page, between Stats and Add Task (non-aplite)
  SECTION0_ROW_CALENDAR, // calendar month view, right after Schedule, opt-in / default off (non-aplite)
  SECTION0_ROW_UPCOMING, // upcoming page, between Schedule and Add Task (non-aplite)
  SECTION0_ROW_NOTESPAGE, // notes page, right after Upcoming, opt-in / default off (non-aplite)
  SECTION0_ROW_SEARCH,    // voice search, right after Notes, opt-in / default off (non-aplite, mic)
  SECTION0_ROW_ADD_TASK,
} Section0RowKind;

// Whether the STATUS_OK/zero-tasks empty state shows section 0's normal
// interactive rows (with a "No tasks for today." header) instead of the hidden
// phantom retry row. aplite keeps the old behavior - the extra logic pushed it
// 176 bytes past its APP region. A compile-time macro, not a runtime check, so
// the dead branches are eliminated on aplite.
#ifdef PBL_PLATFORM_APLITE
#define ACTIONABLE_EMPTY_ACTIVE() false
#else
#define ACTIONABLE_EMPTY_ACTIVE() (s_status_code == STATUS_OK)
#endif

// Whether the "Projects" row sits in section 0. Compile-time false where the
// browser isn't built (aplite - see PROJECTS_BROWSER).
#if PROJECTS_BROWSER
#define PROJECTS_ROW_ACTIVE() (s_projects_enabled)
#else
#define PROJECTS_ROW_ACTIVE() false
#endif

// Whether the "Tags" row sits in section 0 - opt-in (default off) and only
// where the browser it reuses is built.
#if PROJECTS_BROWSER
#define TAGS_ROW_ACTIVE() (s_tags_enabled)
#else
#define TAGS_ROW_ACTIVE() false
#endif

// Whether the "Stats" row sits in section 0. Compile-time false on aplite.
#ifdef PBL_PLATFORM_APLITE
#define STATS_ROW_ACTIVE() false
#else
#define STATS_ROW_ACTIVE() (s_stats_enabled)
#endif

// Whether the "Schedule" row sits in section 0. Compile-time false on aplite.
#ifdef PBL_PLATFORM_APLITE
#define SCHEDULE_ROW_ACTIVE() false
#else
#define SCHEDULE_ROW_ACTIVE() (s_schedule_enabled)
#endif

// Whether the "Upcoming" row sits in section 0. Compile-time false on aplite.
#ifdef PBL_PLATFORM_APLITE
#define UPCOMING_ROW_ACTIVE() false
#define NOTESPAGE_ROW_ACTIVE() false
#define SEARCH_ROW_ACTIVE() false
#else
#define UPCOMING_ROW_ACTIVE() (s_upcoming_enabled)
#define NOTESPAGE_ROW_ACTIVE() (s_notespage_enabled)
// Voice search is dictation-driven, so a mic is required (same gate as Add Task).
#define SEARCH_ROW_ACTIVE() (PBL_IF_MICROPHONE_ELSE(s_search_enabled, false))
#endif

// Whether the "Calendar" row sits in section 0 - emery-only (see the month
// view's own comment for why), unlike every other date-browsing row above.
#ifdef PBL_PLATFORM_EMERY
#define CALENDAR_ROW_ACTIVE() (s_calendar_enabled)
#else
#define CALENDAR_ROW_ACTIVE() false
#endif

// A remote presence session shows in the pinned "TRACKING" section
// (remote_in_pinned_section) whenever nothing is tracked locally. There used
// to be a separate dark-blue "LIVE" row at the top of section 0 as well; it
// only duplicated the same task and was removed. In the rare overlap - this
// watch tracking its own task while another device also tracks - the pinned
// slot shows the local task and the remote session is not surfaced
// separately.

static int section0_row_count(void) {
  int count = 1; // Resync always present.
  if (s_habits_enabled) {
    count++;
  }
  if (PROJECTS_ROW_ACTIVE()) {
    count++;
  }
  if (TAGS_ROW_ACTIVE()) {
    count++;
  }
  if (STATS_ROW_ACTIVE()) {
    count++;
  }
  if (SCHEDULE_ROW_ACTIVE()) {
    count++;
  }
  if (CALENDAR_ROW_ACTIVE()) {
    count++;
  }
  if (UPCOMING_ROW_ACTIVE()) {
    count++;
  }
  if (NOTESPAGE_ROW_ACTIVE()) {
    count++;
  }
  if (SEARCH_ROW_ACTIVE()) {
    count++;
  }
  if (PBL_IF_MICROPHONE_ELSE(s_add_task_enabled, false)) {
    count++;
  }
  return count;
}

// Maps a section-0 row index to its action. Resync is row 0; Habits then Add
// Task fill in after it, matching section0_row_count()'s order.
static Section0RowKind section0_row_kind(int row) {
  int next = 0;
  if (row == next) {
    return SECTION0_ROW_RESYNC;
  }
  next++;
  if (s_habits_enabled) {
    if (row == next) {
      return SECTION0_ROW_HABITS;
    }
    next++;
  }
  if (PROJECTS_ROW_ACTIVE()) {
    if (row == next) {
      return SECTION0_ROW_PROJECTS;
    }
    next++;
  }
  if (TAGS_ROW_ACTIVE()) {
    if (row == next) {
      return SECTION0_ROW_TAGS;
    }
    next++;
  }
  if (STATS_ROW_ACTIVE()) {
    if (row == next) {
      return SECTION0_ROW_STATS;
    }
    next++;
  }
  if (SCHEDULE_ROW_ACTIVE()) {
    if (row == next) {
      return SECTION0_ROW_SCHEDULE;
    }
    next++;
  }
  if (CALENDAR_ROW_ACTIVE()) {
    if (row == next) {
      return SECTION0_ROW_CALENDAR;
    }
    next++;
  }
  if (UPCOMING_ROW_ACTIVE()) {
    if (row == next) {
      return SECTION0_ROW_UPCOMING;
    }
    next++;
  }
  if (NOTESPAGE_ROW_ACTIVE()) {
    if (row == next) {
      return SECTION0_ROW_NOTESPAGE;
    }
    next++;
  }
  if (SEARCH_ROW_ACTIVE()) {
    if (row == next) {
      return SECTION0_ROW_SEARCH;
    }
    next++;
  }
  if (PBL_IF_MICROPHONE_ELSE(s_add_task_enabled, false)) {
    if (row == next) {
      return SECTION0_ROW_ADD_TASK;
    }
  }
  return SECTION0_ROW_RESYNC; // unreachable for any row menu_get_num_rows reported
}

#ifndef PBL_PLATFORM_APLITE
// Index into s_tasks of the pinned "TRACKING" task, or -1 if s_pinned_task_id
// is unset or gone from the list. Does NOT check the enable flag - see
// has_pinned_row().
static int pinned_task_index(void) {
  if (s_pinned_task_id[0] == '\0') {
    return -1;
  }
  for (int i = 0; i < s_task_count; i++) {
    if (strncmp(s_tasks[i].id, s_pinned_task_id, MAX_ID_LEN) == 0) {
      return i;
    }
  }
  return -1;
}

// pinned_task_index() converted from a raw s_tasks index to its logical
// (s_row_map) position, for comparing against TaskGroup.start/count - which
// describe logical positions once Later Today has reordered the view. -1
// when there's no pinned task. See rebuild_row_map.
static int pinned_task_row_map_index(void) {
  int raw = pinned_task_index();
  if (raw < 0) {
    return -1;
  }
  for (int k = 0; k < s_task_count; k++) {
    if (s_row_map[k] == raw) {
      return k;
    }
  }
  return -1;
}

// Whether the pinned "TRACKING" section is currently shown: the phone setting
// is on AND there's a real locally-tracked task to put in it, OR a remote
// presence session is riding this section (remote_in_pinned_section).
static bool has_pinned_row(void) {
  return pinned_task_index() >= 0 || remote_in_pinned_section();
}

// Section index of project group 0 - 1 normally, 2 when the pinned section sits
// between section 0 and the groups. Menu callbacks derive a group index as
// (section_index - group_section_base()).
static int group_section_base(void) {
  return 1 + (has_pinned_row() ? 1 : 0);
}
#endif

// group_section_base() as a plain constant on aplite (no pinned section), the
// runtime value elsewhere - lets callbacks skip an #ifdef at every site.
#ifdef PBL_PLATFORM_APLITE
#define GROUP_SECTION_BASE 1
#else
#define GROUP_SECTION_BASE group_section_base()
#endif

#ifndef PBL_PLATFORM_APLITE

// False for a subtask hidden under a collapsed parent (Task.collapsed).
static bool task_row_shown(const Task *t) {
#ifdef PBL_PLATFORM_EMERY
  return !(t->is_subtask && t->collapsed);
#else
  return true;
#endif
}

// How many of group g's tasks are drawn - all, minus the pinned task if it's in
// this group and any subtasks collapsed under their parent. A group with 0 visible tasks collapses entirely.
static int group_visible_task_count(int g) {
  int pinned_idx = has_pinned_row() ? pinned_task_row_map_index() : -1;
  int count = 0;
  for (int i = s_groups[g].start; i < s_groups[g].start + s_groups[g].count; i++) {
    if (i != pinned_idx && task_row_shown(&s_tasks[s_row_map[i]])) {
      count++;
    }
  }
  return count;
}
#endif

// Whether `cell_index` is the always-last "Finish Day" row. With tasks it
// sits past every project group, found via GROUP_SECTION_BASE + s_group_count
// the usual way. With an empty (but actionable - see ACTIONABLE_EMPTY_ACTIVE,
// always false on aplite) list, menu_get_num_sections gives it section 1
// outright instead - s_group_count is 0 there, but GROUP_SECTION_BASE also
// depends on has_pinned_row(), which a remote presence session can hold true
// independent of s_task_count, so this stays a direct check rather than
// reusing that arithmetic. Used from both the aplite and non-aplite row-menu
// callbacks, so it isn't itself aplite-excluded.
static bool is_finish_day_cell(MenuIndex *cell_index) {
  if (s_task_count == 0) {
    return ACTIONABLE_EMPTY_ACTIVE() && cell_index->section == 1;
  }
  return (int)cell_index->section - GROUP_SECTION_BASE == s_group_count;
}

static uint16_t menu_get_num_sections(MenuLayer *menu_layer, void *context) {
  if (s_task_count == 0) {
    // Actionable empty state gets a second section for the always-reachable
    // Finish Day row - completing every task shouldn't lock the day out from
    // being finished. Every other empty reason (not paired, error, syncing)
    // stays the single phantom-row section.
    return ACTIONABLE_EMPTY_ACTIVE() ? 2 : 1;
  }
#ifndef PBL_PLATFORM_APLITE
  return (uint16_t)(1 + (has_pinned_row() ? 1 : 0) + s_group_count + 1);
#else
  return (uint16_t)(1 + s_group_count + 1);
#endif
}

static uint16_t menu_get_num_rows(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  if (s_task_count == 0) {
    // Actionable empty state (STATUS_OK, nothing due): the menu stays visible
    // with its normal section-0 rows, plus a section 1 holding just the
    // Finish Day row. Every other empty reason hides the menu but still needs
    // one reportable row so SELECT dispatches ("Select to retry"); that row
    // is never drawn since the layer is hidden.
    if (!ACTIONABLE_EMPTY_ACTIVE()) {
      return 1;
    }
    if (section_index == 1) {
      return 1; // Finish Day row
    }
    return (uint16_t)section0_row_count();
  }
  if (section_index == 0) {
    return (uint16_t)section0_row_count();
  }
#ifndef PBL_PLATFORM_APLITE
  // The pinned "TRACKING" section at index 1 - one row, the tracked task.
  if (has_pinned_row() && section_index == 1) {
    return 1;
  }
#endif
  int group_idx = (int)section_index - GROUP_SECTION_BASE;
  if (group_idx == s_group_count) {
    return 1; // Finish Day row
  }
  if (group_idx > s_group_count) {
    return 0;
  }
#ifndef PBL_PLATFORM_APLITE
  // A named group gets one extra row up front - a selectable "project row"
  // standing in for the plain header (double-click Select shows its notes).
  // aplite keeps the plain non-interactive header.
  int visible = group_visible_task_count(group_idx);
  if (visible == 0) {
    return 0; // whole group collapsed - its only task is pinned at the top
  }
  if (s_groups[group_idx].name[0] != '\0') {
    if (s_collapse_groups && s_groups[group_idx].user_collapsed) {
      return 1; // just the project row - its tasks are hidden
    }
    return (uint16_t)(visible + 1);
  }
  return (uint16_t)visible;
#else
  return (uint16_t)s_groups[group_idx].count;
#endif
}

static int16_t menu_get_header_height(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  if (s_task_count == 0) {
    // Actionable empty state only - "Nothing left for today." is section 1's
    // header, sitting just above Finish Day: after the pinned action rows in
    // section 0, not over them.
    return (section_index == 1 && ACTIONABLE_EMPTY_ACTIVE()) ? GROUP_HEADER_HEIGHT : 0;
  }
  if (section_index == 0) {
    return 0;
  }
#ifndef PBL_PLATFORM_APLITE
  // The pinned "TRACKING" section gets a short labelled header strip.
  if (has_pinned_row() && section_index == 1) {
    return PINNED_HEADER_HEIGHT;
  }
#endif
  int group_idx = (int)section_index - GROUP_SECTION_BASE;
  // Empty group name = grouping off (one '' group) - no header, flat list.
  if (group_idx >= s_group_count || s_groups[group_idx].name[0] == '\0') {
    return 0;
  }
#ifndef PBL_PLATFORM_APLITE
  // A group whose only task is pinned at the top collapses whole - no header.
  if (group_visible_task_count(group_idx) == 0) {
    return 0;
  }
  // Non-aplite: the group name lives in a selectable project row instead
  // (menu_get_num_rows/menu_draw_row), so no separate header.
  return 0;
#else
  return GROUP_HEADER_HEIGHT;
#endif
}

#ifndef PBL_PLATFORM_APLITE
// A project's theme-colour swatch: a 16px square filled with the phone-packed
// GColor8 byte, centred vertically at x. Returns the x for the following text -
// unchanged (nothing drawn) when the project has no colour. Shared by the
// grouped today view's project rows and the Projects browser's list.
static int16_t draw_project_swatch(GContext *ctx, int16_t x, int16_t cell_h, uint8_t color) {
  if (color == 0) {
    return x;
  }
  fill_bg(ctx, GRect(x, (cell_h - 16) / 2, 16, 16), (GColor8){ .argb = color });
  return x + 22;
}

// Super Productivity's built-in default project - a fixed id, the same on every
// instance (see index.js's Inbox fallback for Add Task).
#define INBOX_PROJECT_ID "INBOX_PROJECT"

// The left-of-title marker on a project row: the Inbox glyph for the default
// project (which typically has no theme colour to show a swatch for), else the
// project's colour swatch. Returns the x the title text should start at.
static int16_t draw_project_marker(GContext *ctx, int16_t x, int16_t cell_h,
                                    const char *project_id, uint8_t color, bool is_selected) {
  if (project_id && strcmp(project_id, INBOX_PROJECT_ID) == 0) {
    graphics_context_set_compositing_mode(ctx, GCompOpSet);
    graphics_draw_bitmap_in_rect(ctx, is_selected ? s_inbox_white_bitmap : s_inbox_bitmap,
                                  GRect(x, (cell_h - 16) / 2, 16, 16));
    return x + 22;
  }
  return draw_project_swatch(ctx, x, cell_h, color);
}
#endif

#ifndef PBL_PLATFORM_APLITE
// Defined further down with the marquee helpers; used by the subtitle marquee
// and the pinned tracking row below.
static int16_t title_natural_width_font(const char *title, GFont font);
#endif

static void menu_draw_header(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *context) {
  if (s_task_count == 0) {
    if (section_index == 1 && ACTIONABLE_EMPTY_ACTIVE()) {
      // "Nothing left for today." - below the pinned section-0 action rows,
      // right above Finish Day.
      GRect bounds = layer_get_bounds(cell_layer);
      fill_bg(ctx, bounds, GColorWhite);
      graphics_context_set_text_color(ctx, GColorBlack);
      draw_text(ctx, "Nothing left for today.", EMPTY_MSG_FONT_KEY, bounds, GTextOverflowModeWordWrap, GTextAlignmentCenter);
    }
    return;
  }
  if (section_index == 0) {
    return;
  }
#ifndef PBL_PLATFORM_APLITE
  // The pinned tracking header - a thin green strip. Bold (CHROME_FONT_BOLD_KEY,
  // same point size as CHROME_FONT_KEY) so it reads at a glance. Static, centred;
  // the "live" cue is the pulse dot on the row below.
  if (has_pinned_row() && section_index == 1) {
    GRect hb = layer_get_bounds(cell_layer);
    fill_bg(ctx, hb, GColorGreen);
    graphics_context_set_text_color(ctx, GColorBlack);
    const char *label = !focus_active() ? "TRACKING"
                        : s_focus_on_break ? "BREAK"
                        : s_focus_end_epoch == 0 ? "FLOWING"
                        : "FOCUSING";
    graphics_draw_text(ctx, label, fonts_get_system_font(CHROME_FONT_BOLD_KEY),
                       GRect(0, 0, hb.size.w, hb.size.h),
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
    graphics_context_set_stroke_color(ctx, GColorBlack);
    graphics_draw_line(ctx, GPoint(0, hb.size.h - 1), GPoint(hb.size.w, hb.size.h - 1));
    return;
  }
#endif
  int group_idx = (int)section_index - GROUP_SECTION_BASE;
  if (group_idx >= s_group_count || s_groups[group_idx].name[0] == '\0') {
    return;
  }
  const char *name = s_groups[group_idx].name;
  GRect bounds = layer_get_bounds(cell_layer);
  GFont bold_font = fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD);
  GRect text_rect = GRect(6, 2, bounds.size.w - 12, bounds.size.h - 4);

  // Fill first - a MenuLayer header has no built-in background, so the text and
  // lines below would otherwise draw onto stale framebuffer content. This
  // whole branch only actually renders on aplite - menu_get_header_height
  // returns 0 for this section everywhere else, where the group name lives
  // in a selectable "project row" drawn by menu_draw_row instead (see the
  // wipe-in on that row below for the animated version of this).
  fill_bg(ctx, bounds, GColorGreen);

  graphics_context_set_text_color(ctx, GColorBlack);
  graphics_draw_text(ctx, name, bold_font, text_rect,
                      GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

  // Full-width divider separating this group from its tasks.
  int16_t divider_y = bounds.size.h - 2;
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_draw_line(ctx, GPoint(0, divider_y), GPoint(bounds.size.w, divider_y));
}

// Looks up a task by id - a resync can rebuild s_tasks wholesale, so a Task*
// captured earlier (e.g. at start_tracking()) can't be trusted.
static Task *find_task_by_id(const char *id) {
  for (int i = 0; i < s_task_count; i++) {
    if (strncmp(s_tasks[i].id, id, MAX_ID_LEN) == 0) {
      return &s_tasks[i];
    }
  }
  return NULL;
}

#ifndef PBL_PLATFORM_APLITE
// find_task_by_id for the habits list - only used by the habit-tracking
// functions, themselves aplite-excluded.
static Habit *find_habit_by_id(const char *id) {
  for (int i = 0; i < s_habit_count; i++) {
    if (strncmp(s_habits[i].id, id, MAX_HABIT_ID_LEN) == 0) {
      return &s_habits[i];
    }
  }
  return NULL;
}
#endif

// Resolves a MenuIndex to the Task it points at, or NULL if it isn't on a task
// row (Resync, a project row, Finish Day, or out of range - an empty list has
// s_group_count 0 and falls through the same group_idx bounds check).
static Task *resolve_task_at(MenuIndex index) {
  if (index.section == 0) {
    return NULL;
  }
#ifndef PBL_PLATFORM_APLITE
  // The pinned "TRACKING" section's single row IS the tracked task - Select
  // (toggle), long-Select (stop tracking) and double-click (notes) all fall
  // through to the same handlers a normal task row uses.
  if (has_pinned_row() && index.section == 1) {
    int pi = pinned_task_index();
    return pi >= 0 ? &s_tasks[pi] : NULL;
  }
#endif
  int group_idx = (int)index.section - GROUP_SECTION_BASE;
  if (group_idx >= s_group_count) {
    return NULL;
  }
  int row = (int)index.row;
#ifndef PBL_PLATFORM_APLITE
  // Row 0 of a named group is the selectable project row, not a task; every
  // other row shifts down by one.
  if (s_groups[group_idx].name[0] != '\0') {
    if (row == 0) {
      return NULL;
    }
    row -= 1;
  }
  // Walk the group's tasks skipping the pinned one (drawn in the pinned
  // section), so visible row N is the Nth non-pinned task. `i` is a logical
  // (s_row_map) position - see rebuild_row_map.
  int pinned_idx = has_pinned_row() ? pinned_task_row_map_index() : -1;
  int start = s_groups[group_idx].start;
  int end = start + s_groups[group_idx].count;
  int seen = 0;
  for (int i = start; i < end && i < s_task_count; i++) {
    if (i == pinned_idx || !task_row_shown(&s_tasks[s_row_map[i]])) {
      continue;
    }
    if (seen == row) {
      return &s_tasks[s_row_map[i]];
    }
    seen++;
  }
  return NULL;
#else
  int task_idx = s_groups[group_idx].start + row;
  if (task_idx < 0 || task_idx >= s_task_count) {
    return NULL;
  }
  return &s_tasks[task_idx];
#endif
}

#ifndef PBL_PLATFORM_APLITE
// Resolves a MenuIndex to a TaskGroup if it's on that group's selectable
// project row (row 0 of a named group), else NULL.
static TaskGroup *resolve_project_row_at(MenuIndex index) {
  if (index.section == 0) {
    return NULL;
  }
  if (has_pinned_row() && index.section == 1) {
    return NULL;
  }
  int group_idx = (int)index.section - GROUP_SECTION_BASE;
  if (group_idx >= s_group_count) {
    return NULL;
  }
  if (s_groups[group_idx].name[0] == '\0' || (int)index.row != 0) {
    return NULL;
  }
  // A fully-collapsed group (its only task pinned) shows no project row.
  if (group_visible_task_count(group_idx) == 0) {
    return NULL;
  }
  return &s_groups[group_idx];
}
#endif

// Resolves the row MenuLayer currently has highlighted to a Task, or NULL
// if the selection isn't on a task row at all.
static Task *resolve_selected_task(void) {
  return resolve_task_at(menu_layer_get_selected_index(s_menu_layer));
}

static int16_t title_natural_width(const char *title) {
  GSize size = graphics_text_layout_get_content_size(
      title, fonts_get_system_font(TITLE_FONT_KEY), GRect(0, 0, 2000, 100),
      GTextOverflowModeFill, GTextAlignmentLeft);
  return size.w;
}

#ifndef PBL_PLATFORM_APLITE
static int16_t title_natural_width_font(const char *title, GFont font) {
  GSize size = graphics_text_layout_get_content_size(
      title, font, GRect(0, 0, 2000, 100),
      GTextOverflowModeFill, GTextAlignmentLeft);
  return size.w;
}

// Draws `text` in `box`: left-aligned with a trailing ellipsis normally, or -
// when `marquee` is set - as a looping two-copy horizontal scroll offset by
// s_scroll_offset_px (whose timer refresh_scroll_state owns). Used by the
// live-tracking presence rows; the task rows have their own inline copy.
static void draw_marquee_title(GContext *ctx, GRect box, const char *text, GFont font, bool marquee) {
  if (marquee) {
    int16_t natural_width = title_natural_width_font(text, font);
    int16_t period = natural_width + SCROLL_GAP_PX;
    int16_t x = -(s_scroll_offset_px % period);
    graphics_draw_text(ctx, text, font,
                        GRect(box.origin.x + x, box.origin.y, natural_width, box.size.h),
                        GTextOverflowModeFill, GTextAlignmentLeft, NULL);
    graphics_draw_text(ctx, text, font,
                        GRect(box.origin.x + x + period, box.origin.y, natural_width, box.size.h),
                        GTextOverflowModeFill, GTextAlignmentLeft, NULL);
  } else {
    graphics_draw_text(ctx, text, font, box, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  }
}

#endif

// Formats due_min (minutes since local midnight) as "@ 9:41 AM" / "@ 21:41",
// respecting the watch's 12h/24h setting. The phone already sent local time.
// relative: within the next 90 min show "in 25m" / "in 1h 05m" instead - more
// useful on a task row, but NOT for the launcher glance (it would go stale).
static void format_due_time(int due_min, bool relative, char *out, size_t out_len) {
  if (relative) {
    time_t nowt = time(NULL);
    struct tm *lt = localtime(&nowt);
    int delta = due_min - (lt->tm_hour * 60 + lt->tm_min);
    if (delta > 0 && delta <= 90) {
      if (delta < 60) {
        snprintf(out, out_len, "in %dm", delta);
      } else {
        snprintf(out, out_len, "in 1h %02dm", delta - 60);
      }
      return;
    }
  }
  int h = due_min / 60;
  int m = due_min % 60;
  if (clock_is_24h_style()) {
    snprintf(out, out_len, "@ %d:%02d", h, m);
  } else {
    int h12 = h % 12;
    if (h12 == 0) {
      h12 = 12;
    }
    snprintf(out, out_len, "@ %d:%02d %s", h12, m, h < 12 ? "AM" : "PM");
  }
}

// Formats tracked time as "1h 23m" (seconds dropped), "5m 09s", or "42s".
// is_tracking prefixes "> " so a live number reads as running vs a static total.
static void format_duration_ms(int ms, bool is_tracking, char *out, size_t out_len) {
  int total_s = ms / 1000;
  int h = total_s / 3600;
  int m = (total_s % 3600) / 60;
  int s = total_s % 60;
  const char *prefix = is_tracking ? "> " : "";
  if (h > 0) {
    snprintf(out, out_len, "%s%dh %02dm", prefix, h, m);
  } else if (m > 0) {
    snprintf(out, out_len, "%s%dm %02ds", prefix, m, s);
  } else {
    snprintf(out, out_len, "%s%ds", prefix, s);
  }
}

static void stop_scroll_timer(void) {
  if (s_scroll_timer) {
    app_timer_cancel(s_scroll_timer);
    s_scroll_timer = NULL;
  }
}

#ifndef PBL_PLATFORM_APLITE
// "Tracking on Desktop" / "Paused on Desktop" / "Was tracking on Desktop", or a
// device-less form when the presence payload could not be decoded phone-side.
static const char *presence_state_phrase(void) {
  static char buf[48];
  // State 5: the phone's presence socket dropped mid-session (index.js's
  // pushPresenceOffline). Not "<verb> on <device>" - the device is fine, our
  // link to it isn't.
  if (s_presence_state == 5) {
    return "Offline";
  }
  const char *verb = s_presence_state == 2 ? "Paused"
                     : s_presence_state == 3 ? "Was tracking"
                     : s_presence_state == 4 ? "Stopped"
                     : "Tracking";
  if (s_presence_device[0] != '\0') {
    snprintf(buf, sizeof(buf), "%s on %s", verb, s_presence_device);
  } else {
    snprintf(buf, sizeof(buf), "%s on another device", verb);
  }
  return buf;
}
#endif

static void scroll_timer_callback(void *data) {
  s_scroll_offset_px += SCROLL_STEP_PX;
#ifndef PBL_PLATFORM_APLITE
  if (s_pending_reschedule_kind != RESCHEDULE_NONE) {
    s_pending_reschedule_tick++;
  }
#endif
#ifdef PBL_PLATFORM_EMERY
  if (s_pending_done_task_id[0] != '\0') {
    s_pending_done_tick++;
  }
  if (s_sync_check_active) {
    s_sync_check_tick++;
    if (s_sync_check_tick * SCROLL_INTERVAL_MS >= SYNC_GREEN_MS) {
      s_sync_check_active = false;
    }
  }
  if (s_addtask_flash_active) {
    s_addtask_flash_tick++;
    if (s_addtask_flash_tick * SCROLL_INTERVAL_MS >= ADDTASK_FLASH_MS) {
      s_addtask_flash_active = false;
    }
  }
  if (s_lasthabit_flash_active) {
    s_lasthabit_flash_tick++;
    if (s_lasthabit_flash_tick * SCROLL_INTERVAL_MS >= LASTHABIT_FLASH_MS) {
      s_lasthabit_flash_active = false;
    }
  }
#endif
  layer_mark_dirty(menu_layer_get_layer(s_menu_layer));
#if PROJECTS_BROWSER
  // A pending reschedule started from the browser task view animates its bar
  // there, not on the today list.
  if (s_browse_menu && s_pending_reschedule_kind != RESCHEDULE_NONE) {
    layer_mark_dirty(menu_layer_get_layer(s_browse_menu));
  }
#endif
  s_scroll_timer = app_timer_register(SCROLL_INTERVAL_MS, scroll_timer_callback, NULL);
}

#ifndef PBL_PLATFORM_APLITE
// Whether the currently selected row is the remote row in the pinned
// "TRACKING" section - it draws s_presence_task and so can also want a
// marquee when the name overflows.
static bool selected_row_is_presence_title(void) {
  if (s_presence_task[0] == '\0') {
    return false;
  }
  MenuIndex sel = menu_layer_get_selected_index(s_menu_layer);
  return remote_in_pinned_section() && sel.section == 1 && sel.row == 0;
}
#endif

// Starts/stops the marquee timer to match whether the selected row needs it,
// optionally resetting the scroll position. Called on selection or list change.
// Assembles a task row's subtitle: issue key, then the deadline marker, the
// due time, and spent/estimate - each appended with a separator only when
// something's already there. Shared by draw_task_row and refresh_scroll_state
// (which needs the text to decide whether to run the marquee timer). Does NOT
// cover the "Done" / pending-reschedule cases - draw_task_row handles those
// before calling here, and they never need to scroll.
static void build_task_subtitle(Task *task, char *out, size_t cap) {
  out[0] = '\0';
  bool is_tracking_this = s_tracking_task_id[0] != '\0' &&
                           strncmp(s_tracking_task_id, task->id, MAX_ID_LEN) == 0;
  int effective_ms = task->time_spent_ms;
  if (is_tracking_this) {
    time_t elapsed_s = time(NULL) - s_tracking_start_epoch;
    if (elapsed_s > 0) {
      effective_ms += (int)elapsed_s * 1000;
    }
  }
#ifndef PBL_PLATFORM_APLITE
  if (task->issue_key[0] != '\0') {
    str_copy(out, task->issue_key, cap);
  }
#endif
  if (task->deadline_days != DEADLINE_NONE) {
    char dl_text[16];
    if (task->deadline_days < 0) {
      str_copy(dl_text, "! overdue", sizeof(dl_text));
    } else if (task->deadline_days == 0) {
      str_copy(dl_text, "! today", sizeof(dl_text));
    } else if (task->deadline_days == 1) {
      str_copy(dl_text, "! tomorrow", sizeof(dl_text));
    } else if (task->deadline_days <= 6) {
      // Within the week - the weekday name places it faster than a day count.
      static const char *const WD[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
      time_t nowt = time(NULL);
      struct tm *lt = localtime(&nowt);
      snprintf(dl_text, sizeof(dl_text), "! %s", WD[(lt->tm_wday + task->deadline_days) % 7]);
    } else {
      snprintf(dl_text, sizeof(dl_text), "! %dd", task->deadline_days);
    }
    size_t n = strlen(out);
    if (n > 0) {
      snprintf(out + n, cap - n, "  %s", dl_text);
    } else {
      str_copy(out, dl_text, cap);
    }
  }
  if (task->due_min >= 0) {
    char due_text[16];
    format_due_time(task->due_min, true, due_text, sizeof(due_text));
    size_t n = strlen(out);
    if (n > 0) {
      snprintf(out + n, cap - n, "  %s", due_text);
    } else {
      str_copy(out, due_text, cap);
    }
  }
  if (effective_ms > 0 || is_tracking_this) {
    char time_text[20];
    format_duration_ms(effective_ms, is_tracking_this, time_text, sizeof(time_text));
    if (task->time_estimate_ms > 0) {
      char estimate_text[20];
      format_duration_ms(task->time_estimate_ms, false, estimate_text, sizeof(estimate_text));
      char combined[48];
      // "! " when the spent time has passed the estimate.
      snprintf(combined, sizeof(combined), "%s%s / %s",
               effective_ms > task->time_estimate_ms ? "! " : "", time_text, estimate_text);
      str_copy(time_text, combined, sizeof(time_text));
    }
    size_t n = strlen(out);
    if (n > 0) {
      snprintf(out + n, cap - n, "%s%s", is_tracking_this ? "  " : " - ", time_text);
    } else {
      str_copy(out, time_text, cap);
    }
  }
}

static void refresh_scroll_state(bool reset_offset) {
  if (reset_offset) {
    s_scroll_offset_px = 0;
  }
  Task *selected = resolve_selected_task();
  GRect menu_bounds = layer_get_bounds(menu_layer_get_layer(s_menu_layer));
  int16_t available = menu_bounds.size.w - TITLE_BOX_X * 2;
  bool needs_scroll = selected && title_natural_width(selected->title) > available;
#ifndef PBL_PLATFORM_APLITE
  // Keep the repaint timer alive while a transient row animation is running.
  if (s_pending_reschedule_kind != RESCHEDULE_NONE
#ifdef PBL_PLATFORM_EMERY
      || s_pending_done_task_id[0] != '\0' || s_sync_check_active || s_addtask_flash_active ||
      s_lasthabit_flash_active
#endif
     ) {
    needs_scroll = true;
  }
  if (!needs_scroll && selected && !selected->done) {
    char sub[56];
    build_task_subtitle(selected, sub, sizeof(sub));
    if (sub[0] != '\0' &&
        title_natural_width_font(sub, fonts_get_system_font(SUBTITLE_FONT_KEY)) > available) {
      needs_scroll = true;
    }
  }
#endif
#ifndef PBL_PLATFORM_APLITE
  if (!needs_scroll && selected_row_is_presence_title()) {
    needs_scroll = title_natural_width(s_presence_task) > available;
  }
  // Keep the repaint timer running the whole time this watch is tracking: the
  // pinned tracking header's word sweeps left<->right for the entire session,
  // regardless of the selected row.
  if (has_pinned_row() && s_tracking_task_id[0] != '\0') {
    needs_scroll = true;
  }
#endif
  if (needs_scroll && !s_scroll_timer) {
    s_scroll_timer = app_timer_register(SCROLL_INTERVAL_MS, scroll_timer_callback, NULL);
  } else if (!needs_scroll) {
    stop_scroll_timer();
  }
}

static void menu_selection_changed(MenuLayer *menu_layer, MenuIndex new_index, MenuIndex old_index, void *context) {
  refresh_scroll_state(true);
  backlight_touch();
#ifdef PBL_PLATFORM_EMERY
  if (s_scroll_pip_layer) {
    layer_mark_dirty(s_scroll_pip_layer);
  }
#endif
#if defined(PBL_TOUCH)
  // A tap that moved the highlight to a new row is select-only; a tap on the
  // already-selected row (no move) activates it - see menu_select_click.
  s_tap_moved_sel = true;
#endif
}

#ifndef PBL_PLATFORM_APLITE
static void backlight_timer_callback(void *data) {
  s_backlight_timer = NULL;
  // Hands control back to automatic backlight behavior, not "force off". With
  // no button press at this instant, auto control has nothing to keep it lit,
  // which is what makes the custom timeout duration real.
  light_enable(false);
}

// Called on every button interaction (select/long-select/scroll) - NOT on a
// settings change (that's apply_backlight_mode()). Mode 0 is a no-op: the app
// never touches the backlight API unless the phone opts into another mode.
static void backlight_touch(void) {
  if (s_backlight_timer) {
    app_timer_cancel(s_backlight_timer);
    s_backlight_timer = NULL;
  }
  if (s_backlight_mode == 0) {
    return;
  }
  light_enable(true);
  if (s_backlight_mode == BACKLIGHT_MODE_ALWAYS_ON) {
    return; // Stays on until the mode itself changes - see apply_backlight_mode().
  }
  s_backlight_timer = app_timer_register(s_backlight_mode * 1000, backlight_timer_callback, NULL);
}

// Reacts to s_backlight_mode changing (a settings save via MSG_SYNC_STATUS),
// not to user interaction. Leaving always-on needs an explicit light_enable(false)
// here - backlight_touch() only turns it on, and the timeout timer never runs
// in always-on mode.
static void apply_backlight_mode(void) {
  if (s_backlight_mode == 0) {
    if (s_backlight_timer) {
      app_timer_cancel(s_backlight_timer);
      s_backlight_timer = NULL;
    }
    light_enable(false);
    return;
  }
  backlight_touch();
}
#endif // !PBL_PLATFORM_APLITE

// Recurring-task glyph: an open circle-arrow, drawn at the very start of the
// subtitle line (ahead of the issue-source glyph and issue_key text, if any -
// see draw_task_row). All-platform, like task->recurs itself. `box` is the
// left edge of the subtitle line; the glyph is centred in its height.
#define RECURS_GLYPH_W 12
static void draw_recurs_glyph(GContext *ctx, GRect box, GColor fg) {
  int16_t gx = box.origin.x;
  int16_t gy = box.origin.y + box.size.h / 2;
  graphics_context_set_stroke_color(ctx, fg);
  graphics_draw_arc(ctx, GRect(gx, gy - 5, 11, 11), GOvalScaleModeFitCircle,
                    DEG_TO_TRIGANGLE(35), DEG_TO_TRIGANGLE(330));
  graphics_draw_line(ctx, GPoint(gx + 9, gy - 5), GPoint(gx + 12, gy - 2));
  graphics_draw_line(ctx, GPoint(gx + 9, gy - 5), GPoint(gx + 6, gy - 3));
}

// Subtask glyph: a tree-connector stem dropping from the row's top edge into
// a short arm pointing at the title - reads as "nests under the row above".
// Drawn at the start of the title line, indenting the title text to match.
// Replaced an earlier phone-baked "»" text prefix (worked, but depended on
// the watch font having that exact glyph - see task-store.js history).
#define SUBTASK_INDENT_W 14
static void draw_subtask_glyph(GContext *ctx, GRect box, GColor fg) {
  int16_t x = box.origin.x + 3;
  int16_t y_mid = box.origin.y + box.size.h / 2;
  graphics_context_set_stroke_color(ctx, fg);
  graphics_draw_line(ctx, GPoint(x, box.origin.y), GPoint(x, y_mid));
  graphics_draw_line(ctx, GPoint(x, y_mid), GPoint(x + 6, y_mid));
}

#ifndef PBL_PLATFORM_APLITE
// Small monochrome glyph marking which external tracker a task's issue link
// came from, drawn at the start of the subtitle line, after the recurring
// glyph (if any) and ahead of the issue_key text, or alone for a CalDAV/iCal
// task whose uid was too long for a text badge.
// Plain vector shapes, no bitmap resource - real per-provider logos were
// tried for habits (built then reverted, see 1d99447) for pbpack + heap cost
// basalt/chalk/diorite can't spare; this costs neither. `box` is the left
// edge of the subtitle line; the glyph is centred in its height.
#define ISSUE_GLYPH_W 12
static void draw_issue_src_glyph(GContext *ctx, GRect box, uint8_t src, GColor fg) {
  int16_t cx = box.origin.x + 4;
  int16_t cy = box.origin.y + box.size.h / 2;
  graphics_context_set_stroke_color(ctx, fg);
  graphics_context_set_fill_color(ctx, fg);
  switch (src) {
    case ISSUE_SRC_GIT: {
      // A branch/fork: three spokes off a centre point, dotted at the tips.
      GPoint top = GPoint(cx + 2, cy - 4);
      GPoint bl = GPoint(cx - 3, cy + 3);
      GPoint br = GPoint(cx + 3, cy + 3);
      GPoint mid = GPoint(cx, cy);
      graphics_draw_line(ctx, mid, top);
      graphics_draw_line(ctx, mid, bl);
      graphics_draw_line(ctx, mid, br);
      graphics_fill_circle(ctx, top, 1);
      graphics_fill_circle(ctx, bl, 1);
      graphics_fill_circle(ctx, br, 1);
      break;
    }
    case ISSUE_SRC_TICKET: {
      // A ticket stub: an outline with one perforation tick near the edge.
      GRect r = GRect(cx - 4, cy - 3, 9, 7);
      graphics_draw_rect(ctx, r);
      graphics_draw_line(ctx, GPoint(r.origin.x + 3, r.origin.y + 1),
                         GPoint(r.origin.x + 3, r.origin.y + r.size.h - 2));
      break;
    }
    case ISSUE_SRC_CALENDAR: {
      // A wall calendar: an outline, a header rule, two binder ticks on top.
      GRect r = GRect(cx - 4, cy - 2, 9, 8);
      graphics_draw_rect(ctx, r);
      graphics_draw_line(ctx, GPoint(r.origin.x, r.origin.y + 2),
                         GPoint(r.origin.x + r.size.w - 1, r.origin.y + 2));
      graphics_draw_line(ctx, GPoint(r.origin.x + 2, r.origin.y - 2), GPoint(r.origin.x + 2, r.origin.y));
      graphics_draw_line(ctx, GPoint(r.origin.x + r.size.w - 3, r.origin.y - 2),
                         GPoint(r.origin.x + r.size.w - 3, r.origin.y));
      break;
    }
    default:
      break;
  }
}
#endif

#ifndef PBL_PLATFORM_APLITE
// Priority icon, matching the desktop's default chevron preset: a single
// chevron down (low), a single chevron up (medium), a double chevron up (high).
// Coloured red / orange / cerulean. `box` is the right-aligned slot,
// PRIORITY_GLYPH_W wide; the icon is centred in it.
#define PRIORITY_GLYPH_W 12
static void draw_priority_glyph(GContext *ctx, GRect box, uint8_t prio, GColor fg) {
  int16_t cx = box.origin.x + box.size.w / 2;
  int16_t cy = box.origin.y + box.size.h / 2 + 1;
  GColor c = prio == PRIO_HIGH ? PBL_IF_COLOR_ELSE(GColorRed, fg)
           : prio == PRIO_MEDIUM ? PBL_IF_COLOR_ELSE(GColorOrange, fg)
           : PBL_IF_COLOR_ELSE(GColorVividCerulean, fg);
  graphics_context_set_stroke_color(ctx, c);
  graphics_context_set_stroke_width(ctx, 2);
  int16_t dir = prio == PRIO_LOW ? -1 : 1; // chevron points down for low, else up
  for (int i = 0; i < (prio == PRIO_HIGH ? 2 : 1); i++) {
    int16_t y = cy + (prio == PRIO_HIGH ? 2 - i * 4 : 0);
    graphics_draw_line(ctx, GPoint(cx - 4, y + 2 * dir), GPoint(cx, y - 2 * dir));
    graphics_draw_line(ctx, GPoint(cx, y - 2 * dir), GPoint(cx + 4, y + 2 * dir));
  }
  graphics_context_set_stroke_width(ctx, 1);
}
#endif

// Draws one task row - marquee/ellipsized title plus the "@ due  > spent /
// estimate" subtitle. Shared by per-group rows and the pinned "TRACKING" row.
// show_project right-aligns task->project on the subtitle line; only the pinned
// row passes true (and only shows it when grouping is on). `bounds` is a
// MenuLayer cell's own bounds (origin 0,0).
static void draw_task_row(GContext *ctx, GRect bounds, Task *task, bool is_selected, bool show_project) {
  int16_t available = bounds.size.w - TITLE_BOX_X * 2;
  if (task->is_subtask) {
    available -= SUBTASK_INDENT_W;
  }
  int16_t natural_width = title_natural_width(task->title);
  bool needs_marquee = is_selected && natural_width > available;

  GColor bg = is_selected ? GColorBlack : GColorWhite;
  GColor fg = is_selected ? GColorWhite : GColorBlack;
  if (task->done) {
    fg = is_selected ? GColorLightGray : GColorDarkGray;
  }
  fill_bg(ctx, bounds, bg);
#ifdef PBL_COLOR
  // Deadline urgency: a stripe down the left edge - red once overdue, amber for
  // due today. Not for a done task (the deadline no longer matters).
  if (!task->done && task->deadline_days != DEADLINE_NONE && task->deadline_days <= 0) {
    graphics_context_set_fill_color(ctx, task->deadline_days < 0 ? GColorRed : GColorOrange);
#ifdef PBL_PLATFORM_EMERY
    // Breathes 2px<->4px off the shared scroll offset - a quiet, ongoing "this
    // still needs you" rather than a static bar. Only actually animates while
    // something else keeps the repaint timer running (a marquee, a pending
    // window); otherwise it just sits at whatever phase that left it.
    int ph = ((s_scroll_offset_px % 40) + 40) % 40;
    int up = ph < 20 ? ph : 40 - ph;
    graphics_fill_rect(ctx, GRect(0, 0, 2 + up / 10, bounds.size.h), 0, GCornerNone);
#else
    graphics_fill_rect(ctx, GRect(0, 0, 3, bounds.size.h), 0, GCornerNone);
#endif
  }
#endif
  graphics_context_set_text_color(ctx, fg);

  GFont title_font = fonts_get_system_font(TITLE_FONT_KEY);
  GSize one_line_size = graphics_text_layout_get_content_size(
      "Ag", title_font, GRect(0, 0, 200, 100), GTextOverflowModeFill, GTextAlignmentLeft);
  int16_t title_box_h = one_line_size.h > 0 ? one_line_size.h : (bounds.size.h - TITLE_BOX_Y);
  GRect title_box = GRect(TITLE_BOX_X, ROW_TITLE_TOP_Y(bounds.size.h, title_box_h, SUBTITLE_STRIP_H),
                           bounds.size.w - TITLE_BOX_X * 2, title_box_h);
  if (task->is_subtask) {
    draw_subtask_glyph(ctx, title_box, fg);
    title_box.origin.x += SUBTASK_INDENT_W;
    title_box.size.w -= SUBTASK_INDENT_W;
  }

  if (needs_marquee) {
    int16_t period = natural_width + SCROLL_GAP_PX;
    int16_t x = -(s_scroll_offset_px % period);
    graphics_draw_text(ctx, task->title, title_font,
                        GRect(title_box.origin.x + x, title_box.origin.y, natural_width, title_box.size.h),
                        GTextOverflowModeFill, GTextAlignmentLeft, NULL);
    graphics_draw_text(ctx, task->title, title_font,
                        GRect(title_box.origin.x + x + period, title_box.origin.y, natural_width, title_box.size.h),
                        GTextOverflowModeFill, GTextAlignmentLeft, NULL);
  } else {
    graphics_draw_text(ctx, task->title, title_font, title_box,
                        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
  }

  GRect subtitle_box = GRect(TITLE_BOX_X, ROW_SUBTITLE_TOP_Y(bounds.size.h, title_box_h, SUBTITLE_STRIP_H),
                              bounds.size.w - TITLE_BOX_X * 2, SUBTITLE_STRIP_H);

#ifndef PBL_PLATFORM_APLITE
  // A pending reschedule (and, on emery, a pending done-toggle) takes over the
  // whole subtitle line with a centred message; emery also draws a centre-
  // anchored bar that shrinks as the cancel window runs out. A Select commits
  // early / cancels. See begin_pending_reschedule / begin_pending_done.
  {
    const char *pending_msg = NULL;
    int pend_total_ms = 0, pend_tick = 0;
    if (s_pending_reschedule_kind != RESCHEDULE_NONE &&
        strncmp(s_pending_reschedule_task_id, task->id, MAX_ID_LEN) == 0) {
#ifdef PBL_PLATFORM_EMERY
      static char at_msg[28];
      if (s_pending_reschedule_kind == RESCHEDULE_AT) {
        int h = s_pending_reschedule_at_hour;
        if (clock_is_24h_style()) {
          snprintf(at_msg, sizeof(at_msg), "Scheduling %d:00...", h);
        } else {
          int h12 = h % 12;
          if (h12 == 0) {
            h12 = 12;
          }
          snprintf(at_msg, sizeof(at_msg), "Scheduling %d %s...", h12, h < 12 ? "AM" : "PM");
        }
        pending_msg = at_msg;
      } else
#endif
      {
        pending_msg = s_pending_reschedule_kind == RESCHEDULE_TOMORROW ? "Moving to tomorrow..."
                      : s_pending_reschedule_kind == RESCHEDULE_TODAY ? "Scheduling for today..."
                      : s_pending_reschedule_kind == RESCHEDULE_TO_BACKLOG ? "Moving to backlog..."
                      : s_pending_reschedule_kind == RESCHEDULE_FROM_BACKLOG ? "Moving to list..."
                      : "Un-Scheduling...";
      }
      pend_total_ms = RESCHEDULE_WINDOW_MS;
      pend_tick = s_pending_reschedule_tick;
    }
#ifdef PBL_PLATFORM_EMERY
    else if (s_pending_done_task_id[0] != '\0' &&
             strncmp(s_pending_done_task_id, task->id, MAX_ID_LEN) == 0) {
      pending_msg = "Marking done...";
      pend_total_ms = DONE_WINDOW_MS;
      pend_tick = s_pending_done_tick;
    }
#endif
    if (pending_msg) {
      GColor pend_crisp = is_selected ? GColorWhite : GColorBlack;
      graphics_context_set_text_color(ctx, pend_crisp);
      draw_text(ctx, pending_msg, SUBTITLE_FONT_KEY, subtitle_box, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter);
#ifdef PBL_PLATFORM_EMERY
      int rem_ms = pend_total_ms - pend_tick * SCROLL_INTERVAL_MS;
      if (rem_ms < 0) {
        rem_ms = 0;
      }
      int full_w = subtitle_box.size.w;
      int bar_w = full_w * rem_ms / pend_total_ms;
      // Warms green -> red as the cancel window runs out, so the last stretch
      // before it commits actually looks urgent.
      bool bar_urgent = rem_ms * 100 / pend_total_ms <= 25;
      graphics_context_set_fill_color(ctx, bar_urgent ? GColorRed : GColorIslamicGreen);
      graphics_fill_rect(ctx,
                         GRect(subtitle_box.origin.x + (full_w - bar_w) / 2,
                               subtitle_box.origin.y + subtitle_box.size.h - 3, bar_w, 2),
                         0, GCornerNone);
#else
      (void)pend_total_ms;
      (void)pend_tick;
#endif
      return;
    }
  }
#endif

  if (task->done) {
    draw_text(ctx, "Done", SUBTITLE_FONT_KEY, subtitle_box, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter);
    return;
  }

  char subtitle[56];
  build_task_subtitle(task, subtitle, sizeof(subtitle));

  GRect left_box = subtitle_box;
#ifndef PBL_PLATFORM_APLITE
  // Priority icon: the far-right slot of the subtitle line. Claimed first so the
  // pinned row's right-aligned project name sits to its left, not under it.
  if (task->priority != PRIO_NONE) {
    draw_priority_glyph(ctx, GRect(subtitle_box.origin.x + subtitle_box.size.w - PRIORITY_GLYPH_W,
                                   subtitle_box.origin.y, PRIORITY_GLYPH_W, subtitle_box.size.h),
                        task->priority, fg);
    subtitle_box.size.w -= PRIORITY_GLYPH_W;
    left_box.size.w -= PRIORITY_GLYPH_W;
  }
  // Project name for the pinned row, right-aligned - reserve up to half the
  // width and shrink the left (due/time) box to match.
  if (show_project && task->project[0] != '\0') {
    GFont pfont = fonts_get_system_font(FONT_KEY_GOTHIC_14);
    GSize psize = graphics_text_layout_get_content_size(
        task->project, pfont, subtitle_box, GTextOverflowModeTrailingEllipsis, GTextAlignmentRight);
    int16_t pw = psize.w;
    if (pw > subtitle_box.size.w / 2) {
      pw = subtitle_box.size.w / 2;
    }
    graphics_draw_text(ctx, task->project, pfont,
                        GRect(subtitle_box.origin.x + subtitle_box.size.w - pw, subtitle_box.origin.y + 2,
                              pw, subtitle_box.size.h - 2),
                        GTextOverflowModeTrailingEllipsis, GTextAlignmentRight, NULL);
    left_box.size.w -= (pw + 6);
  }
#else
  (void)show_project;
#endif

  if (task->recurs) {
    draw_recurs_glyph(ctx, left_box, fg);
    left_box.origin.x += RECURS_GLYPH_W;
    left_box.size.w -= RECURS_GLYPH_W;
  }

#ifndef PBL_PLATFORM_APLITE
  if (task->issue_src != ISSUE_SRC_NONE) {
    draw_issue_src_glyph(ctx, left_box, task->issue_src, fg);
    left_box.origin.x += ISSUE_GLYPH_W;
    left_box.size.w -= ISSUE_GLYPH_W;
  }
#endif

  if (subtitle[0] != '\0') {
#ifndef PBL_PLATFORM_APLITE
    GFont sf = fonts_get_system_font(SUBTITLE_FONT_KEY);
    // Marquee the subtitle on the selected row when it overflows - same
    // two-copy scroll the title uses, sharing s_scroll_offset_px. Not on the
    // pinned row (its right-aligned project name already claims half the line).
    bool sub_marquee = is_selected && !show_project &&
                        title_natural_width_font(subtitle, sf) > left_box.size.w;
    draw_marquee_title(ctx, left_box, subtitle, sf, sub_marquee);
#else
    draw_text(ctx, subtitle, SUBTITLE_FONT_KEY, left_box, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
#endif
  }
}

#ifdef PBL_PLATFORM_EMERY
// A calm ~2s throb on the pinned tracking row: a small filled dot at the right
// edge that grows and shrinks so the row reads as "live" at a glance. No timer
// of its own - it rides s_scroll_offset_px, which the marquee scroll timer
// keeps advancing the whole time a local session is pinned (refresh_scroll_state
// forces needs_scroll while has_pinned_row() && tracking).
static void draw_live_pulse(GContext *ctx, GRect row_bounds, bool is_selected) {
  int ph = ((s_scroll_offset_px % 40) + 40) % 40;  // 0..39, ~2s at 100ms/2px
  int up = ph < 20 ? ph : 40 - ph;                 // 0..20..0 triangle
  int r = 2 + up / 5;                              // 2..6..2 px
  GPoint c = GPoint(row_bounds.size.w - 12, row_bounds.size.h / 2);
  graphics_context_set_fill_color(ctx, is_selected ? GColorMintGreen : GColorIslamicGreen);
  graphics_fill_circle(ctx, c, r);
}
#endif

static void menu_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context) {
  if (s_task_count == 0 && !ACTIONABLE_EMPTY_ACTIVE()) {
    // Menu hidden for every empty reason except the actionable one - nothing to
    // draw. In the actionable case section 0 draws below as for a full list.
    return;
  }
#ifdef PBL_PLATFORM_EMERY
  // The scroll pip is a sibling layer, so it isn't repainted when the menu
  // scrolls on its own (a touch drag especially). Any row repaint means the
  // viewport moved - re-mark it so the thumb keeps up with the selection.
  if (s_scroll_pip_layer) {
    layer_mark_dirty(s_scroll_pip_layer);
  }
#endif
  if (cell_index->section == 0) {
    bool is_selected = menu_layer_get_selected_index(s_menu_layer).section == cell_index->section &&
                        menu_layer_get_selected_index(s_menu_layer).row == cell_index->row;
    GRect bounds = layer_get_bounds(cell_layer);
    Section0RowKind kind = section0_row_kind((int)cell_index->row);

    if (kind == SECTION0_ROW_HABITS) {
      // Navigates to the habits (SimpleCounter) page. Icon matches the real
      // app's "heart_check" icon; GCompOpSet lets its transparent background
      // through.
#ifdef PBL_PLATFORM_APLITE
      // aplite has no RAM headroom for the progress layout below - plain row.
      GRect ic = GRect(bounds.size.w - ROW_ICON_SIZE - 10, (bounds.size.h - ROW_ICON_SIZE) / 2,
                       ROW_ICON_SIZE, ROW_ICON_SIZE);
      draw_nav_row(ctx, bounds, is_selected, GColorVividCerulean, "Habits",
                   s_heart_bitmap, s_heart_white_bitmap, ic);
#else
      // Title + a "N left today" / "All done today" progress subtitle, laid out
      // like the Resync row below. The all-done state gets a small check drawn
      // ahead of its subtitle.
      int habits_left = 0;
      for (int i = 0; i < s_habit_count; i++) {
        if (!s_habits[i].done) {
          habits_left++;
        }
      }
      bool all_done = s_habit_count > 0 && habits_left == 0;
      GColor fg = is_selected ? GColorWhite : GColorBlack;
      GColor habits_bg = GColorVividCerulean;
#ifdef PBL_PLATFORM_EMERY
      // Solid gold when a bump just finished the last habit of the day; a couple
      // of cerulean blinks at the tail as it winds down.
      if (s_lasthabit_flash_active) {
        int rem_ms = LASTHABIT_FLASH_MS - s_lasthabit_flash_tick * SCROLL_INTERVAL_MS;
        if (rem_ms > 1200 || (s_lasthabit_flash_tick / 3) % 2 == 0) {
          habits_bg = GColorYellow;
          fg = GColorBlack;
        }
      }
#endif
      fill_bg(ctx, bounds, habits_bg);
      graphics_context_set_text_color(ctx, fg);
      draw_text(ctx, "Habits", HEADING_FONT_KEY,
                GRect(TITLE_BOX_X, ROW_TITLE_TOP_Y(bounds.size.h, HEADING_TITLE_H, CHROME_STRIP_H),
                      bounds.size.w - TITLE_BOX_X * 2 - ROW_ICON_SIZE - 8, HEADING_TITLE_H),
                GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
      if (s_habit_count > 0) {
        int16_t sub_x = TITLE_BOX_X;
        int16_t sub_y = ROW_SUBTITLE_TOP_Y(bounds.size.h, HEADING_TITLE_H, CHROME_STRIP_H);
        char hsub[22];
        if (all_done) {
          graphics_context_set_stroke_color(ctx, fg);
          graphics_draw_line(ctx, GPoint(sub_x, sub_y + 8), GPoint(sub_x + 3, sub_y + 11));
          graphics_draw_line(ctx, GPoint(sub_x + 3, sub_y + 11), GPoint(sub_x + 9, sub_y + 4));
          sub_x += 13;
          str_copy(hsub, "All done today", sizeof(hsub));
        } else {
          snprintf(hsub, sizeof(hsub), "%d left today", habits_left);
        }
        draw_text(ctx, hsub, CHROME_FONT_KEY,
                  GRect(sub_x, sub_y, bounds.size.w - sub_x - TITLE_BOX_X, CHROME_STRIP_H),
                  GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
      }
      GRect ic = GRect(bounds.size.w - ROW_ICON_SIZE - 10,
                       s_habit_count > 0
                         ? ROW_TITLE_TOP_Y(bounds.size.h, HEADING_TITLE_H, CHROME_STRIP_H) + 2
                         : (bounds.size.h - ROW_ICON_SIZE) / 2,
                       ROW_ICON_SIZE, ROW_ICON_SIZE);
      graphics_context_set_compositing_mode(ctx, GCompOpSet);
      graphics_draw_bitmap_in_rect(ctx, is_selected ? s_heart_white_bitmap : s_heart_bitmap, ic);
#endif
      return;
    }

#if PROJECTS_BROWSER
    if (kind == SECTION0_ROW_PROJECTS) {
      // Navigates to the projects browser. Purple - its own colour among the
      // section-0 nav rows. White folder icon on the right.
      GRect ic = GRect(bounds.size.w - ROW_ICON_SIZE - 8, (bounds.size.h - 20) / 2, 20, 20);
      draw_nav_row(ctx, bounds, is_selected, GColorPurple, "Projects",
                   s_project_bitmap, s_project_white_bitmap, ic);
      return;
    }

    if (kind == SECTION0_ROW_TAGS) {
      // Opens the Tags page (Projects browser reused in BROWSE_TAGS mode). Mint
      // green - distinct from every other section-0 row (the old Limerick read
      // as the same yellow as the Schedule row). Darkens to JaegerGreen with
      // white text on select. A "#" hash glyph on the right, from primitives.
      GColor icon = is_selected ? GColorWhite : GColorBlack;
      fill_bg(ctx, bounds, is_selected ? GColorJaegerGreen : GColorMintGreen);
      graphics_context_set_text_color(ctx, icon);
      GRect tags_title_box = GRect(TITLE_BOX_X, HEADING_TITLE_Y(bounds.size.h),
                                    bounds.size.w - TITLE_BOX_X * 2 - ROW_ICON_SIZE - 8, HEADING_TITLE_H);
      draw_text(ctx, "Tags", HEADING_FONT_KEY, tags_title_box, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
      int16_t hx = bounds.size.w - ROW_ICON_SIZE - 4;
      int16_t hy = bounds.size.h / 2;
      graphics_context_set_stroke_color(ctx, icon);
      graphics_draw_line(ctx, GPoint(hx + 2, hy - 6), GPoint(hx, hy + 6));       // "#" left bar
      graphics_draw_line(ctx, GPoint(hx + 8, hy - 6), GPoint(hx + 6, hy + 6));   // "#" right bar
      graphics_draw_line(ctx, GPoint(hx - 2, hy - 2), GPoint(hx + 10, hy - 2));  // "#" top bar
      graphics_draw_line(ctx, GPoint(hx - 3, hy + 3), GPoint(hx + 9, hy + 3));   // "#" bottom bar
      return;
    }
#endif

#ifndef PBL_PLATFORM_APLITE
    if (kind == SECTION0_ROW_STATS) {
      // Opens the read-only Stats page. Orange, bar-chart icon.
      GRect ic = GRect(bounds.size.w - ROW_ICON_SIZE - 8, (bounds.size.h - 20) / 2, 20, 20);
      draw_nav_row(ctx, bounds, is_selected, GColorOrange, "Stats",
                   s_stats_bitmap, s_stats_white_bitmap, ic);
      return;
    }
#endif

#ifndef PBL_PLATFORM_APLITE
    if (kind == SECTION0_ROW_SCHEDULE) {
      // Opens the Schedule page - today's timed tasks in time order. Yellow: the
      // old teal read as the same blue as the Habits row (cerulean) on-watch, so
      // it's now a hue nothing else in section 0 uses (Resync red, Habits
      // cerulean, Projects purple, Stats orange, Upcoming indigo, Add Task
      // green). A clock glyph on the right, drawn from primitives (no bitmap
      // asset), black normally / white when selected like the others.
      GColor icon = is_selected ? GColorWhite : GColorBlack;
      fill_bg(ctx, bounds, GColorYellow);
      graphics_context_set_text_color(ctx, icon);
      GRect sched_title_box = GRect(TITLE_BOX_X, HEADING_TITLE_Y(bounds.size.h),
                                     bounds.size.w - TITLE_BOX_X * 2 - ROW_ICON_SIZE - 8, HEADING_TITLE_H);
      draw_text(ctx, "Schedule", HEADING_FONT_KEY, sched_title_box, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
      GPoint clock_c = GPoint(bounds.size.w - ROW_ICON_SIZE - 8 + 10, bounds.size.h / 2);
      graphics_context_set_stroke_color(ctx, icon);
      graphics_draw_circle(ctx, clock_c, 8);
      graphics_draw_line(ctx, clock_c, GPoint(clock_c.x, clock_c.y - 5));     // minute hand
      graphics_draw_line(ctx, clock_c, GPoint(clock_c.x + 4, clock_c.y + 2)); // hour hand
      return;
    }
#endif

#ifdef PBL_PLATFORM_EMERY
    if (kind == SECTION0_ROW_CALENDAR) {
      // Opens the Calendar month view (browse-only). Blue Moon, its own colour.
      // A hand-drawn page-a-day glyph: an outlined square with a header line
      // and two ring ticks, on the right.
      GColor icon = is_selected ? GColorWhite : GColorBlack;
      fill_bg(ctx, bounds, GColorBlueMoon);
      graphics_context_set_text_color(ctx, icon);
      GRect cal_title_box = GRect(TITLE_BOX_X, HEADING_TITLE_Y(bounds.size.h),
                                   bounds.size.w - TITLE_BOX_X * 2 - ROW_ICON_SIZE - 8, HEADING_TITLE_H);
      draw_text(ctx, "Calendar", HEADING_FONT_KEY, cal_title_box, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
      GRect cal_ic = GRect(bounds.size.w - ROW_ICON_SIZE - 6, bounds.size.h / 2 - 7, 15, 14);
      graphics_context_set_stroke_color(ctx, icon);
      graphics_draw_rect(ctx, cal_ic);
      graphics_draw_line(ctx, GPoint(cal_ic.origin.x, cal_ic.origin.y + 4),
                         GPoint(cal_ic.origin.x + cal_ic.size.w - 1, cal_ic.origin.y + 4));
      graphics_draw_line(ctx, GPoint(cal_ic.origin.x + 4, cal_ic.origin.y - 2),
                         GPoint(cal_ic.origin.x + 4, cal_ic.origin.y + 1));
      graphics_draw_line(ctx, GPoint(cal_ic.origin.x + 11, cal_ic.origin.y - 2),
                         GPoint(cal_ic.origin.x + 11, cal_ic.origin.y + 1));
      return;
    }
#endif

#ifndef PBL_PLATFORM_APLITE
    if (kind == SECTION0_ROW_UPCOMING) {
      // Opens the Upcoming page - future-dated tasks by day. Indigo, its own
      // colour among the nav rows. A ">>" glyph on the right, from primitives.
      GColor icon = is_selected ? GColorWhite : GColorBlack;
      fill_bg(ctx, bounds, GColorIndigo);
      graphics_context_set_text_color(ctx, icon);
      GRect up_title_box = GRect(TITLE_BOX_X, HEADING_TITLE_Y(bounds.size.h),
                                  bounds.size.w - TITLE_BOX_X * 2 - ROW_ICON_SIZE - 8, HEADING_TITLE_H);
      draw_text(ctx, "Upcoming", HEADING_FONT_KEY, up_title_box, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
      int16_t cx = bounds.size.w - ROW_ICON_SIZE - 4;
      int16_t cy = bounds.size.h / 2;
      graphics_context_set_stroke_color(ctx, icon);
      for (int k = 0; k < 2; k++) {
        int16_t x = cx + k * 6;
        graphics_draw_line(ctx, GPoint(x, cy - 5), GPoint(x + 5, cy));
        graphics_draw_line(ctx, GPoint(x + 5, cy), GPoint(x, cy + 5));
      }
      return;
    }
#endif

#ifndef PBL_PLATFORM_APLITE
    if (kind == SECTION0_ROW_NOTESPAGE) {
      // Opens the Notes page - today-pinned standalone notes. Light grey,
      // inverting to dark grey + white on select like the other nav rows. A
      // lined-note glyph on the right.
      GColor fg = is_selected ? GColorWhite : GColorBlack;
      fill_bg(ctx, bounds, is_selected ? GColorDarkGray : GColorLightGray);
      graphics_context_set_text_color(ctx, fg);
      GRect np_title_box = GRect(TITLE_BOX_X, HEADING_TITLE_Y(bounds.size.h),
                                  bounds.size.w - TITLE_BOX_X * 2 - ROW_ICON_SIZE - 8, HEADING_TITLE_H);
      draw_text(ctx, "Notes", HEADING_FONT_KEY, np_title_box, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
      int16_t gx = bounds.size.w - ROW_ICON_SIZE - 4;
      int16_t gy = bounds.size.h / 2;
      graphics_context_set_stroke_color(ctx, fg);
      graphics_draw_rect(ctx, GRect(gx, gy - 7, 13, 15));
      for (int k = 0; k < 3; k++) {
        graphics_draw_line(ctx, GPoint(gx + 3, gy - 3 + k * 3), GPoint(gx + 10, gy - 3 + k * 3));
      }
      return;
    }
#endif

#ifndef PBL_PLATFORM_APLITE
    if (kind == SECTION0_ROW_SEARCH) {
      // Dictate a query; menu_select_click starts dictation. Cadet blue, its
      // own colour, with a hand-drawn magnifier glyph on the right.
      GColor fg = is_selected ? GColorWhite : GColorBlack;
      fill_bg(ctx, bounds, is_selected ? GColorOxfordBlue : GColorCadetBlue);
      graphics_context_set_text_color(ctx, fg);
      GRect se_title_box = GRect(TITLE_BOX_X, HEADING_TITLE_Y(bounds.size.h),
                                  bounds.size.w - TITLE_BOX_X * 2 - ROW_ICON_SIZE - 8, HEADING_TITLE_H);
      draw_text(ctx, "Search", HEADING_FONT_KEY, se_title_box, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
      int16_t gx = bounds.size.w - ROW_ICON_SIZE - 2;
      int16_t gy = bounds.size.h / 2 - 3;
      graphics_context_set_stroke_color(ctx, fg);
      graphics_context_set_stroke_width(ctx, 2);
      graphics_draw_circle(ctx, GPoint(gx, gy), 6);
      graphics_draw_line(ctx, GPoint(gx + 5, gy + 5), GPoint(gx + 10, gy + 10));
      graphics_context_set_stroke_width(ctx, 1);
      return;
    }
#endif

#ifndef PBL_PLATFORM_APLITE
    if (kind == SECTION0_ROW_ADD_TASK) {
      // Mic platforms with the feature enabled only. Starts dictation via
      // menu_select_click.
      GRect ic = GRect(bounds.size.w - ROW_ICON_SIZE - 10, (bounds.size.h - ROW_ICON_SIZE) / 2,
                       ROW_ICON_SIZE, ROW_ICON_SIZE);
#ifdef PBL_PLATFORM_EMERY
      if (s_addtask_flash_active) {
        draw_nav_row(ctx, bounds, is_selected, GColorGreen, "Added",
                     s_mic_bitmap, s_mic_white_bitmap, ic);
        return;
      }
#endif
      draw_nav_row(ctx, bounds, is_selected, GColorJaegerGreen, "Add Task",
                   s_mic_bitmap, s_mic_white_bitmap, ic);
      return;
    }
#endif

    // Reflects live sync status so a resync failure is visible while the
    // cached list still shows, instead of being silently swallowed.
    static char s_resync_subtitle[MAX_STATUS_MSG_LEN + 16];
    const char *subtitle = "Synced";
    switch (s_status_code) {
      case STATUS_SYNCING:
        // s_status_msg carries the phone's "Decrypting N%" during the slow
        // E2EE replay (index.js's sendStatus); empty once past it.
        subtitle = s_status_msg[0] != '\0' ? s_status_msg : "Syncing...";
        break;
#ifndef PBL_PLATFORM_APLITE
      case STATUS_OK: {
        // "Synced" while fresh, then how long ago so a stale list is obvious.
        int age = s_last_sync_epoch ? (int)(time(NULL) - s_last_sync_epoch) : -1;
        if (age >= 60 && age < 3600) {
          snprintf(s_resync_subtitle, sizeof(s_resync_subtitle), "Synced %dm ago", age / 60);
          subtitle = s_resync_subtitle;
        } else if (age >= 3600 && age < 86400) {
          snprintf(s_resync_subtitle, sizeof(s_resync_subtitle), "Synced %dh ago", age / 3600);
          subtitle = s_resync_subtitle;
        } else if (age >= 86400) {
          subtitle = "Synced over a day ago";
        }
        break;
      }
#endif
      case STATUS_ERROR:
        if (s_status_msg[0] != '\0') {
          snprintf(s_resync_subtitle, sizeof(s_resync_subtitle), "Failed: %s", s_status_msg);
          subtitle = s_resync_subtitle;
        } else {
          subtitle = "Sync failed";
        }
        break;
      case STATUS_NOT_PAIRED:
        subtitle = "Not paired - open phone app";
        break;
      default:
        break;
    }
    // Background stays red regardless of selection so this row reads as a
    // standing call-to-action, not a task; the text still inverts on select.
    // On emery it holds green for SYNC_GREEN_MS right after a clean sync.
    GColor resync_bg = GColorRed;
#ifdef PBL_PLATFORM_EMERY
    if (s_sync_check_active) {
      resync_bg = GColorIslamicGreen;
    }
#endif
    fill_bg(ctx, bounds, resync_bg);
    graphics_context_set_text_color(ctx, is_selected ? GColorWhite : GColorBlack);
    GRect title_box = GRect(TITLE_BOX_X, ROW_TITLE_TOP_Y(bounds.size.h, HEADING_TITLE_H, CHROME_STRIP_H),
                             bounds.size.w - TITLE_BOX_X * 2 - ROW_ICON_SIZE - 8, HEADING_TITLE_H);
    draw_text(ctx, "Resync", HEADING_FONT_KEY, title_box, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
    // Full row width - not truncating a "Failed: ..." status matters more than
    // dodging the icon, which sits up in the title band.
    GRect subtitle_box = GRect(TITLE_BOX_X, ROW_SUBTITLE_TOP_Y(bounds.size.h, HEADING_TITLE_H, CHROME_STRIP_H),
                                bounds.size.w - TITLE_BOX_X * 2, CHROME_STRIP_H);
    draw_text(ctx, subtitle, CHROME_FONT_KEY, subtitle_box, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
    // The sp checkmark logo, matching the Habits row's icon.
    GRect icon_rect = GRect(bounds.size.w - ROW_ICON_SIZE - 10,
                             ROW_TITLE_TOP_Y(bounds.size.h, HEADING_TITLE_H, CHROME_STRIP_H) + 2,
                             ROW_ICON_SIZE, ROW_ICON_SIZE);
    graphics_context_set_compositing_mode(ctx, GCompOpSet);
    graphics_draw_bitmap_in_rect(ctx, is_selected ? s_check_white_bitmap : s_check_bitmap, icon_rect);
    return;
  }
  if (is_finish_day_cell(cell_index)) {
#ifndef PBL_PLATFORM_APLITE
    // Finish Day row, always last - long-select archives every done task, plain
    // Select is a no-op. Inverts on selection like a task row. aplite-excluded -
    // the plain version-only footer stays there instead.
    bool is_selected = menu_layer_get_selected_index(s_menu_layer).section == cell_index->section &&
                        menu_layer_get_selected_index(s_menu_layer).row == cell_index->row;
    GRect bounds = layer_get_bounds(cell_layer);
    GColor bg = is_selected ? GColorBlack : GColorWhite;
    GColor fg = is_selected ? GColorWhite : GColorBlack;
    fill_bg(ctx, bounds, bg);
    graphics_context_set_text_color(ctx, fg);
    GRect title_box = GRect(TITLE_BOX_X, ROW_TITLE_TOP_Y(bounds.size.h, HEADING_TITLE_H, CHROME_STRIP_H),
                             bounds.size.w - TITLE_BOX_X * 2, HEADING_TITLE_H);
    draw_text(ctx, "Finish Day", HEADING_FONT_KEY, title_box, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter);
    // Version text as this row's subtitle - the same footer slot it's lived in
    // since v0.6.5, just no longer alone.
    GRect subtitle_box = GRect(TITLE_BOX_X, ROW_SUBTITLE_TOP_Y(bounds.size.h, HEADING_TITLE_H, CHROME_STRIP_H),
                                bounds.size.w - TITLE_BOX_X * 2, CHROME_STRIP_H);
    draw_text(ctx, "v" APP_VERSION, CHROME_FONT_KEY, subtitle_box, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter);
#else
    // Plain version-only footer - no tap/long-select action on aplite.
    GRect bounds = layer_get_bounds(cell_layer);
    fill_bg(ctx, bounds, GColorWhite);
    graphics_context_set_text_color(ctx, GColorBlack);
    draw_text(ctx, "v" APP_VERSION, FONT_KEY_GOTHIC_14, bounds, GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter);
#endif
    return;
  }
#ifndef PBL_PLATFORM_APLITE
  // The project row - the old plain header's green/divider look, now a
  // selectable row (double-click Select shows the project's notes). Text, not
  // the green fill, inverts on selection.
  TaskGroup *project_row = resolve_project_row_at(*cell_index);
  if (project_row) {
    bool is_selected = menu_layer_get_selected_index(s_menu_layer).section == cell_index->section &&
                        menu_layer_get_selected_index(s_menu_layer).row == cell_index->row;
    GRect bounds = layer_get_bounds(cell_layer);
    GFont bold_font = fonts_get_system_font(HEADING_FONT_KEY);
    int16_t text_top = HEADING_TITLE_Y(bounds.size.h);
    GColor fg = is_selected ? GColorWhite : GColorBlack;
    // Empty project_id is the shared "not a real project" signal - the
    // synthetic Later Today group has always forced this (recompute_groups),
    // and a phone-side group-by-tag/deadline/planned-date group now does the
    // same, so one check here covers all of them.
    bool group_is_real_project = project_row->project_id[0] != '\0';

    // Wipes in left->right on emery the first sync after open, like every
    // other header_bar_w bar (see header_begin_reveal) - a no-op stub off
    // emery, so this is a plain fill everywhere else.
    int16_t bar_w = header_bar_w(bounds.size.w);
    // Darker bar = this group's tasks are hidden (Select toggles) - reuses
    // the existing fill instead of drawing a separate glyph, which is far
    // cheaper on emery's near-zero .text headroom (see CLAUDE.md).
    fill_bg(ctx, GRect(bounds.origin.x, bounds.origin.y, bar_w, bounds.size.h),
            (s_collapse_groups && project_row->user_collapsed) ? GColorIslamicGreen : GColorGreen);
    if (bar_w < bounds.size.w) {
      return;
    }
#if TODAY_PROJECT_SWATCH
    // A non-project group (Later Today, or a tag/deadline/planned-date
    // group) has nothing real to swatch, same as "No Project" - color 0.
    int16_t text_x = draw_project_marker(ctx, TITLE_BOX_X, bounds.size.h,
                                          project_row->project_id,
                                          group_is_real_project
                                            ? s_tasks[s_row_map[project_row->start]].project_color : 0,
                                          is_selected);
#else
    int16_t text_x = TITLE_BOX_X;
#endif
    GRect text_rect = GRect(text_x, text_top, bounds.size.w - text_x - TITLE_BOX_X, bounds.size.h - 4);
    graphics_context_set_text_color(ctx, fg);
    graphics_draw_text(ctx, project_row->name, bold_font, text_rect,
                        GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);

    // Full-width divider separating this project from its tasks.
    int16_t divider_y = bounds.size.h - 2;
    graphics_context_set_stroke_color(ctx, fg);
    graphics_draw_line(ctx, GPoint(0, divider_y), GPoint(bounds.size.w, divider_y));
    return;
  }
#endif
#ifndef PBL_PLATFORM_APLITE
  // A remote presence session in the pinned "TRACKING" section - drawn as a
  // task row would be (white cell under the green strip), but from s_presence_*
  // since the watch may not hold that task at all.
  if (remote_in_pinned_section() && cell_index->section == 1) {
    GRect bounds = layer_get_bounds(cell_layer);
    bool is_selected = menu_layer_get_selected_index(s_menu_layer).section == 1 &&
                        menu_layer_get_selected_index(s_menu_layer).row == 0;
    GColor bg = is_selected ? GColorBlack : GColorWhite;
    GColor fg = is_selected ? GColorWhite : GColorBlack;
    fill_bg(ctx, bounds, bg);
    graphics_context_set_text_color(ctx, fg);
    GRect title_box = GRect(TITLE_BOX_X, ROW_TITLE_TOP_Y(bounds.size.h, HEADING_TITLE_H, CHROME_STRIP_H),
                             bounds.size.w - TITLE_BOX_X * 2, HEADING_TITLE_H);
    GFont pinned_font = fonts_get_system_font(TITLE_FONT_KEY);
    const char *pinned_title = s_presence_task[0] != '\0' ? s_presence_task : "Live tracking";
    draw_marquee_title(ctx, title_box, pinned_title, pinned_font,
                        is_selected && s_presence_task[0] != '\0' &&
                        title_natural_width_font(pinned_title, pinned_font) > title_box.size.w);
    char sub[52];
    if (s_presence_state == 1) {
      int total_s = (int)(time(NULL) - s_presence_elapsed_base);
      if (total_s < 0) {
        total_s = 0;
      }
      int h = total_s / 3600;
      int m = (total_s % 3600) / 60;
      if (h > 0) {
        snprintf(sub, sizeof(sub), "%s  %d:%02d", presence_state_phrase(), h, m);
      } else {
        snprintf(sub, sizeof(sub), "%s  %dm", presence_state_phrase(), m);
      }
    } else {
      snprintf(sub, sizeof(sub), "%s", presence_state_phrase());
    }
    GRect subtitle_box = GRect(TITLE_BOX_X, ROW_SUBTITLE_TOP_Y(bounds.size.h, HEADING_TITLE_H, CHROME_STRIP_H),
                                bounds.size.w - TITLE_BOX_X * 2, CHROME_STRIP_H);
    draw_text(ctx, sub, SUBTITLE_FONT_KEY, subtitle_box, GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
    return;
  }
#endif

  Task *task = resolve_task_at(*cell_index);
  if (!task) {
    return;
  }

  bool is_selected = menu_layer_get_selected_index(s_menu_layer).section == cell_index->section &&
                      menu_layer_get_selected_index(s_menu_layer).row == cell_index->row;
  bool is_pinned_row = false;
#ifndef PBL_PLATFORM_APLITE
  is_pinned_row = has_pinned_row() && cell_index->section == 1;
#endif
  draw_task_row(ctx, layer_get_bounds(cell_layer), task, is_selected, is_pinned_row);
#ifdef PBL_PLATFORM_EMERY
  if (is_pinned_row && s_tracking_task_id[0] != '\0' && !task->done) {
    draw_live_pulse(ctx, layer_get_bounds(cell_layer), is_selected);
  }
#endif
}
