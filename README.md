# Pebble Super Productivity

A PebbleOS watchapp that shows today's tasks from [Super
Productivity](https://super-productivity.com) and lets you mark them done
from your wrist, synced through Super Productivity's own **SuperSync**
server.

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

- **`src/c/main.c`** — the watchapp. A `MenuLayer` showing today's tasks;
  select-click toggles done/not-done. Persists the last-synced list via the
  `Storage` API so the list survives an app relaunch even offline.
- **`src/pkjs/index.js`** — the sync engine. Downloads operations from
  SuperSync, replays them into a local task cache, decrypts payloads if
  end-to-end encryption is on, sends the day's tasks to the watch, and
  uploads a task-completion operation when you toggle one on the watch.
- **`src/pkjs/lib/supersync-client.js`** — thin REST client for the
  SuperSync API (`/api/sync/ops`, `/api/sync/restore-points`,
  `/api/sync/restore/:serverSeq`) plus `createCrypto(password)`, which
  builds the real Argon2id/AES-GCM decrypt+encrypt pair (with a per-salt
  key cache, since Argon2id is multi-second at production parameters).
- **`src/pkjs/lib/task-store.js`** — replays SuperSync's op log into an
  entity cache and picks "today's" tasks out of it. This is a Redux
  action-replay log, not a flat CRUD op log - see the file's top comment.
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
- The operation-replay/today-filter logic (`task-store.js`) is unit tested
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

**Known gaps, not "assumed" so much as "not yet built":**
- Only the TASK action types actually observed in one real (fairly
  calendar/JIRA-integration-heavy) account are handled: `addTask`,
  `updateTask`, `scheduleTaskWithTime`, `planTasksForToday`. Notably
  **absent: deletion/archival** (`deleteTask`, `moveToArchive`,
  `restoreTask`, or similar) - a task deleted on another device won't
  disappear from the watch's view, since nothing currently removes it from
  the local cache. A heavier or longer-lived account almost certainly uses
  more action types than this one did.
- The "today" list prefers the app's own `planTasksForToday` action when
  it's fresh (dated today); otherwise it falls back to a
  `dueDay`/`dueWithTime` guess, and finally to "everything not done" if
  neither matches anything - which can mean a full backlog rather than a
  curated list, on an account whose last sync predates that day's planning.

None of this is guesswork about *how to write a Pebble watchapp* — the
AppMessage/MenuLayer/Storage APIs and the phone-relay networking
architecture are well documented and used exactly as documented. The
uncertainty that's left is narrowly about less-common Super Productivity
action types this session's one test account never happened to exercise.

## Building

**Not yet compiled in this environment** — there's no Pebble SDK/ARM
toolchain installed here (`pebble-tool` / `arm-none-eabi-gcc` are both
absent), and installing the real one requires network access to
Pebble/Rebble's SDK distribution, which felt like the wrong thing to do
without checking first. Once you have the toolchain:

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
   paste it into the "SuperSync access token" field, then Save.
3. The watch requests a sync automatically on next launch, or immediately
   if it's already open.

## AppMessage protocol

| Direction | `MSG_TYPE` | Purpose | Extra keys |
|---|---|---|---|
| phone→watch | `TASK_SYNC_START` | begin a task list push | `TASK_TOTAL` |
| phone→watch | `TASK_ITEM` | one task | `TASK_INDEX`, `TASK_ID`, `TASK_TITLE`, `TASK_DONE` |
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
