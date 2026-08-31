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

| | | |
|---|---|---|
| ![Scrolling the task list](screenshots/task-list-scroll.gif) | ![Marking a task done](screenshots/task-complete.gif) | ![Incrementing habits](screenshots/habits.gif) |
| Scrolling the whole list — action rows, per-project groups, due times, time tracking, the Finish Day row and version footer | Select on a task toggles it done | Open Habits, Select to increment a counter toward its goal |

## Navigation and Functionality
* Up and down buttons to scroll through the habits and task list
* Select (Middle button)
    * Click once to toggle Done/In-progress
    * Long hold to start time tracking when on tasks and habits
      * Optionally (enable "Notify when a task runs over its estimate" in settings) the watch vibrates and shows a banner once the tracked task's time spent reaches its estimate — with a sub-option to repeat that every 5 minutes while it stays over — not on original Pebble/Pebble Steel (aplite)
      * Optionally (enable "Pin the task you're tracking to the top" in settings) the task being tracked moves into its own "Tracking" section at the top of the list until ~10s after you stop — not on original Pebble/Pebble Steel (aplite)
    * Long hold to append a note when on the notes screen
    * Double click to show task and project notes, plus a task's tags (shown above its notes)
      * To show projects and project notes you must enable "Group tasks by project" in settings
* Back returns to the prior screen

### Touch (Pebble Time 2)

Experimental, **off by default** — enable "Touch navigation" in settings.

* Swipe up/down to scroll; swipe right to go back
* Tap a row to activate it — toggle a task done, open Habits/Add Task, tap a
  note to dismiss it
* Long-press for the same action a long Select press does — start/stop time
  tracking, Finish Day, habit −1 / StopWatch, dictate a note
* Double-click-to-view-notes stays on the physical Select button
* The physical buttons keep working regardless

Scrolling, taps, and swipe-back come from PebbleOS's own touch layer; this app
only adds the long-press. The first-gen Time 2 touch firmware currently
misreads taps near the screen edges (they register as swipes), so it works
best for rows in the middle of the screen — hence off by default until that's
fixed in firmware.

## App Settings

* Pretty straight forward, all settings should have a subtext describing their function
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

See the [Releases page](https://github.com/BigWebstas/PebbleSuperProductivity/releases) for downloadable `.pbw` artifacts of each version.

### [v0.6.24](https://github.com/BigWebstas/PebbleSuperProductivity/releases/tag/v0.6.24)
- The over-estimate notification now has a "Repeat the notification every 5 minutes" sub-option: with it on, the watch keeps re-vibrating and re-showing the banner every 5 minutes for as long as the tracked task stays over its estimate, instead of notifying just once.
- Bigger, better-aligned text throughout the task list, habits and notes screens on the Pebble Time 2, matching its larger display. Other watches are unchanged.
- Dropped the underline under project group names in the task list; the green header bar already sets them apart.

### [v0.6.23](https://github.com/BigWebstas/PebbleSuperProductivity/releases/tag/v0.6.23)
- Add an optional "Pin the task you're tracking to the top" setting: while a task's timer is running it moves into its own "Tracking" section at the top of the list (with its live duration, and its project when grouping is on) and out of its normal group, sliding back into place about 10 seconds after you stop. Off by default; not available on original Pebble/Pebble Steel.
- Add an optional "Notify when a task runs over its estimate" setting: while tracking a task that has a time estimate, the watch vibrates and shows a banner the moment the time spent reaches that estimate. Off by default; not available on original Pebble/Pebble Steel.
- A task you complete on the watch with "Hide completed tasks" on now stays visible for about 10 seconds before disappearing, up from 5.

### [v0.6.22](https://github.com/BigWebstas/PebbleSuperProductivity/releases/tag/v0.6.22)
- Fix the Habits screen crashing the app (out of memory) on every watch except the Pebble Time 2.
- Show subtasks of recurring tasks on the watch, instead of just the bare parent row (also fixes any subtask added since the last full sync going missing). Existing accounts need one "Clear all data & resync" to pick up subtasks synced before this fix.
- Recover automatically when "Clear all data & resync" hits a server error fetching the starting snapshot, instead of getting stuck retrying forever.
- Add optional touch navigation for the Pebble Time 2: swipe to scroll, tap a row to toggle/open it, long-press for the press-and-hold action, swipe right to go back. Off by default while the Time 2's touch firmware matures - enable it under "Touch navigation" in settings.
- Wait longer before the notes screen shows a "couldn't load" error, so opening a task's notes right after launching the app no longer fails spuriously while the first sync is still running.

### [v0.6.21](https://github.com/BigWebstas/PebbleSuperProductivity/releases/tag/v0.6.21)
- Faster syncs on encrypted accounts: the watch now remembers the slow-to-compute decryption keys between app opens instead of recomputing them every time, so most syncs after the first skip that work entirely.

### [v0.6.20](https://github.com/BigWebstas/PebbleSuperProductivity/releases/tag/v0.6.20)
- Fix "Clear all data & resync" sometimes leaving the watch showing stale/blank data instead of forcing a fresh sync, if reopened within 5 minutes of the previous sync.

### [v0.6.19](https://github.com/BigWebstas/PebbleSuperProductivity/releases/tag/v0.6.19)
- Add project notes: when "group tasks by project" is on, double-click Select on a project row to view (and long-select to append to) its notes, same as a task's.
- Show a task's tags above its notes in the notes overlay (double-click Select on a task).
- Always show the tags section in the notes overlay, saying "No tags for this task"/"No tags for this project" instead of hiding it when there are none, mirroring the notes body's own "(No notes for this task)" placeholder.
- Remove the "Show completed habits last" and "Hide completed habits" settings - habits now always sort plain alphabetically.
- Skip a redundant full sync when reopening the app within 5 minutes of the last successful one, pushing the already-fresh cached list to the watch instead.

### [v0.6.18](https://github.com/BigWebstas/PebbleSuperProductivity/releases/tag/v0.6.18)
- Add a "Complete main task when all subtasks are done" setting (off by default), mirroring the real app's own auto-complete-parent behavior.
- Fix an auto-completed parent task vanishing from the watch's list without ever showing "Done" when paired with "Hide completed tasks".

### [v0.6.17](https://github.com/BigWebstas/PebbleSuperProductivity/releases/tag/v0.6.17)
- Add a backlight setting (always-on, or relight-and-hold for a custom duration after any button press).
- Double-click Select on a task to view its notes.
- A task marked done on the watch now stays visible for ~5 seconds before disappearing when "Hide completed tasks" is on, instead of vanishing as soon as the next auto-sync lands.
- "Sync automatically on a timer" now actually syncs while the app is closed too, via the watch periodically waking itself up (a brief screen flash each time, not a silent background refresh) - not available on aplite (RAM).

### [v0.6.16](https://github.com/BigWebstas/PebbleSuperProductivity/releases/tag/v0.6.16)
- Add a "Hide completed tasks" setting: removes a done task from the watch's list entirely instead of showing it dimmed (a done subtask under a still-open parent is hidden individually; a done main task hides its whole subtask block along with it). Off by default.

### [v0.6.15](https://github.com/BigWebstas/PebbleSuperProductivity/releases/tag/v0.6.15)
- Raise the task/habit limits to 50/16 on emery (Pebble Time 2).
- Add RepeatedCountdownReminder habit support with its own long-select-to-start/stop timer (Select pauses/resumes it) - not available on aplite.
- Add a background auto-sync-on-a-timer setting.
- Retry a failed watch↔phone send a few times before showing a sync error, instead of failing on the first hiccup.
- Fix a habit list silently truncated to 8 even on platforms that could show more.
- Habits now sort alphabetically by default, with a setting to restore the old not-done-before-done grouping.

### [v0.6.14](https://github.com/BigWebstas/PebbleSuperProductivity/releases/tag/v0.6.14)
- Close the app automatically once Finish Day's archive send is confirmed.
- Bigger subtitle text on task/habit rows.
- Habit-page text stays black when selected.
- Subtask marker changed from `~` to `»`.

### [v0.6.13](https://github.com/BigWebstas/PebbleSuperProductivity/releases/tag/v0.6.13)
- Add a "Finish Day" row (long-select, always last in the task list) that archives every currently-done task; not available on aplite (RAM constraints).

### [v0.6.12](https://github.com/BigWebstas/PebbleSuperProductivity/releases/tag/v0.6.12)
- StopWatch-type habits now show a live timer (long-select to start/stop), formatted like a task's spent/estimate subtitle; not available on aplite (RAM constraints).

### [v0.6.11](https://github.com/BigWebstas/PebbleSuperProductivity/releases/tag/v0.6.11)
- Change the Add Task row's icon from a mic glyph to a plus sign.

### [v0.6.10](https://github.com/BigWebstas/PebbleSuperProductivity/releases/tag/v0.6.10)
- Fix watch-uploaded task/time/habit ops silently failing to apply on desktop (missing `entityChanges` on the op payload).

### [v0.6.9](https://github.com/BigWebstas/PebbleSuperProductivity/releases/tag/v0.6.9)
- Fix voice-added tasks not showing under Today Only.
- Add settings to disable Habits/Add Task.

### [v0.6.8](https://github.com/BigWebstas/PebbleSuperProductivity/releases/tag/v0.6.8)
- Fix "Add to Today" leaving a stale `dueWithTime` that hid the task from Today Only.

### [v0.6.7](https://github.com/BigWebstas/PebbleSuperProductivity/releases/tag/v0.6.7)
- Add voice-dictated task creation via a new "Add Task" row.

### v0.6.6
- Keep the version footer's text black instead of gray.

### [v0.6.5](https://github.com/BigWebstas/PebbleSuperProductivity/releases/tag/v0.6.5)
- Add a version footer row to the task list.
- Stop dimming done habit titles.

### [v0.6.4](https://github.com/BigWebstas/PebbleSuperProductivity/releases/tag/v0.6.4)
- Fix a task planned-for-today (no time) not syncing to the watch.
- Change Resync's idle subtitle to "Synced".

### [v0.6.3](https://github.com/BigWebstas/PebbleSuperProductivity/releases/tag/v0.6.3)
- Keep habit count visible alongside "Done".
- Match selected-row color to nav.

### [v0.6.2](https://github.com/BigWebstas/PebbleSuperProductivity/releases/tag/v0.6.2)
- Habit increment/decrement.
- Row icons.
- Task time estimate.
- Sync message.

### [v0.6.1](https://github.com/BigWebstas/PebbleSuperProductivity/releases/tag/v0.6.1)
- Fix todayOnly wrongly hiding a backlog task planned for today.

### [v0.6.0](https://github.com/BigWebstas/PebbleSuperProductivity/releases/tag/v0.6.0)
- Add a habits page.

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
