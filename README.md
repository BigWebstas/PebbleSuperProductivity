# Pebble Super Productivity

A PebbleOS watchapp that shows your active tasks from
[Super Productivity](https://super-productivity.com), optionally grouped by
project, and lets you mark them done, track time, dictate new tasks by voice, and
track habits — synced through Super Productivity's own **SuperSync** server.

| | | |
|---|---|---|
| ![Pinned action rows](screenshots/pebble-time-2/task-list-top.png) | ![Grouped task list](screenshots/pebble-time-2/task-list-garden.png) | ![Time tracking](screenshots/pebble-time-2/task-list-work.png) |
| ![Habits](screenshots/pebble-time-2/habits.png) | ![Scrolling](screenshots/pebble-time-2/task-list-scroll.gif) | ![Marking done](screenshots/pebble-time-2/task-complete.gif) |

Shown at Pebble Time 2 size. The same shots sized for other platforms:
[`pebble-time-2`](screenshots/pebble-time-2) (200×228),
[`pebble-round`](screenshots/pebble-round) (180×180, chalk),
[`pebble-time`](screenshots/pebble-time) and
[`pebble-2`](screenshots/pebble-2) (144×168).

## Controls

- **Up / Down** — scroll the list
- **Select** — toggle a task done; on a project header, open that project (see
  [Projects](#projects)); on an action row, activate it (Resync, Habits,
  Projects, [Stats](#stats), Add Task)
- **Long-press Select** — start/stop time tracking; open a project's notes;
  append a note on the notes screen; increment a habit
- **Double-click Select** — show a task's notes and tags
- **Long-press Down** — move the highlighted task to tomorrow
- **Long-press Up** — clear the highlighted task's scheduling (today list)
- **Back** — previous screen

Long-press Up/Down opens a 3-second cancel window — the row shows "Moving to
tomorrow..." / "Un-Scheduling..." (or "Scheduling for today..." in the Projects
view), and a Select press cancels before it commits. Not available on `aplite`.

### Projects

The **Projects** action row opens a list of every project; pick one for its
tasks — the live list, then its backlog under a divider. Not on `aplite`.

- **Select** a project — open its tasks
- **Long-press Select** a project — open its notes
- **Select** a task — toggle it done
- **Long-press Select** a task — start / stop tracking it
- **Long-press Down** a task — move it to tomorrow
- **Long-press Up** a task — move it to today

Scheduling a task that's in the backlog also moves it into the regular list.
Toggling done and tracking round-trip to the desktop, and a task tracked here
shows in the today page's TRACKING section too.

### Stats

The **Stats** action row opens a read-only summary, scrollable with Up / Down.
Not on `aplite`.

- **Estimate remaining** — unfinished estimated time across today's tasks
- **Worked today** — time tracked today across every task
- **Current session** — how long the active tracking run has lasted: the
  watch's own, or a remote device's when the watch isn't tracking (the
  desktop's "time without a break" is local to the desktop and can't sync,
  so this is the closest number the watch has)
- **Completed today** — tasks done among today's list
- **Projects** — every project with its open-task count

### Touch (Pebble Time 2)

Experimental, off by default — enable **Touch navigation** in settings.

- **Swipe up / down** — scroll
- **Tap a row** — select it only (toggling done, opening notes, and bumping a
  habit stay on the Select button)
- **Long-press a row** — the same action as long-press Select
- **Swipe left** — move the highlighted task to tomorrow
- **Swipe right** — back

On first-generation Time 2 hardware the touch driver misreads edge taps, so
touch is opt-in until a firmware fix lands.

## Pairing

1. Install the watchapp, open the Pebble app, find "Super Productivity" in your
   watchapps, tap Settings.
2. Enter your SuperSync server URL, email, and sync encryption password. Tap
   "Open SuperSync login", sign in, copy the token, and paste it into "SuperSync
   access token". Adjust the display settings, then Save.
3. The watch syncs on next launch.

## Limitations

- **Initial sync is slow** — the first decrypt can take 5+ minutes.
- **Add Task** needs a Pebble with a mic and an internet connection (dictation
  runs in the cloud).
- **Only some actions round-trip** — completion, scheduling, backlog/project
  moves, subtask promote/demote, time tracking, habits. Anything else (deadlines,
  tags) is a no-op on replay.
- **Offline changes are queued, conflicts are not** — a change made while
  the phone is offline is held locally and re-sent, in order, on the next
  sync. One the server actively rejects (a conflict with a newer edit made
  elsewhere) is dropped rather than retried.
- **No delete from the watch** — creation and completion round-trip; deleting a
  task or habit still needs the desktop/mobile app.
- **`aplite` (original Pebble / Steel)** is RAM-limited: Finish Day, StopWatch
  habits, the over-estimate notification, and pin-tracked-task are unavailable.

## Downloads & changelog

`.pbw` artifacts and full release notes:
[Releases](https://github.com/BigWebstas/PebbleSuperProductivity/releases).
