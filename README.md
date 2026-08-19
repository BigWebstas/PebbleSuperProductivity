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
