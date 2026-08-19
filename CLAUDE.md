# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

A PebbleOS watchapp that shows active tasks from [Super Productivity](https://super-productivity.com) on the wrist and lets you mark them done from there, synced through Super Productivity's own **SuperSync** REST API. There is no Pebble-native networking, so the phone is in the loop as a relay:

```
Watch (C, src/c/main.c) <--AppMessage/Bluetooth--> PebbleKit JS (src/pkjs/index.js) <--HTTPS/JSON--> SuperSync server
```

If PebbleOS ever ships watch-native networking, only `src/pkjs/index.js` and friends would need to move to `src/c/`.

## Commands

```bash
# Build (requires pebble-tool + SDK; see "Local environment" below)
pebble build
pebble install --emulator basalt   # or --phone <ip>, or basalt/aplite/chalk/diorite/emery

# JS-only unit tests - no SDK required, plain Node/CommonJS
npm test                        # runs all three below
node scripts/test-crypto.js     # aes-gcm.js/sha256.js/base64.js vs Node's real `crypto` module
node scripts/test-argon2.js     # blake2b.js/argon2id.js vs hash-wasm (~15s: one production-params case)
node scripts/test-task-store.js # op-log replay / active-task-list logic
```

There is no lint config; JS style is plain ES5 CommonJS (`var`, no arrow functions) throughout `src/pkjs/**` — this is deliberate, not an oversight, since the PebbleKit JS runtime's engine varies across phone platforms/app versions and can't be assumed to support newer syntax.

### Local environment gotchas (this machine)

- `pebble-tool` lives in a venv at `/var/tmp/pebble-sdk/pebbletool-venv/bin` (symlinked from `~/.local/share/pebble-sdk`) — put it on `PATH` before running `pebble`.
- After changing `messageKeys` in `package.json`, run `pebble clean` before `pebble build` — the generated `message_keys.auto.h` doesn't always regenerate on an incremental build.
- **Never run `pebble wipe --everything`** — despite sounding like it clears emulator/app storage, it deletes the *entire* local Pebble SDK/tool installation (toolchain, `pebble-tool` itself, login state), not just app data. Plain `pebble install --emulator <platform>` reinstalls the app fine without wiping anything; there's no need to wipe storage between test runs.
- To visually verify a UI change: temporarily seed `s_tasks`/`s_task_count` in `main.c`'s `init()` (guarded by `if (s_task_count == 0)`), build, `pebble install --emulator basalt`, `pebble screenshot`, then revert the seed before committing. `pebble emu-button click {up,down,select,back}` drives selection; screenshots are native resolution (e.g. 144×168 on basalt) — upscale with ImageMagick (`-filter point -resize 600%`) before reading them, nearest-neighbor so pixel text stays legible.

## Architecture

### The op log is a Redux action-replay log, not a flat CRUD log

This is the single most important thing to know before touching sync code. SuperSync's `GET /api/sync/ops` returns entries shaped `{ serverSeq, op, receivedAt }`, and for `entityType: "TASK"`, `op.opType` (CRT/UPD/...) does **not** tell you how to interpret `op.payload` — `op.actionType` does, mirroring the real Super Productivity app's actual NgRx actions (`"[Task Shared] addTask"` → a full task object, `"[Task Shared] updateTask"` → `{ id, changes }`, `"[Task Shared] planTasksForToday"` → `{ taskIds, today }`, etc.). `src/pkjs/lib/task-store.js`'s `applyTaskAction()`/`applyProjectAction()` switch on `actionType`, and any unhandled type is a documented no-op, not a crash — check the real app's `root-store/meta/task-shared.actions.ts` and its meta-reducers before assuming a new action type needs handling, and check the actual reducer logic (not just the action shape) before assuming a fix is complete — see "Cross-checking against the real app" below.

`SYNC_IMPORT`/`BACKUP_IMPORT`/`REPAIR` ops instead carry a full NgRx `EntityState` snapshot per feature slice (`payload.task = { ids: [...], entities: {...} }`) — this is what backfills tasks created before the visible op history begins.

### Vector clocks are mandatory on upload

Every uploaded op needs a `vectorClock` field (`{ clientId: counter, ... }`) or the server rejects the **entire op** before storage (`sanitizeVectorClock()` in the real server's `validation.service.ts`). `index.js` tracks a simplified one in `localStorage` (`sp_vector_clock`): merged from every downloaded op's own `vectorClock`, incremented for this client on every local upload. It doesn't need to be a byte-perfect port of the real client's `VectorClockService` — the server only checks shape/size, not completeness — but it does need to be *causally accurate enough* not to look concurrent with newer server-side state, or the upload gets rejected as a conflict.

**A rejected upload still comes back as HTTP 200** — acceptance is per-op in `res.results[].accepted`, not the HTTP status. Always check `results[].accepted` after an upload; treating any 2xx as success will silently swallow rejections forever (this was a real, previously-shipped bug — see git history around the "surface rejected task-toggle uploads" commit).

### dueDay / dueWithTime are mutually exclusive

The real app enforces this (`task-shared-scheduling.reducer.ts`): setting `dueWithTime` clears `dueDay`, and vice versa. Anywhere date-matching logic needs "is this task today" (see `taskIsPlannedForToday()` in `task-store.js`), check `dueWithTime` *first* and only fall back to `dueDay` if it's unset — not an unconditional OR — to correctly handle legacy data that predates this mutual exclusivity and can carry both fields.

The `todayOnly` filter also has to consider a main task's **subtasks'** own due dates, not just the main task's — the real selector (`computeOrderedTaskIdsForToday` in `work-context.selectors.ts`) evaluates every task/subtask independently. This app always nests subtasks under their parent (see `pushTaskAndSubtasks`), so a main task with no due date of its own but a subtask due today still has to qualify, or the subtask has nowhere to nest.

### Component map

- **`src/c/main.c`** — the watchapp UI. A sectioned `MenuLayer`: a red "Resync" row pinned to section 0 (status/subtitle reflects live sync state), then either one section per project group or a single unheaded section. Every task row shares one custom draw path (`menu_draw_row`, not `menu_cell_basic_draw`) so title size/background/selection-color are consistent, since Pebble's default cell drawing can't do a centered "Done" subtitle or match custom colors. `get_cell_height` gives a done task's row extra height so its "Done" label isn't cramped against the title. A too-wide selected title marquee-scrolls (`AppTimer`, 300ms tick); a too-long title is otherwise ellipsis-truncated to one line, never wrapped (the title's text box height is deliberately constrained to one measured line). Persists the last-synced list via the `Storage` API.
- **`src/pkjs/index.js`** — the sync engine: `doSync()` (download+replay+push-to-watch), `handleTaskToggle()` (upload a watch-side completion), pairing/config persistence (`localStorage`: `sp_config`, `sp_password`, `sp_entities`, `sp_last_seq`, `sp_vector_clock`, `sp_client_id`).
- **`src/pkjs/lib/supersync-client.js`** — REST client (`/api/sync/ops`, `/api/sync/restore-points`, `/api/sync/restore/:serverSeq`) plus `createCrypto(password)` (Argon2id → AES-GCM encrypt/decrypt, with a per-salt key cache since Argon2id is multi-second at production params). Wire format: `[16-byte salt][12-byte IV][AES-GCM ciphertext+tag]`, base64, with a legacy PBKDF2 fallback for short/old ciphertexts.
- **`src/pkjs/lib/task-store.js`** — replays the op log into an entity cache (`{ task: {...}, project: {...} }`) and derives the watch's active task list from it (`getActiveTasks`): excludes backlog tasks, optionally groups by project, optionally filters to today (`todayOnly`).
- **`src/pkjs/lib/blake2b.js`, `argon2id.js`** — dependency-free BLAKE2b / Argon2id (RFC 9106), `parallelism=1` hardcoded. 64-bit words are `[hi, lo]` number pairs, not `BigInt` — the build toolchain's JS bundler can't parse `BigInt` literal syntax.
- **`src/pkjs/lib/aes-gcm.js`, `sha256.js`, `base64.js`** — dependency-free AES-128/256-GCM, SHA-256/HMAC/PBKDF2, base64, since PebbleKit JS doesn't reliably expose `window.crypto.subtle` across phone platforms.
- **`src/pkjs/lib/pairing-page.js`** — the phone-side settings page, inlined as a `data:` URI (`Pebble.openURL()` can't load a file bundled inside the `.pbw`, and this project hosts nothing of its own). `config/pairing.html` is a plain-HTML mirror kept for reference/local editing only — `pairing-page.js` is what actually ships. Settings: group-by-project, today-only, sync-on-complete, plus a "clear all local data & resync" action.

### AppMessage protocol (`main.c` ⟷ `index.js`)

| Direction | `MSG_TYPE` | Purpose | Extra keys |
|---|---|---|---|
| phone→watch | `TASK_SYNC_START` | begin a task list push | `TASK_TOTAL` |
| phone→watch | `TASK_ITEM` | one task | `TASK_INDEX`, `TASK_ID`, `TASK_TITLE`, `TASK_DONE`, `TASK_PROJECT`, `TASK_DUE_MIN` (minutes since local midnight; omitted when the task has no `dueWithTime`) |
| phone→watch | `TASK_SYNC_END` | list push finished, redraw | — |
| phone→watch | `SYNC_STATUS` | syncing / ok / not-paired / error | `STATUS_CODE`, `STATUS_MSG` |
| watch→phone | `REQUEST_SYNC` | ask phone to sync now | — |
| watch→phone | `TASK_TOGGLE` | user marked a task done/undone | `TASK_ID`, `TASK_DONE` |

Message keys are declared in `package.json`'s `pebble.messageKeys` array (order gives each one its runtime ID) and referenced in C via `MESSAGE_KEY_*` externs, not hand-picked integers — `#define KEY_TASK_ID MESSAGE_KEY_TASK_ID` etc. at the top of `main.c`.

## Cross-checking against the real app

There's no way to create a real SuperSync account from this environment (needs a live email/WebAuthn ceremony), so when in doubt about wire format, action shapes, reducer behavior, or UI semantics ("what does the real Today view actually show"), check the actual `super-productivity/super-productivity` source rather than guessing — a full clone (including `packages/super-sync-server`, the sync server's own source) is available as a sibling checkout, typically at `../super-productivity` relative to this repo. Grep for the relevant action name in `src/app/root-store/meta/task-shared.actions.ts` and its meta-reducers, or the relevant selector in `src/app/features/*/store/*.selectors.ts`, before assuming behavior. Several real bugs in this project were only found this way (missing `vectorClock` on upload, a stale/flat upload payload shape, `todayOnly` not checking subtask due dates) — reading the actual reducer/selector code caught things that reading only the action *shape* or API docs missed. `README.md`'s "What is verified vs. assumed" section has the fuller history of what's been confirmed this way vs. still inferred.

## Testing philosophy

The crypto and hashing primitives are cross-verified against independent reference implementations, not just internally self-consistent: `aes-gcm.js` against Node's real `crypto` module (byte-for-byte ciphertext/tag match, plus bidirectional interop — Node decrypting our output and vice versa), `blake2b.js`/`argon2id.js` against `hash-wasm` (the library the real Super Productivity client uses). When adding crypto-adjacent code, prefer extending these cross-checks over writing round-trip-only tests, which can't catch a bug that's wrong in a self-consistent way.
