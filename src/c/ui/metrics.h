// Platform-scaled UI metrics and small drawing helpers shared by every screen.
// Textually #included by main.c (one translation unit) - not compiled alone.

// --- Platform-scaled UI metrics ---------------------------------------------
// emery renders third-party apps at the SDK's "Large" content size, so its
// MenuLayer cells are ~61px vs 44px elsewhere. Everything that positions text
// in a cell needs an emery variant or the 44px offsets leave the title jammed
// up top and the subtitle stranded at the bottom. Non-emery values are
// unchanged from before this block.
//
// TITLE/SUBTITLE_FONT_KEY: a row's title and its due/time/"Done" subtitle.
// HEADING_FONT_KEY: the bold Resync/Habits/Add Task/Finish Day/project rows.
// TRACKING_FONT_KEY: the "TRACKING" pinned-section header strip.
// CHROME_FONT_KEY: secondary text (Resync status, version subtitle, "No tasks").
// *_STRIP_H: height reserved at the bottom of a cell for its subtitle.
// HEADING_TITLE_H: title-box height for the taller HEADING_FONT_KEY rows.
#ifdef PBL_PLATFORM_EMERY
#define SCROLL_INTERVAL_MS 100
#define SCROLL_STEP_PX 2
#define TITLE_FONT_KEY FONT_KEY_GOTHIC_24_BOLD
#define SUBTITLE_FONT_KEY FONT_KEY_GOTHIC_18
#define HEADING_FONT_KEY FONT_KEY_GOTHIC_28_BOLD
#define TRACKING_FONT_KEY FONT_KEY_GOTHIC_24_BOLD
#define CHROME_FONT_KEY FONT_KEY_GOTHIC_18
#define CHROME_FONT_BOLD_KEY FONT_KEY_GOTHIC_18_BOLD
#define EMPTY_MSG_FONT_KEY FONT_KEY_GOTHIC_24
#define TITLE_BOX_X 10
#define TITLE_BOX_Y 3
#define SUBTITLE_STRIP_H 26
#define CHROME_STRIP_H 22
#define HEADING_TITLE_H 34
#define GROUP_HEADER_HEIGHT 48
#else
#define SCROLL_INTERVAL_MS 300
#define SCROLL_STEP_PX 6
#define TITLE_FONT_KEY FONT_KEY_GOTHIC_18_BOLD
#define SUBTITLE_FONT_KEY FONT_KEY_GOTHIC_18
#define HEADING_FONT_KEY FONT_KEY_GOTHIC_24_BOLD
#define TRACKING_FONT_KEY FONT_KEY_GOTHIC_18_BOLD
#define CHROME_FONT_KEY FONT_KEY_GOTHIC_14
#define CHROME_FONT_BOLD_KEY FONT_KEY_GOTHIC_14_BOLD
#define EMPTY_MSG_FONT_KEY FONT_KEY_GOTHIC_18
#define TITLE_BOX_X 6
#define TITLE_BOX_Y 2
#define SUBTITLE_STRIP_H 22
#define CHROME_STRIP_H 18
#define HEADING_TITLE_H 30
#define GROUP_HEADER_HEIGHT 40
#endif

// Lays out a title line (height title_h) + a bottom subtitle strip (strip_h)
// in a cell_h-tall cell. Pure macros, no locals - aplite has no stack headroom
// for extra int16 slots in the draw paths, and non-emery expands to the exact
// fixed offsets it replaced. Non-emery: title at TITLE_BOX_Y, subtitle flush
// bottom. Emery: title+subtitle centred as one block in the taller cell.
// HEADING_TITLE_Y is the single-line form for the bold nav rows.
#ifdef PBL_PLATFORM_EMERY
#define ROW_TITLE_TOP_Y(cell_h, title_h, strip_h) \
  (((cell_h) - (title_h) - (strip_h)) / 2 < TITLE_BOX_Y \
   ? TITLE_BOX_Y : ((cell_h) - (title_h) - (strip_h)) / 2)
#define ROW_SUBTITLE_TOP_Y(cell_h, title_h, strip_h) \
  (ROW_TITLE_TOP_Y(cell_h, title_h, strip_h) + (title_h))
#define HEADING_TITLE_Y(cell_h) (((cell_h) - HEADING_TITLE_H) / 2)
#else
#define ROW_TITLE_TOP_Y(cell_h, title_h, strip_h) (TITLE_BOX_Y)
#define ROW_SUBTITLE_TOP_Y(cell_h, title_h, strip_h) ((cell_h) - (strip_h))
#define HEADING_TITLE_Y(cell_h) (TITLE_BOX_Y)
#endif

static AppTimer *s_scroll_timer = NULL;
static int s_scroll_offset_px = 0;

// Cycles "Syncing" -> "Syncing..." on the empty screen during the first sync
// (no cached list yet) so it doesn't sit on static text through the op-log replay.
#define SYNCING_ANIM_INTERVAL_MS 400
static AppTimer *s_syncing_timer = NULL;
static int s_syncing_dots = 0;

// strncpy that always NUL-terminates - the copy-into-fixed-buffer pattern used
// throughout. `cap` is the full buffer size (a sizeof or a MAX_* constant).
static void str_copy(char *dst, const char *src, size_t cap) {
  strncpy(dst, src, cap - 1);
  dst[cap - 1] = '\0';
}

// graphics_draw_text with the system-font lookup and the always-NULL text
// attributes arg folded in - the shape nearly every draw_* proc uses.
static void draw_text(GContext *ctx, const char *text, const char *font_key,
                      GRect box, GTextOverflowMode overflow, GTextAlignment align) {
  graphics_draw_text(ctx, text, fonts_get_system_font(font_key), box, overflow, align, NULL);
}

// Flat colour fill of `r` - the set-fill-colour + square-cornered fill_rect
// pair every row/header draw proc opens with.
static void fill_bg(GContext *ctx, GRect r, GColor color) {
  graphics_context_set_fill_color(ctx, color);
  graphics_fill_rect(ctx, r, 0, GCornerNone);
}

// The create + add-to-parent pair every window_load repeats.
static StatusBarLayer *add_status_bar(Layer *parent) {
  StatusBarLayer *sb = status_bar_layer_create();
  layer_add_child(parent, status_bar_layer_get_layer(sb));
  return sb;
}

// A text layer with the system font, an alignment, and parented - the common
// case in the sub-window loads. Callers add any extra text_layer_set_* after.
static TextLayer *make_text_layer(Layer *parent, GRect frame, const char *font_key,
                                  GTextAlignment align) {
  TextLayer *tl = text_layer_create(frame);
  text_layer_set_font(tl, fonts_get_system_font(font_key));
  text_layer_set_text_alignment(tl, align);
  layer_add_child(parent, text_layer_get_layer(tl));
  return tl;
}

// One section-0 nav row: a solid colour fill, a left title in the heading font
// (inverting on select), and a right-hand bitmap icon (its white variant on
// select). Shared by Habits / Projects / Stats / Add Task; `icon_rect` keeps
// each row's own icon geometry.
static void draw_nav_row(GContext *ctx, GRect bounds, bool is_selected, GColor bg,
                         const char *label, GBitmap *icon, GBitmap *icon_white, GRect icon_rect) {
  fill_bg(ctx, bounds, bg);
  graphics_context_set_text_color(ctx, is_selected ? GColorWhite : GColorBlack);
  draw_text(ctx, label, HEADING_FONT_KEY,
            GRect(TITLE_BOX_X, HEADING_TITLE_Y(bounds.size.h),
                  bounds.size.w - TITLE_BOX_X * 2 - ROW_ICON_SIZE - 8, HEADING_TITLE_H),
            GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(ctx, is_selected ? icon_white : icon, icon_rect);
}

// Read an optional AppMessage tuple, `fb` when the key is absent. The task /
// habit / project / stats / presence payloads are full of "present it if sent,
// else a default" fields.
static int32_t tuple_int(DictionaryIterator *it, uint32_t key, int32_t fb) {
  Tuple *t = dict_find(it, key);
  return t ? t->value->int32 : fb;
}
static const char *tuple_str(DictionaryIterator *it, uint32_t key, const char *fb) {
  Tuple *t = dict_find(it, key);
  return t ? t->value->cstring : fb;
}

#ifndef PBL_PLATFORM_APLITE
static int current_minute_of_day(void) {
  time_t now = time(NULL);
  struct tm *lt = localtime(&now);
  return lt->tm_hour * 60 + lt->tm_min;
}

// Fills s_row_map for this regroup pass and returns where the trailing
// "Later Today" block starts (s_task_count when there isn't one). Identity
// mapping, no later block, unless Later Today is on AND the list is actually
// grouped (some task has a non-empty project - '' means grouping is off
// entirely, see the TaskGroup comment). s_tasks itself is never reordered:
// every "later" task (has a scheduled time strictly after right now) is
// logically moved to the end, sorted by that time, while every other task
// keeps its original (phone-sorted, project-contiguous) relative order -
// a stable partition, recomputed fresh from s_tasks every call, so a task
// promoted out of Later Today lands back in its correct spot within its
// project's run rather than just tacked onto wherever "normal" currently ends.
static int rebuild_row_map(void) {
  bool grouped = s_task_count > 0 && s_tasks[0].project[0] != '\0';
  if (!s_later_today_enabled || !grouped) {
    for (int i = 0; i < s_task_count; i++) {
      s_row_map[i] = i;
    }
    return s_task_count;
  }
  int now_min = current_minute_of_day();
  int later_start = s_task_count;
  int normal_n = 0;
  for (int i = 0; i < s_task_count; i++) {
    if (s_tasks[i].due_min >= 0 && s_tasks[i].due_min > now_min) {
      later_start--;
      s_row_map[later_start] = i; // fill the later block from the back for now
    } else {
      s_row_map[normal_n++] = i;
    }
  }
  // The later block above landed back-to-front (last later task found ends up
  // first) - reverse it in place, then sort by due_min ascending. Both over a
  // handful of entries at most (MAX_TASKS), insertion sort is plenty.
  for (int a = later_start, b = s_task_count - 1; a < b; a++, b--) {
    int t = s_row_map[a]; s_row_map[a] = s_row_map[b]; s_row_map[b] = t;
  }
  for (int a = later_start + 1; a < s_task_count; a++) {
    int v = s_row_map[a];
    int b = a - 1;
    while (b >= later_start && s_tasks[s_row_map[b]].due_min > s_tasks[v].due_min) {
      s_row_map[b + 1] = s_row_map[b];
      b--;
    }
    s_row_map[b + 1] = v;
  }
  return later_start;
}
#endif

static void recompute_groups(void) {
  s_group_count = 0;
#ifndef PBL_PLATFORM_APLITE
  int later_start = rebuild_row_map();
#else
  int later_start = s_task_count;
#endif
  int i = 0;
  while (i < later_start && s_group_count < MAX_GROUPS) {
    int j = i + 1;
#ifndef PBL_PLATFORM_APLITE
    while (j < later_start && strncmp(s_tasks[s_row_map[j]].project, s_tasks[s_row_map[i]].project, MAX_PROJECT_LEN) == 0) {
      j++;
    }
#else
    while (j < later_start && strncmp(s_tasks[j].project, s_tasks[i].project, MAX_PROJECT_LEN) == 0) {
      j++;
    }
#endif
    // Last slot absorbs every remaining task instead of dropping the tail
    // (including any Later Today block - a today list with 13+ distinct
    // projects AND tasks due later is a vanishingly rare overlap).
    if (s_group_count == MAX_GROUPS - 1) {
      j = later_start;
    }
#ifndef PBL_PLATFORM_APLITE
    str_copy(s_groups[s_group_count].name, s_tasks[s_row_map[i]].project, MAX_PROJECT_LEN);
    str_copy(s_groups[s_group_count].project_id, s_tasks[s_row_map[i]].project_id, MAX_PROJECT_ID_LEN);
    s_groups[s_group_count].is_later_today = false;
#else
    str_copy(s_groups[s_group_count].name, s_tasks[i].project, MAX_PROJECT_LEN);
#endif
    s_groups[s_group_count].start = i;
    s_groups[s_group_count].count = j - i;
    s_group_count++;
    i = j;
  }
#ifndef PBL_PLATFORM_APLITE
  // The Later Today block, if any: one group, not run through the strncmp
  // scan above (its members span whatever projects they came from), and
  // never a real project - no colour swatch, no per-project notes.
  if (later_start < s_task_count && s_group_count < MAX_GROUPS) {
    str_copy(s_groups[s_group_count].name, "Later Today", MAX_PROJECT_LEN);
    s_groups[s_group_count].project_id[0] = '\0';
    s_groups[s_group_count].is_later_today = true;
    s_groups[s_group_count].start = later_start;
    s_groups[s_group_count].count = s_task_count - later_start;
    s_group_count++;
  }
#endif
}
