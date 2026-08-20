# Pebble Super Productivity

A PebbleOS watchapp that shows your active tasks from [Super
Productivity](https://super-productivity.com) — optionally grouped by
project — and lets you mark them done, track time, dictate new tasks by
voice, and track habits, all from your wrist, synced through Super
Productivity's own **SuperSync** server.

## Screenshots

| | | |
|---|---|---|
| ![Resync, Habits, and Add Task rows](screenshots/task-list-top.png) | ![Grouped task list with a due time](screenshots/task-list-garden.png) | ![Task list with time tracking](screenshots/task-list-work.png) |
| Pinned action rows, plus the app version footer | Per-project grouping, done-task dimming, due times | Time tracking (`spent / estimate`) |

| |
|---|
| ![Habits page](screenshots/habits.png) |
| Habit tracking (Super Productivity's SimpleCounter feature) — increment/decrement with Select/long-Select |


## Pairing

1. Install the watchapp, open the Pebble mobile app, find "Super
   Productivity" in your watchapps, and tap Settings.
2. Enter your SuperSync server URL, email, and sync encryption password.
   Tap "Open SuperSync login", sign in there, copy the token it displays,
   and paste it into the "SuperSync access token" field. Adjust the display
   settings if you want (group by project, today-only, auto-sync, default
   project for dictated tasks, enable/disable Habits or Add Task). Save.
3. The watch syncs automatically on next launch, or immediately if it's
   already open.

## Changelog

- **v0.6.14** — close the app automatically once Finish Day's archive send is confirmed; bigger subtitle text on task/habit rows; habit-page text stays black when selected; subtask marker changed from `~` to `»`
- **v0.6.13** — add a "Finish Day" row (long-select, always last in the task list) that archives every currently-done task; not available on aplite (RAM constraints)
- **v0.6.12** — StopWatch-type habits now show a live timer (long-select to start/stop), formatted like a task's spent/estimate subtitle; not available on aplite (RAM constraints)
- **v0.6.11** — change the Add Task row's icon from a mic glyph to a plus sign
- **v0.6.10** — fix watch-uploaded task/time/habit ops silently failing to apply on desktop (missing `entityChanges` on the op payload)
- **v0.6.9** — fix voice-added tasks not showing under Today Only, add settings to disable Habits/Add Task
- **v0.6.8** — fix "Add to Today" leaving a stale `dueWithTime` that hid the task from Today Only
- **v0.6.7** — add voice-dictated task creation via a new "Add Task" row
- **v0.6.6** — keep the version footer's text black instead of gray
- **v0.6.5** — add a version footer row to the task list, stop dimming done habit titles
- **v0.6.4** — fix a task planned-for-today (no time) not syncing to the watch, change Resync's idle subtitle to "Synced"
- **v0.6.3** — keep habit count visible alongside "Done", match selected-row color to nav
- **v0.6.2** — habit increment/decrement, row icons, task time estimate, sync message
- **v0.6.1** — fix todayOnly wrongly hiding a backlog task planned for today
- **v0.6.0** — add a habits page

See [GitHub Releases](https://github.com/BigWebstas/PebbleSuperProductivity/releases) for the full history.

## Known limitations

- **Initial Sync slow?** The initial sync and decrypt can take several minutes upwards of 5+
- **Add Task Requirements.** The Add Task feature requires a newer Pebble with a Mic and a steady internet connection as its done on the phone via the cloud.
- **Only a subset of Super Productivity actions round-trip.** Task
  completion, scheduling, backlog moves, project moves, subtask
  promotion/demotion, time tracking, and habit adjustments are handled;
  anything else (deadlines, tags, etc.) is a no-op on replay rather than a
  crash. See `task-store.js`'s `applyTaskAction` for the full handled list.
- **No offline retry queue.** If uploading a change fails (phone offline,
  server error), it's logged and left for the next full sync to reconcile.
- **No delete from the watch.** Task/habit creation (via "Add Task" voice
  dictation) and completion round-trip; deleting an existing task or habit
  still requires the desktop/mobile app.
- **Conflict resolution** relies entirely on SuperSync's own operation-log
  ordering; this client doesn't do any additional merging.
- Subtasks are never selected independently — they ride along with their
  parent task, indented with a `~` prefix.
