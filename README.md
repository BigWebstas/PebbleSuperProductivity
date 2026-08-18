# Pebble Super Productivity

A PebbleOS watchapp that shows your active tasks from [Super
Productivity](https://super-productivity.com) - optionally grouped by
project - and lets you mark them done from your wrist, synced through
Super Productivity's own **SuperSync** server.

## Architecture

```
 ┌─────────────┐   AppMessage    ┌──────────────────┐   HTTPS (JSON)   ┌──────────────────┐
 │  Watch (C)  │ ◄──────────────► │ PebbleKit JS      │ ◄───────────────► │ SuperSync server │
 │ src/c/main.c│   over Bluetooth │ src/pkjs/index.js │                   │ (self-hosted or  │
 └─────────────┘                  └──────────────────┘                   │ sync.super-      │
                                                                          │ productivity.com)│
                                                                          └──────────────────┘
```

**Why the phone is in the loop:** the request that kicked this project off
asked for the watch to reach SuperSync directly over WiFi. I checked the
Pebble C SDK reference and the PebbleOS docs and found no networking module
at all — no sockets, no HTTP client, no TLS — on any current Pebble/PebbleOS
hardware. Every existing Pebble watchapp that touches the internet does it
by relaying through **PebbleKit JS**, a JS component that runs inside the
paired phone's Pebble app. So that's what this does: the watch talks
AppMessage to the phone, and the phone does the actual HTTPS calls to
SuperSync. If PebbleOS ever ships a watch-native networking API, only
`src/pkjs/index.js` would need to move to `src/c/`.

### Components

- **`src/c/main.c`** — the watchapp. A sectioned `MenuLayer`: a "Resync" row
  pinned to section 0 with a red background and black text (a standing
  call-to-action, not just another list item), its subtitle reflecting live
  sync status so a failed resync is visible even with a cached list still
  showing, followed by one section per project group when "Group tasks by
  project" is on (large, bold, underlined, blue project-name header per
  section - drawn manually with `graphics_draw_text`/`graphics_draw_line`,
  since Pebble has no built-in bold+underline cell style) or a single
  unheaded section when it's off. Every task row (not just the ones that
  need to scroll or show "Done") is drawn through the same custom path
  rather than `menu_cell_basic_draw`, so title size is consistent across the
  whole list; select-click toggles done/not-done, and a done task's "Done"
  status shows centered under its title (something `menu_cell_basic_draw`'s
  subtitle can't do - it's always left-aligned). A selected title too wide
  for the screen scrolls as a looping marquee instead, black background/
  white text matching the platform's own invert-on-select style
  (`AppTimer`-driven, redrawn via `menu_draw_row`'s own selection-state
  check - see "Marquee title scrolling" below). The empty/status screen
  shown before any task list has ever loaded animates ("Syncing" ->
  "Syncing." -> ".." -> "...", another `AppTimer`) while that first sync is
  in flight, instead of sitting on static text with no sign anything is
  happening. Persists the last-synced list via the `Storage` API so the
  list survives an app relaunch even offline.
- **`src/pkjs/index.js`** — the sync engine. Downloads operations from
  SuperSync, replays them into a local task cache, decrypts payloads if
  end-to-end encryption is on, sends the active task list to the watch
  (grouped by project first if that setting is on), and uploads a
  task-completion operation when you toggle one on the watch, wrapped in
  the same `{ actionType: "[Task Shared] updateTask", payload:
  { actionPayload: { task: { id, changes } } } }` shape the real op log
  uses - see "What is verified vs. assumed" below for why the flat shape
  this used to send didn't actually round-trip.
- **`src/pkjs/lib/supersync-client.js`** — thin REST client for the
  SuperSync API (`/api/sync/ops`, `/api/sync/restore-points`,
  `/api/sync/restore/:serverSeq`) plus `createCrypto(password)`, which
  builds the real Argon2id/AES-GCM decrypt+encrypt pair (with a per-salt
  key cache, since Argon2id is multi-second at production parameters).
- **`src/pkjs/lib/task-store.js`** — replays SuperSync's op log into an
  entity cache and picks the active (non-backlog) task list out of it,
  optionally grouped by project. This is a Redux action-replay log, not a
  flat CRUD op log - see the file's top comment.
- **`src/pkjs/lib/blake2b.js`, `argon2id.js`** — dependency-free BLAKE2b and
  Argon2id (RFC 9106), specialized for `parallelism=1` (a hardcoded app
  constant). 64-bit words are `[hi, lo]` pairs of plain numbers, not
  BigInt: the Pebble build toolchain's bundler can't parse BigInt literal
  syntax. **Verified against `hash-wasm`'s output** (the same library the
  real client uses), including full production parameters and a real
  account's actual ciphertext — see Testing below.
- **`src/pkjs/lib/aes-gcm.js`, `sha256.js`, `base64.js`** — dependency-free
  AES-128/256-GCM, SHA-256/HMAC/PBKDF2, and base64, written from scratch
  because the PebbleKit JS runtime does not reliably expose
  `window.crypto.subtle` across phone platforms/app versions. **Verified
  byte-for-byte against Node's native `crypto` module** — see Testing below.
- **`config/pairing.html`** — plain-HTML reference copy of the phone-side
  pairing page. `src/pkjs/lib/pairing-page.js` mirrors this same markup and
  is what actually ships, inlined as a `data:` URI and opened from the
  watchapp's Settings entry in the Pebble mobile app.
- **`resources/images/icon.png`** — the watch app-launcher icon: Super
  Productivity's own checkmark logo (`src/assets/icons/sp.svg` in the
  super-productivity repo, MIT-licensed), rendered down to Pebble's 25×25
  menu-icon size limit.

## ⚠️ What is verified vs. assumed

This started with no way to create a real SuperSync account (registration
needs a live email inbox / WebAuthn ceremony), so the wire format was
originally built from route lists and architecture prose, not captured
traffic. Since then, three things resolved almost everything that was
"assumed" down to "verified":

1. Reading the actual super-productivity/super-productivity GitHub repo's
   source (`packages/sync-core/src/encryption.ts`, `encryption/argon2.ts`,
   and the sync server's own `app.js`) instead of guessing.
2. A user sharing a real (temporary, since-rotated) SuperSync API token,
   letting the actual API routes and `GET /api/sync/ops` response shape be
   checked against live traffic.
3. The same user sharing their real (temporary, since-rotated) sync
   encryption password, letting the full E2EE pipeline be checked against
   their actual encrypted tasks — not just spec compliance, but a real
   AES-GCM authentication-tag pass against production ciphertext.

**Solid — actually verified:**
- The crypto primitives (`aes-gcm.js`, `sha256.js`, `base64.js`) are checked
  against Node's built-in `crypto` module for AES-128/256-GCM
  encrypt/decrypt/tag-verification, HMAC-SHA256, PBKDF2, and SHA-256, plus
  base64 round-tripping. Run `node scripts/test-crypto.js`.
- `blake2b.js`/`argon2id.js` are checked against `hash-wasm` (the library
  Super Productivity's real client uses) across RFC 7693 BLAKE2b vectors
  and ~20 Argon2id cases spanning tiny-to-production memory/iteration
  parameters, hash lengths, and passwords. Run `node scripts/test-argon2.js`
  (the production-parameters case takes ~15s - Argon2id is a memory-hard
  KDF by design).
- The E2EE pipeline end-to-end against a real account: the exact wire
  format (`[16-byte salt][12-byte IV][AES-GCM ciphertext+tag]`, base64,
  Argon2id with `parallelism=1, iterations=3, memorySize=65536 KiB`, a
  legacy PBKDF2 fallback for short/old ciphertexts), confirmed by
  successfully decrypting real tasks - AES-GCM's authentication tag is a
  cryptographic pass/fail, not a guess that merely looks plausible.
- The operation-replay/active-list logic (`task-store.js`) is unit tested
  in isolation, and was run against one real account's entire decrypted
  500-op history. Run `node scripts/test-task-store.js`.
- The AppMessage protocol between `main.c` and `index.js` is internally
  consistent (same key/enum values on both sides).
- The pairing flow, end to end: `sync.super-productivity.com`'s root page
  issues the same bearer token used by the app's Settings → Sync →
  SuperSync → Access Token field, and `Authorization: Bearer <token>` is
  confirmed to be the real auth scheme (matches the server's own `app.js`).
- The API routes: `/api/sync/ops`, `/api/sync/restore-points`,
  `/api/sync/status` all confirmed live.
- `GET /api/sync/ops`'s response shape: `{ ops: [...], hasMore, latestSeq,
  ... }`, where each entry is `{ serverSeq, op: {...}, receivedAt }` - **not**
  a flat Operation object as originally assumed. `op` has `opType` (not
  `type`), `entityType` **uppercase** (`"TASK"`, `"GLOBAL_CONFIG"`, `"TAG"`,
  `"TIME_TRACKING"`, `"PLUGIN_USER_DATA"`, `"SIMPLE_COUNTER"`, `"ALL"` for
  `SYNC_IMPORT`), `entityId`, `clientId`, `actionType`, `timestamp`,
  `schemaVersion`, `vectorClock`, and `isPayloadEncrypted` (not
  `encrypted`).
- **The sync log is a Redux action-replay log, not a flat CRUD op log** -
  this was the biggest surprise. For `entityType: "TASK"`, `op.opType`
  (CRT/UPD) doesn't tell you how to interpret the payload; `op.actionType`
  does, and each action type has its own payload shape mirroring the real
  app's NgRx actions (`"[Task Shared] addTask"` → a full task object,
  `"[Task Shared] updateTask"` → `{id, changes}`, `"[Task Shared]
  planTasksForToday"` → `{taskIds, today}`, and so on). `task-store.js`
  handles the action types observed across one real account's *entire*
  history; anything else is a documented no-op, not a crash.
- `SYNC_IMPORT`/`BACKUP_IMPORT`/`REPAIR` carry a full NgRx `EntityState`
  snapshot per feature slice (`payload.task = { ids: [...], entities: {}
  }`) - this is what fills in tasks created before the visible op history
  begins.
- `GET /api/sync/restore/:serverSeq` (the snapshot-bootstrap optimization
  for a fresh watch): confirmed to **always fail with 400
  `ENCRYPTED_OPS_NOT_SUPPORTED`** for E2EE accounts - "Server-side snapshot
  is unavailable because operations are end-to-end encrypted. Use the
  client app's 'Sync Now' button to decrypt and restore locally." Since
  SuperSync is built around E2EE, `doSync()` treats this specific error as
  expected and falls straight through to a full `GET /api/sync/ops` replay
  from `sinceSeq=0` instead - exactly what the server's error says to do.
- **The upload direction needed the same action-replay shape as the
  download direction, and originally didn't have it.** `index.js`'s
  watch→phone toggle handler used to upload a flat `{ isDone }` payload -
  a leftover from before the op log was known to be a Redux action-replay
  log rather than flat CRUD. That payload isn't decodable by
  `applyTaskAction()`'s `"[Task Shared] updateTask"` handler (which reads
  `actionPayload.task` as an `{ id, changes }` `Update<Task>`, matching
  `task-shared.actions.ts`), so a watch-originated toggle silently failed to
  round-trip on the next sync - sync only actually worked downloadward.
  Fixed to upload the same `{ actionPayload: { task: { id, changes } } }`
  shape the server sends back down; covered by a
  `scripts/test-task-store.js` case that round-trips this exact upload
  shape through `applyOperations()`, plus a manual encrypt/decrypt check
  through `createCrypto()`.
- **`downloadOps`'s `excludeClient` query param was an unverified
  assumption, and index.js no longer sends it.** Every other route/field in
  this file was checked against real traffic; this one wasn't - it was
  meant purely as an optimization to avoid re-downloading this client's own
  already-applied uploads (which `saveLastSeq()` after each upload already
  makes unnecessary anyway, by advancing past them). A completed-task
  change made on another device not coming back down on the next sync
  matches exactly the kind of bug a wrong assumption about that param's
  real filtering semantics would cause. Removing it is safe either way:
  `task-store.js`'s merges are idempotent, so re-downloading and
  re-applying this client's own ops (on the rare occasion `saveLastSeq`
  hasn't already skipped past them) is a no-op, not a correctness risk.
  Verified in the emulator end-to-end: a task marked done by a different
  `clientId`, via a plain `"[Task Shared] updateTask"` op the same as any
  other client would send, now reliably shows up as "Done" on the watch
  after a resync.

**Known gaps, not "assumed" so much as "not yet built":**
- Handled TASK action types, confirmed against
  `root-store/meta/task-shared.actions.ts` and its meta-reducers (scheduling,
  short-syntax, crud) in the super-productivity GitHub repo: `addTask`,
  `updateTask`, `updateTasks`, `scheduleTaskWithTime`,
  `reScheduleTaskWithTime`, `planTasksForToday`, `unscheduleTask`,
  `deleteTask`, `deleteTasks`, `moveToArchive`, `restoreTask`,
  `restoreDeletedTask`, `applyShortSyntax` (typing scheduling info directly
  into a task's title), `convertToMainTask`/`convertToSubTask` (subtask
  promotion/demotion - `convertToMainTask` clearing `parentId` is what makes
  a promoted subtask visible as a main task at all). Everything else in
  that file (`moveToOtherProject`, `setDeadline`/deadline-related actions,
  `addTagToTask`, ...) is a no-op - the task keeps whatever state it had
  before that action, which is usually harmless (most of those don't touch
  title/isDone/backlog-membership) but not guaranteed to be.
- **The active-task filter is backlog membership, not a due date** - this
  changed from "only tasks due today" in an earlier version, per explicit
  direction, once it turned out "due today" and "not in the backlog" are
  two genuinely different, independently-tracked concepts in the real app
  (confirmed via `project.model.ts`: `backlogTaskIds` lives on the
  **project** entity, separate from the task's own `dueDay`/`dueWithTime`).
  `task.__inBacklog` is a synthetic flag this project maintains itself -
  seeded from each project's `backlogTaskIds` at `SYNC_IMPORT`, and kept
  current via `addTask`'s `isAddToBacklog`,
  `scheduleTaskWithTime`/`applyShortSyntax`'s `isMoveToBacklog`, and the two
  *membership*-changing actions in `project.actions.ts`'s "MOVE TASK
  ACTIONS" section (`"[Project] ... Move Task from regular to backlog"` and
  its inverse). The several backlog *reorder* actions there
  (`moveProjectTask{Up,Down,ToTop,ToBottom,In}BacklogList`) only change
  position within the backlog, not membership, and are deliberately
  untouched.
- Subtasks (`task.parentId` set) are never selected independently - a
  subtask shows up only by riding along with its own (active) parent's
  `subTaskIds`, prefixed with `    - ` (plain ASCII, not a Unicode arrow/
  bullet - not guaranteed to exist in Pebble's system fonts on every
  platform this targets), regardless of the subtask's own
  `isDone`/backlog status.
- **Grouping by project** (a Settings toggle on the pairing page) sorts
  main tasks by project title before sending them to the watch, tagging
  every row (including subtask rows, which inherit their parent's tag)
  with the group name; tasks with no `projectId` land in a "No Project"
  group. The watch (`main.c`) detects group boundaries as runs of equal
  consecutive `TASK_PROJECT` values - when grouping is off, every row gets
  `''`, which always collapses to one run covering the whole list, so the
  off case is a special case of the same code path, not a separate one.
- **Every task row shares one custom draw path** in `menu_draw_row`, instead
  of only the ones that need to scroll or show "Done" going through it and
  everything else falling back to `menu_cell_basic_draw` - that split used
  to mean a plain row's title rendered in `menu_cell_basic_draw`'s own
  (larger) default title font, visibly different in size from a
  scrolling/done title drawn manually with `TITLE_FONT_KEY`. One font for
  every row fixes that. The custom path also has to pick its own
  background/text colors from the row's selection state (black background/
  white text selected, white/black unselected) to match the platform's own
  invert-on-select cell style exactly, since it no longer gets that for
  free from `menu_cell_basic_draw` - confirmed in the emulator that
  skipping this fill leaves stale (black) framebuffer content behind the
  text.
- **Marquee title scrolling**: when the selected row's title doesn't fit,
  it's drawn twice back-to-back at a scrolling x-offset (an `AppTimer`
  ticking every 300ms) so the full title is eventually readable instead of
  permanently ellipsis-truncated; `selection_changed` resets the offset and
  starts/stops the timer as selection moves on/off a too-wide row. Pebble
  SDK 4.33.1 has no public `graphics_context_set_clip_rect`-style API, so
  the "don't bleed into neighboring rows" guarantee comes entirely from
  `menu_draw_row`'s own `cell_layer` bounds, the same as every other row,
  not from any manual clip call.
- **Centered "Done" status**: a done task's "Done" label is drawn centered
  under its title - `menu_cell_basic_draw`'s subtitle is always
  left-aligned, with no way to override that, which is the other reason
  every row needs the shared custom draw path above.
- **Resync row and project headers are colored** to read as distinct UI
  elements rather than just more list rows: Resync is a standing red
  banner with black text (background stays red regardless of selection, so
  it can't be mistaken for a task), and project group headers are large,
  bold, and blue (text and both underlines). Plain color constants
  (`GColorRed`, `GColorBlue`) - Pebble's SDK degrades these automatically
  on the B&W platforms this app also targets (aplite, diorite), no
  per-platform branching needed.
- **Syncing animation**: the empty/status screen cycles "Syncing" through 0-3
  trailing dots (another `AppTimer`, 400ms) while `s_task_count == 0` and a
  sync is in flight - i.e. only for the very first sync, before any task
  list has ever loaded. A resync with an already-populated list has its own
  live status via the Resync row's subtitle instead, so this doesn't apply
  there.

None of this is guesswork about *how to write a Pebble watchapp* — the
AppMessage/MenuLayer/Storage APIs and the phone-relay networking
architecture are well documented and used exactly as documented. The
uncertainty that's left is narrowly about less-common Super Productivity
action types this session's one test account never happened to exercise.

## Building

```bash
# Rebble's maintained pebble-tool (the original Pebble SDK build servers are gone)
pip install pebble-tool
pebble build
pebble install --phone <phone-ip>   # or --emulator basalt
```

The JS-only logic can be exercised right now, without the SDK, since it's
plain CommonJS:

```bash
node scripts/test-crypto.js
node scripts/test-argon2.js   # includes one ~15s production-parameters case
node scripts/test-task-store.js
```

## Pairing

1. Install the watchapp, open the Pebble mobile app, find "Super
   Productivity" in your watchapps, and tap Settings. This opens the
   pairing page built by `src/pkjs/lib/pairing-page.js` via a `data:` URI —
   nothing to host, since `Pebble.openURL()` has no way to load a file
   bundled inside the `.pbw` and this project has no server of its own.
   `config/pairing.html` is kept as a plain-HTML mirror of the same page for
   reference/local editing; `pairing-page.js` is the copy that actually
   ships.
2. Enter your SuperSync server URL, email, and sync encryption password.
   Tap "Open SuperSync login", sign in there, copy the token it displays,
   paste it into the "SuperSync access token" field. Optionally check
   "Group tasks by project" under Watch display. Then Save.
3. The watch requests a sync automatically on next launch, or immediately
   if it's already open.

## AppMessage protocol

| Direction | `MSG_TYPE` | Purpose | Extra keys |
|---|---|---|---|
| phone→watch | `TASK_SYNC_START` | begin a task list push | `TASK_TOTAL` |
| phone→watch | `TASK_ITEM` | one task | `TASK_INDEX`, `TASK_ID`, `TASK_TITLE`, `TASK_DONE`, `TASK_PROJECT` |
| phone→watch | `TASK_SYNC_END` | list push finished, redraw | — |
| phone→watch | `SYNC_STATUS` | syncing / ok / not-paired / error | `STATUS_CODE`, `STATUS_MSG` |
| watch→phone | `REQUEST_SYNC` | ask phone to sync now | — |
| watch→phone | `TASK_TOGGLE` | user marked a task done/undone | `TASK_ID`, `TASK_DONE` |

## Known gaps for a v1

- **Offline retry queue**: if uploading a task-completion op fails (phone
  offline, server error), it's logged and left for the next full sync to
  reconcile — there's no persisted retry queue yet.
- **Two-way create/delete from the watch**: out of scope for this pass;
  only toggling done/not-done round-trips.
- **Conflict resolution**: relies entirely on SuperSync's own
  operation-log ordering; this client doesn't do any additional merging.
