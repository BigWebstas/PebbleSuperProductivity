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
  SuperSync API (`/api/sync/ops`, `/api/sync/snapshot`,
  `/api/sync/restore-points`, `/api/sync/restore/:serverSeq`) plus the E2EE
  encrypt/decrypt helpers.
- **`src/pkjs/lib/task-store.js`** — replays `CRT`/`UPD`/`DEL`/`MOV`
  operations into an entity cache and picks "today's" tasks out of it.
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

I could not create a real SuperSync account in this environment (it
requires clicking an emailed magic link or a WebAuthn ceremony — both need
a live email inbox / browser interaction I don't have here), so this was
built against the SuperSync server's public route list and architecture
docs, not a captured live request/response. Two layers of this project have
very different confidence levels as a result:

**Solid — actually verified:**
- The crypto primitives (`aes-gcm.js`, `sha256.js`, `base64.js`) are checked
  against Node's built-in `crypto` module for AES-128/256-GCM
  encrypt/decrypt/tag-verification, HMAC-SHA256, PBKDF2, and SHA-256, plus
  base64 round-tripping. Run `node scripts/test-crypto.js`.
- The operation-replay/today-filter logic (`task-store.js`) is unit tested
  in isolation. Run `node scripts/test-task-store.js`.
- The AppMessage protocol between `main.c` and `index.js` is internally
  consistent (same key/enum values on both sides).
- The pairing flow's premise: `sync.super-productivity.com`'s own root page
  confirms it's a login portal ("Connection Successful — copy this token
  and paste it in Super Productivity's sync settings") that issues the same
  bearer token used by the app's Settings → Sync → SuperSync → Access Token
  field. `config/pairing.html` just opens that same page and has you paste
  the token it gives you — no guessed endpoints involved for this part.

**Assumed — needs confirming against a real account before this works
end-to-end**, all flagged in comments at the top of
`src/pkjs/lib/supersync-client.js`:
1. The exact JSON field names on an `Operation` object (`type`,
   `entityType`, `entityId`, `payload`, `encrypted`, ...) — guessed from the
   route/architecture docs, not the literal
   `packages/shared-schema/src/supersync-http-contract.ts` TypeScript
   source.
2. The E2EE key derivation (this uses PBKDF2-HMAC-SHA256, 210,000
   iterations, salt = `"supersync:" + email"`). Getting this wrong doesn't
   corrupt anything — it just means decryption fails with an
   authentication error until the real KDF parameters are dropped in.
3. The wire shape of an encrypted `operation.payload` (assumed:
   `{ iv, ciphertext, tag }`, each base64).
4. What `GET /api/sync/restore/:serverSeq` actually returns (used only for
   the very first sync on a fresh watch).
5. The `Task` entity's field names (`title`, `isDone`, `dueDay`) — the
   `dueDay === today` "today" filter is a reasonable guess based on how
   Super Productivity's UI groups tasks, with a fallback to "everything not
   done" if nothing matches, so the watch doesn't just show an empty list
   if this guess is wrong.
6. ~~Whether Super Productivity exposes a copyable sync token.~~ **Resolved
   — it does.** `sync.super-productivity.com`'s root page is a login portal
   that, after a passkey/magic-link sign-in, displays a bearer token with
   an explicit "copy this token and paste it in Super Productivity's sync
   settings" instruction — the same token the desktop/mobile app uses in
   Settings → Sync → SuperSync → Access Token. `config/pairing.html` opens
   that same page and has you paste the token it gives you; there's no
   OAuth-style `redirect_uri` to auto-capture it, but manual copy/paste of
   a real, already-working token is a solved, working flow, not a
   guess.

None of this is guesswork about *how to write a Pebble watchapp* — the
AppMessage/MenuLayer/Storage APIs and the phone-relay networking
architecture are well documented and used exactly as documented. The
uncertainty is narrowly about SuperSync's private wire format.

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
