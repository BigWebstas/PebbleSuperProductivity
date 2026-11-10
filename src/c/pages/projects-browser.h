// Projects / Tags browser (project list -> that project's tasks).
// Textually #included by main.c (one translation unit) - not compiled alone.

#if PROJECTS_BROWSER
// ================= Projects browser =================
// One window, one MenuLayer. s_browse_level: 0 = the project list, 1 = one
// project's tasks. Back at level 1 returns to the list; at level 0 it pops
// the window. Data lives in s_browse_*; the project list is persisted, task
// lists are always fetched.

static BrowseProject *resolve_browse_project_at(MenuIndex index) {
  if (!s_browse_projects || index.section != 0 || (int)index.row >= s_browse_project_count) {
    return NULL;
  }
  return &s_browse_projects[index.row];
}

#if PROJECTS_CACHE
// Persist the project list (chunked, like save_tasks - MAX_BROWSE_PROJECTS *
// sizeof(BrowseProject) overflows a single 256 B persist key) so it renders
// immediately on the next open and stays viewable with the phone away.
static void save_browse_projects(void) {
  if (!s_browse_projects) {
    return;
  }
  persist_delete(PERSIST_KEY_BROWSE_PROJECTS); // legacy single-blob key (pre-chunk builds)
  save_blob_cache(PERSIST_KEY_BROWSE_PROJECTS + 1, 280, (uint8_t *)s_browse_projects,
                  s_browse_project_count, sizeof(BrowseProject),
                  (size_t)MAX_BROWSE_PROJECTS * sizeof(BrowseProject));
}

static void load_browse_projects(void) {
  if (!s_browse_projects) {
    return;
  }
  s_browse_project_count = load_blob_cache(PERSIST_KEY_BROWSE_PROJECTS + 1, 280,
                                           (uint8_t *)s_browse_projects,
                                           MAX_BROWSE_PROJECTS, sizeof(BrowseProject));
}
#endif // PROJECTS_CACHE

// ---- one project's task list (level 1) ----
// Rows [0, s_browse_backlog_start) are the regular list; the rest are the
// backlog, shown as a second section under a "Backlog" header (the divider).

static int pt_regular_count(void) {
  int r = s_browse_backlog_start;
  if (r > s_browse_task_count) {
    r = s_browse_task_count;
  }
  if (r < 0) {
    r = 0;
  }
  return r;
}

static int pt_backlog_count(void) {
  return s_browse_task_count - pt_regular_count();
}

// Whether section `section` is the backlog one. When there's no regular list,
// the backlog takes section 0; otherwise it's section 1.
static bool pt_section_is_backlog(int section) {
  if (pt_regular_count() == 0) {
    return pt_backlog_count() > 0;
  }
  return section == 1;
}

static Task *resolve_browse_task_at(MenuIndex index) {
  if (!s_browse_tasks || s_browse_task_count == 0) {
    return NULL;
  }
  int base = pt_section_is_backlog((int)index.section) ? pt_regular_count() : 0;
  int i = base + (int)index.row;
  if (i >= s_browse_task_count) {
    return NULL;
  }
  return &s_browse_tasks[i];
}

// ---- shared menu callbacks (branch on s_browse_level) ----

static uint16_t browse_menu_get_num_sections(MenuLayer *menu_layer, void *context) {
  if (s_browse_level == 0) {
    return 1;
  }
  int n = (pt_regular_count() > 0 ? 1 : 0) + (pt_backlog_count() > 0 ? 1 : 0);
  return (uint16_t)(n > 0 ? n : 1);
}

static uint16_t browse_menu_get_num_rows(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  if (s_browse_level == 0) {
    return (uint16_t)s_browse_project_count;
  }
  if (s_browse_task_count == 0) {
    return 0;
  }
  return (uint16_t)(pt_section_is_backlog((int)section_index) ? pt_backlog_count() : pt_regular_count());
}

static int16_t browse_menu_get_header_height(MenuLayer *menu_layer, uint16_t section_index, void *context) {
  return (s_browse_level == 1 && pt_section_is_backlog((int)section_index)) ? MENU_CELL_BASIC_HEADER_HEIGHT : 0;
}

static void browse_menu_draw_header(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *context) {
  if (s_browse_level == 1 && pt_section_is_backlog((int)section_index)) {
    menu_cell_basic_header_draw(ctx, cell_layer, "Backlog");
  }
}

static void browse_menu_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *context) {
  GRect bounds = layer_get_bounds(cell_layer);
  MenuIndex sel = menu_layer_get_selected_index(s_browse_menu);
  bool is_selected = sel.section == cell_index->section && sel.row == cell_index->row;
  if (s_browse_level == 0) {
    BrowseProject *p = resolve_browse_project_at(*cell_index);
    if (!p) {
      return;
    }
    // Green with bold black text - the same treatment the today view gives a
    // project group header (menu_draw_header). The selected row darkens with
    // white text so it stands out (a bare text-colour flip on the bright fill
    // barely read). Any tag-flavoured mode uses the mint green of its nav row.
#ifndef PBL_PLATFORM_APLITE
    if (browse_wants_tags()) {
      fill_bg(ctx, bounds, is_selected ? GColorJaegerGreen : GColorMintGreen);
    } else
#endif
    {
      fill_bg(ctx, bounds, is_selected ? GColorIslamicGreen : GColorGreen);
    }
    int16_t title_y = HEADING_TITLE_Y(bounds.size.h);
    graphics_context_set_text_color(ctx, is_selected ? GColorWhite : GColorBlack);
#ifndef PBL_PLATFORM_APLITE
    if (s_browse_mode == BROWSE_TAG_EDIT) {
      // A checkbox (p->task_count 1 = tag assigned to s_browse_edit_task_id),
      // then the tag name. Select toggles it.
      GColor mk = is_selected ? GColorWhite : GColorBlack;
      GRect box = GRect(TITLE_BOX_X, (bounds.size.h - 16) / 2, 16, 16);
      graphics_context_set_stroke_color(ctx, mk);
      graphics_draw_rect(ctx, box);
      if (p->task_count) {
        graphics_context_set_fill_color(ctx, mk);
        graphics_fill_rect(ctx, GRect(box.origin.x + 4, box.origin.y + 4, 8, 8), 0, GCornerNone);
      }
      int16_t tx = TITLE_BOX_X + 22;
      draw_text(ctx, p->title, FONT_KEY_GOTHIC_24_BOLD,
                GRect(tx, title_y, bounds.size.w - tx - TITLE_BOX_X, HEADING_TITLE_H),
                GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
      return;
    }
#endif
    // The Inbox glyph for the default project, else the project's theme colour
    // as a swatch (draw_project_marker, shared with the today view). The phone
    // already packed the colour to a GColor8.
    int16_t text_x = draw_project_marker(ctx, TITLE_BOX_X, bounds.size.h, p->id,
                                          (uint8_t)p->color, is_selected);
    // Right-aligned count of the project's regular-list tasks (backlog and
    // done excluded, computed phone-side). Fixed-width strip so the title
    // ellipsis lands before it; 3 digits of GOTHIC_24_BOLD fit in 40 px.
    char count_buf[8];
    snprintf(count_buf, sizeof(count_buf), "%d", p->task_count);
    const int16_t count_w = 40;
    draw_text(ctx, count_buf, FONT_KEY_GOTHIC_24_BOLD,
              GRect(bounds.size.w - TITLE_BOX_X - count_w, title_y, count_w, HEADING_TITLE_H),
              GTextOverflowModeTrailingEllipsis, GTextAlignmentRight);
    draw_text(ctx, p->title, FONT_KEY_GOTHIC_24_BOLD,
              GRect(text_x, title_y, bounds.size.w - text_x - TITLE_BOX_X - count_w, HEADING_TITLE_H),
              GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft);
    return;
  }
  Task *bt = resolve_browse_task_at(*cell_index);
  if (!bt) {
    return;
  }
  // draw_task_row reads the same fields the today list draws; a browsed task
  // isn't in s_tasks but the struct is identical, so this reuses it wholesale
  // (including the live-ticking "spent / estimate" when it's the tracked one).
  // Tags mode (and a calendar day, which spans projects the same way a tag's
  // tasks do) passes show_project so each row names its project.
  bool show_project = false;
#ifndef PBL_PLATFORM_APLITE
  show_project = (unsigned)(s_browse_mode - BROWSE_TAGS) < 2; // TAGS or VIEWS (TAG_EDIT is level-0 only)
#ifdef PBL_PLATFORM_EMERY
  show_project = show_project || s_browse_mode == BROWSE_CALENDAR_DAY;
#endif
#endif
  draw_task_row(ctx, bounds, bt, is_selected, show_project);
}

// Switch to level 1 for `project_id` and fetch its tasks. Shared by a
// project-row Select in the browser, by push_browse_window's jump-straight-to
// path, and by the today view's project row (via push_browse_window). For
// BROWSE_CALENDAR_DAY, `project_id` is actually a "YYYY-MM-DD" date and the
// fetch goes out as MSG_CALDAY_REQUEST instead - the phone answers with the
// exact same PROJECT_TASKS_START/ITEM/END triplet either way (see
// handleCalDayRequest in index.js), so nothing past this branch needs to
// know the difference.
static void browse_descend(const char *project_id) {
  str_copy(s_browse_project_id, project_id, MAX_PROJECT_ID_LEN);
  if (!s_browse_tasks) {
    s_browse_tasks = malloc(sizeof(Task) * MAX_BROWSE_TASKS);
  }
  s_browse_task_count = 0;
  s_browse_task_incoming = 0;
  s_browse_backlog_start = 0;
  // A malloc failure here used to leave s_browse_tasks NULL and this stuck on
  // "Loading" forever - the PROJECT_TASKS_START/END handlers silently drop
  // everything while it's NULL, so nothing else ever clears the flag. Fail
  // visibly instead: skip the fetch and let browse_update_empty() show its
  // empty state.
  s_browse_tasks_loading = s_browse_tasks != NULL;
  s_browse_level = 1;
  if (s_browse_menu) {
    menu_layer_set_selected_index(s_browse_menu, MenuIndex(0, 0), MenuRowAlignTop, false);
    menu_layer_reload_data(s_browse_menu);
    browse_update_empty();
  }
  if (!s_browse_tasks) {
    return;
  }
#ifdef PBL_PLATFORM_EMERY
  if (s_browse_mode == BROWSE_CALENDAR_DAY) {
    begin_send(MSG_CALDAY_REQUEST, s_browse_project_id, NULL, 0);
    return;
  }
#endif
  request_project_tasks(s_browse_project_id);
}

// Level 0: Select opens the project's tasks. Level 1: Select opens the task's
// action menu, same as the today list (menu_select_click). A Select within a
// pending-reschedule window just cancels it instead (no menu).
static void browse_menu_select_click(MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  backlight_touch();
  if (s_pending_reschedule_kind != RESCHEDULE_NONE) {
    cancel_pending_reschedule();
    return;
  }
  if (s_browse_level == 0) {
    BrowseProject *p = resolve_browse_project_at(*cell_index);
    if (!p) {
      return;
    }
#ifndef PBL_PLATFORM_APLITE
    if (s_browse_mode == BROWSE_TAG_EDIT) {
      p->task_count = p->task_count ? 0 : 1; // flip the checkbox
      menu_layer_reload_data(s_browse_menu);
      send_toggle_tag(s_browse_edit_task_id, p->id, p->task_count);
      return;
    }
    if (s_browse_mode == BROWSE_MOVE) {
      send_move_to_project(s_browse_edit_task_id, p->id);
      window_stack_pop(true); // back to whatever opened the picker
      return;
    }
#endif
    browse_descend(p->id);
    return;
  }
  Task *bt = resolve_browse_task_at(*cell_index);
  if (!bt) {
    return;
  }
  // Matches the today list's own convention (Select opens the action menu -
  // "Mark done" its first row - rather than toggling in place); same ctx/
  // in_backlog computation as browse_menu_select_long_click, which this now
  // duplicates exactly.
  bool no_project = (s_browse_project_id[0] == '\0' ||
                     strncmp(s_browse_project_id, NO_PROJECT_ID_STR, MAX_PROJECT_ID_LEN) == 0);
  bool calendar_day = false;
#ifdef PBL_PLATFORM_EMERY
  calendar_day = s_browse_mode == BROWSE_CALENDAR_DAY;
#endif
  ActionCtx ctx = (browse_wants_tags() || no_project || calendar_day) ? ACTX_TAG : ACTX_PROJECT;
  push_action_menu(bt->id, ctx, pt_section_is_backlog((int)cell_index->section));
}

// Level 0: long-Select opens the selected project's notes (a project has no
// tags; a tag row is a no-op). Level 1: long-Select opens the per-task action
// menu, same as the today list - with "Move to backlog/list" as an extra row
// for a project task.
static void browse_menu_select_long_click(MenuLayer *menu_layer, MenuIndex *cell_index, void *context) {
  backlight_touch();
  if (s_browse_level == 0) {
#ifndef PBL_PLATFORM_APLITE
    if (browse_wants_tags() || s_browse_mode == BROWSE_MOVE) {
      return; // a tag has no notes; the MOVE picker has no long-press
    }
#endif
    BrowseProject *p = resolve_browse_project_at(*cell_index);
    if (!p) {
      return;
    }
    s_notes_tags_line[0] = '\0';
    show_notes_overlay_for(p->id, true);
    return;
  }
#ifndef PBL_PLATFORM_APLITE
  Task *bt = resolve_browse_task_at(*cell_index);
  if (!bt) {
    return;
  }
  bool no_project = (s_browse_project_id[0] == '\0' ||
                     strncmp(s_browse_project_id, NO_PROJECT_ID_STR, MAX_PROJECT_ID_LEN) == 0);
  // ctx only decides whether the "Move to backlog" row shows - tags, the
  // "No Project" bucket, and a calendar day (s_browse_project_id there is a
  // date, not a real project - "Move to backlog" would send it as one) have
  // no backlog. Scheduling itself routes through begin_pending_reschedule's
  // own window/mode check, not ctx.
  bool calendar_day = false;
#ifdef PBL_PLATFORM_EMERY
  calendar_day = s_browse_mode == BROWSE_CALENDAR_DAY;
#endif
  ActionCtx ctx = (browse_wants_tags() || no_project || calendar_day) ? ACTX_TAG : ACTX_PROJECT;
  push_action_menu(bt->id, ctx, pt_section_is_backlog((int)cell_index->section));
#endif
}

static void browse_update_empty(void) {
  bool empty;
  const char *msg;
  bool tags = false;
#ifndef PBL_PLATFORM_APLITE
  tags = browse_wants_tags();
#endif
  if (s_browse_level == 0) {
    empty = s_browse_project_count == 0;
    msg = s_browse_projects_loading ? (tags ? "Loading tags..." : "Loading projects...")
                                    : (tags ? "No tags." : "No projects.");
  } else {
    empty = s_browse_task_count == 0;
    bool calendar_day = false;
#ifdef PBL_PLATFORM_EMERY
    calendar_day = s_browse_mode == BROWSE_CALENDAR_DAY;
#endif
    msg = s_browse_tasks_loading ? "Loading..."
         : calendar_day ? "No tasks that day."
         : tags ? "No tasks for this tag." : "No tasks in this project.";
  }
  text_layer_set_text(s_browse_empty, msg);
  layer_set_hidden(text_layer_get_layer(s_browse_empty), !empty);
  layer_set_hidden(menu_layer_get_layer(s_browse_menu), empty);
}

// Back at level 1 returns to the (still-loaded) project list; at level 0 it
// pops the window. MenuLayer owns UP/DOWN/SELECT via its own click config;
// this wraps that provider to add BACK, the same pattern the main window uses
// for its long-press gestures.
static ClickConfigProvider s_browse_menu_ccp = NULL;

static void browse_back_click_handler(ClickRecognizerRef recognizer, void *context) {
#ifdef PBL_PLATFORM_EMERY
  // A calendar day has no level 0 to fall back to - Back just leaves.
  if (s_browse_mode == BROWSE_CALENDAR_DAY) {
    window_stack_pop(true);
    return;
  }
#endif
  if (s_browse_level == 1) {
    backlight_touch();
    free(s_browse_tasks);
    s_browse_tasks = NULL;
    s_browse_task_count = 0;
    s_browse_project_id[0] = '\0';
    s_browse_tasks_loading = false;
    s_browse_level = 0;
    // Entering the browser via a today-view project row jumps straight to
    // level 1 and does NOT fetch the list up front (two back-to-back
    // begin_send()s collide on the one outbox slot). Fetch it now, on the
    // first Back, if it isn't already in hand.
    if (s_browse_project_count == 0 && !s_browse_projects_loading) {
      s_browse_projects_loading = true;
      request_project_list();
    }
    menu_layer_reload_data(s_browse_menu);
    menu_layer_set_selected_index(s_browse_menu, MenuIndex(0, 0), MenuRowAlignTop, false);
    browse_update_empty();
    return;
  }
  window_stack_pop(true);
}

// Wraps MenuLayer's provider only to add BACK (level 1 -> level 0). Scheduling
// and backlog moves are in the per-task action menu now (long-Select a task).
static void browse_menu_click_config_provider(void *context) {
  if (s_browse_menu_ccp) {
    s_browse_menu_ccp(context);
  }
  window_single_click_subscribe(BUTTON_ID_BACK, browse_back_click_handler);
}

static void browse_window_load(Window *window) {
  Layer *window_layer;
  GRect content = window_chrome(window, &s_browse_status_bar, &window_layer);

  s_browse_menu = menu_layer_create(content);
  menu_layer_set_callbacks(s_browse_menu, NULL, (MenuLayerCallbacks) {
    .get_num_sections = browse_menu_get_num_sections,
    .get_num_rows = browse_menu_get_num_rows,
    .get_header_height = browse_menu_get_header_height,
    .draw_header = browse_menu_draw_header,
    .draw_row = browse_menu_draw_row,
    .select_click = browse_menu_select_click,
    .select_long_click = browse_menu_select_long_click,
  });
  menu_layer_set_click_config_onto_window(s_browse_menu, window);
  s_browse_menu_ccp = window_get_click_config_provider(window);
  window_set_click_config_provider_with_context(window, browse_menu_click_config_provider,
                                                window_get_click_config_context(window));
  layer_add_child(window_layer, menu_layer_get_layer(s_browse_menu));

  s_browse_empty = make_text_layer(window_layer, content, EMPTY_MSG_FONT_KEY, GTextAlignmentCenter);

  browse_update_empty();
}

static void browse_window_unload(Window *window) {
  menu_layer_destroy(s_browse_menu);
  text_layer_destroy(s_browse_empty);
  status_bar_layer_destroy(s_browse_status_bar);
  s_browse_menu = NULL;
  s_browse_empty = NULL;
  s_browse_status_bar = NULL;
  s_browse_menu_ccp = NULL;
  free(s_browse_projects);
  s_browse_projects = NULL;
  free(s_browse_tasks);
  s_browse_tasks = NULL;
  s_browse_project_count = 0;
  s_browse_task_count = 0;
  s_browse_projects_loading = false;
  s_browse_tasks_loading = false;
  s_browse_project_id[0] = '\0';
  s_browse_level = 0;
}

// jump_to_project non-NULL opens straight at that project's tasks (level 1) -
// the today view's project row uses this. Back from there lands on the project
// list, fetched then (see browse_back_click_handler).
static void push_browse_window(const char *jump_to_project) {
  s_browse_level = 0;
  if (!s_browse_projects) {
    s_browse_projects = malloc(sizeof(BrowseProject) * MAX_BROWSE_PROJECTS);
  }
  s_browse_project_count = 0;
  s_browse_project_incoming = 0;
#if PROJECTS_CACHE
  if (!browse_wants_tags()
#ifdef PBL_PLATFORM_EMERY
      && s_browse_mode != BROWSE_CALENDAR_DAY
#endif
     ) {
    // PROJECTS and the MOVE picker both show the project list - render the cache
    // instantly, the fetch below refreshes it. Tags and a calendar day aren't
    // cached - the latter never even reaches level 0.
    load_browse_projects();
  }
#endif
  bool jumping = jump_to_project && jump_to_project[0] != '\0';
  // Only one begin_send() may be in flight (single outbox slot). A jump sends
  // just the task fetch now; browse_back_click_handler fetches the list later.
  s_browse_projects_loading = !jumping && (s_browse_project_count == 0);
  if (!s_browse_window) {
    s_browse_window = window_create();
    window_set_window_handlers(s_browse_window, (WindowHandlers) {
      .load = browse_window_load,
      .unload = browse_window_unload,
    });
  }
  window_stack_push(s_browse_window, true);
  if (jumping) {
    browse_descend(jump_to_project);
  } else {
    request_project_list();
  }
}
#endif // PROJECTS_BROWSER
