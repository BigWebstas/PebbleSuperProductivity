# Live tracking presence — watch integration spec

Bringing Super Productivity's **live tracking presence** (desktop v18.21.1,
[PR #9771](https://github.com/super-productivity/super-productivity/pull/9771))
to the Pebble watchapp.

> **Status: Phase 1 built** (branch `live-tracking-presence`), not yet tested
> against a live account. Deviation from §7 below: the "LIVE" entry is a **row
> in section 0** (above Resync), not a new menu section — far less perturbation
> of the menu's section-index arithmetic. The detail window + remote Stop are
> as specced. Files touched: `src/pkjs/lib/presence-client.js` (new),
> `scripts/test-presence.js` (new), `src/pkjs/lib/supersync-client.js`
> (`canDecryptWithoutDerive`), `src/pkjs/index.js`, `src/pkjs/lib/pairing-page.js`,
> `src/c/main.c`, `package.json`. All 5 platforms build; `npm test` green.

## 1. What the desktop feature is

An **ephemeral** cross-device signal, opt-in, SuperSync only. When a device
starts time-tracking a task it broadcasts a small state message. Other devices
show a header chip — "Tracking on Desktop · <task> · since 10:14" — with a
**Stop** button that ends the session remotely.

Key facts that shape our design:

- It rides the **existing SuperSync WebSocket**, not the op-log. New message
  types `presence_state` / `presence_cmd`.
- The server keeps **one opaque in-memory slot per user**. Nothing is parsed
  or persisted server-side. Nothing touches the op-log.
- Payloads are **E2E-encrypted with the sync key** when encryption is on, using
  the same scheme as op payloads (`[16-byte salt][12-byte IV][AES-GCM ct+tag]`,
  base64). Receivers **fail closed** — reject a plaintext envelope when a key
  is configured.
- Time accounting is unchanged. It still flows through the `syncTimeSpent` op
  path (`task-store.js` `[TimeTracking] Sync time spent`). Presence is
  **display + remote-stop only**.

## 2. Scope

**Phase 1 — viewer + remote stop (recommended first ship).**
The watch shows what another device is tracking and can stop it. This is the
high-value slice: "I left the timer running on my desktop, kill it from my
wrist."

**Phase 2 — broadcaster (optional, later).**
When you time-track from the watch, the watch emits `presence_state` so the
desktop shows "Tracking on Pebble". Lower value — the watch's tracking already
round-trips via the op-log — and it needs producer-side session/heartbeat
bookkeeping. Deferred. This spec covers Phase 1; Phase 2 notes are in §9.

## 3. Hard constraint: no background worker

PebbleKit JS runs **only while the watchapp is open** (`CLAUDE.md`: "No
background worker"). Consequences:

- The presence screen is live only during an app session (~1–2 min typical).
- No ambient "buzz when the desktop starts tracking". This is a **glance**
  feature, not a notification feature. Say so in the settings copy.
- The WS connects on app open and closes when PKJS is torn down. Accept the
  per-open connection cost; make it opt-in so users who don't want it pay
  nothing.

## 4. Wire protocol (as of PR #9771)

### 4.1 Connection

```
wss://sync.super-productivity.com/api/sync/ws?token=<accessToken>&clientId=<clientId>
```

- `baseUrl` → `wss://` via `https://` → `wss://` swap. Watch already stores the
  base URL, the bearer `token`, and `sp_client_id` (localStorage, `index.js`).
- Server pushes `{type:"connected"}` on open.
- Server pings `{type:"ping"}` ~every 30s; reply `{type:"pong"}`. If no server
  traffic for **45s**, treat the socket as dead and reconnect.
- Reconnect: exponential backoff 1s → 60s, jitter, cap ~50 attempts.
  **Do not reconnect** on close codes `4003` (auth failure / token revoked),
  `4008` (per-user connection limit), `4009` (replaced by newer socket sharing
  this clientId). On `4009` just stop — a later app open re-establishes.

### 4.2 Messages — server → client

```jsonc
// another device's tracking state (cached, replayed to new sockets)
{ "type": "presence_state",
  "payload": "<envelope string>",
  "ordinal": 7,                 // server-assigned, monotonic per user
  "producerConnected": true }   // false once the producing socket is gone

// a relayed command (e.g. a stop request from another viewer). Not cached.
{ "type": "presence_cmd", "payload": "<envelope string>" }
```

On connect, if another device has a fresh slot (`< 30 min` old), the server
immediately sends its `presence_state` so we render without waiting for the
next heartbeat.

### 4.3 Messages — client → server

```jsonc
{ "type": "presence_cmd", "payload": "<envelope string>" }   // Phase 1: stop only
{ "type": "presence_state", "payload": "<envelope string>" } // Phase 2 only
```

Server limits: payload ≤ **8192 bytes**, ≤ **15 messages / 10s** per socket.
We send far under both.

### 4.4 Envelope

`payload` is `JSON.stringify` of:

```jsonc
{ "enc": true,  "data": "<base64: salt|iv|ciphertext|tag>" }   // E2EE on
{ "enc": false, "data": "<JSON string of the payload>" }        // E2EE off
```

Decode: parse envelope → if `enc` decrypt `data` with the sync key, else
`JSON.parse(data)`. **If the account has a key but `enc` is false, drop the
message** (hostile-server guard).

### 4.5 `presence_state` payload

```jsonc
{
  "v": 1,
  "sessionId": "<nanoid>",   // new per tracking session (task switch = new one)
  "seq": 12,                  // producer-monotonic; drop lower seq for same session
  "state": "tracking" | "stopped",
  "reason": "idle",          // optional; only on "stopped" — producer is idle-paused
  "taskId": "<id>" | null,
  "sinceTs": 1717000000000,  // wall-clock ms of session start. DISPLAY ONLY.
  "deviceLabel": "Desktop",  // free string; sanitize before display
  "focusCycle": 3            // optional; focus-mode cycle number, read-only
}
```

Render mapping:

| Condition | Watch shows | Stop button |
|---|---|---|
| `state=tracking`, producer connected, fresh | "Tracking on <device>" + task + elapsed | yes |
| `state=stopped` + `reason=idle` | "Paused on <device>" + task | no |
| `state=stopped`, no reason | linger ~10s then clear | no |
| `!producerConnected` or no update for 90s | "Was tracking on <device>" + last-seen | no |

"fresh" = `now - receivedAt < 90s` **and** `producerConnected`.

### 4.6 `presence_cmd` payload (stop)

```jsonc
{ "v": 1, "cmd": "stop", "sessionId": "<the session we're viewing>", "deviceLabel": "Pebble" }
```

CAS-guarded: the producer ignores the command if `sessionId` no longer matches
what it's tracking. **Do not** clear our UI optimistically — wait for the
producer's `state:"stopped"` broadcast (its ack).

### 4.7 Ordering / dedupe (viewer)

1. Drop if `ordinal < lastOrdinal`. Equal ordinals are re-announcements
   (the `producerConnected` flag flipped) — let them through.
2. For the same `sessionId`, drop if `seq < lastSeq`.
3. Validate: `v===1`, `sessionId` string, `state` in set, `seq` finite,
   `sinceTs` finite, `taskId` string-or-null. Reject otherwise.
4. Sanitize `deviceLabel`: strip `< > & " ' \``, cap 32 chars.

## 5. New PKJS module: `src/pkjs/lib/presence-client.js`

Pure transport + codec. No AppMessage, no task lookup — `index.js` wires those.

```js
// PresenceClient — SuperSync live-tracking presence over the sync WebSocket.
// Phase 1: viewer + remote stop. Transport only; decoding uses the same
// crypto object supersync-client.js builds for op payloads.
//
// Wire contract mirrored from super-productivity PR #9771:
//   super-sync-websocket.service.ts, websocket-connection.service.ts,
//   tracking-presence.service.ts, tracking-presence.model.ts
// This is UNVERIFIED against a live account — see supersync-client.js's
// header for the same caveat.

function PresenceClient(opts) {
  // opts: { baseUrl, token, clientId, crypto|null, log }
  //   crypto: object with .encrypt(obj)->b64 and .decrypt(b64)->obj, or null
  //           when the account is not E2EE. Reuse supersync-client's instance.
}

PresenceClient.prototype.connect = function () {};      // idempotent; starts reconnect loop
PresenceClient.prototype.disconnect = function () {};   // intentional close, no reconnect
PresenceClient.prototype.isConnected = function () {};  // bool

// Fired for every accepted remote state (after dedupe + validation + decode):
//   { state, reason, taskId, sinceTs, deviceLabel, sessionId, seq,
//     producerConnected, ordinal, receivedAt }
PresenceClient.prototype.onState = function (cb) {};

// Fired when the slot should be considered cleared (linger elapsed, or an
// explicit stopped-no-reason after its linger):
PresenceClient.prototype.onCleared = function (cb) {};

// Phase 1 remote stop. Builds+encrypts the cmd envelope and sends it.
// No-op if not connected (caller may retry on next onState).
PresenceClient.prototype.requestStop = function (sessionId) {};
```

Internal responsibilities:

- **Socket lifecycle**: `WebSocket` (supported in PebbleKit JS on both iOS and
  Android), `onopen/onmessage/onclose/onerror`, app-level ping/pong, 45s
  liveness timer, backoff reconnect honouring the no-reconnect close codes.
- **Message routing**: ignore `new_ops` (op sync stays on the HTTP path),
  answer `ping`, handle `presence_state` / `presence_cmd`.
- **Decode** (§4.4): fail closed when `crypto` is set and `enc` is false.
- **Dedupe / validate / sanitize** (§4.7).
- **Linger timers**: `stopped` with no reason → hold 10s then `onCleared`.
  `reason:"idle"` stays until superseded.
- **Staleness**: a 30s internal tick re-checks `now - receivedAt > 90s` and
  re-emits `onState` so `index.js` can flip the watch to "Was tracking".

### 5.1 Crypto reuse and the Argon2 hazard

`supersync-client.js` `createCrypto()` already produces `.encrypt(obj)` /
`.decrypt(b64)` in exactly this envelope format, with a per-salt Argon2id key
cache persisted to `localStorage` (`sp_kdf_keys`). Pass that same instance in.

**Hazard**: a `presence_state` from another device is encrypted under *that
device's* session salt. If it isn't already in `sp_kdf_keys`, `.decrypt()`
blocks on a full Argon2id derivation — **minutes** on phone hardware — on the
message-handling path.

In practice the desktop encrypts presence under the same salt as its ops, which
the watch derived during its last sync, so it's usually a cache hit. Mitigation:

- Before calling `.decrypt()`, check the salt prefix (first 16 bytes of the
  decoded `data`) against the cache. On a miss: emit an `onState` with a
  `pendingDecrypt:true` flag so the watch can show "Live (decrypting…)", and
  do the derive on a `setTimeout(…, 0)` so the socket keeps pumping.
- Never derive synchronously inside `onmessage`.

## 6. `index.js` integration

### 6.1 New AppMessage messages

```js
var MSG_PRESENCE_UPDATE = 27; // phone -> watch: PRESENCE_* keys below
var MSG_PRESENCE_CLEAR  = 28; // phone -> watch: no keys — hide the Live UI
var MSG_PRESENCE_STOP   = 29; // watch -> phone: no keys — request remote stop
```

New `messageKeys` (append to `package.json`):

```
PRESENCE_STATE        // uint8: 0 none, 1 tracking, 2 paused, 3 was-tracking,
                      //        4 stopped (brief linger, mirrors the desktop chip)
PRESENCE_TASK_TITLE   // string, pre-resolved & truncated by the phone
PRESENCE_DEVICE       // string, sanitized deviceLabel
PRESENCE_ELAPSED_S    // uint32: seconds since sinceTs at send time
PRESENCE_CAN_STOP     // uint8: 1 when a remote stop is currently valid
```

The phone resolves `taskId` → title from the task-store state it already holds
(`store` in `index.js`); fall back to `"a recently started task"` (matches the
desktop `FALLBACK_TASK` string) when the task isn't in the active set.
`PRESENCE_ELAPSED_S` is a snapshot; the watch ticks locally from receipt.

### 6.2 Lifecycle

- Gate on a new setting `s_live_tracking_enabled` (default **off**, opt-in) and
  on the account being SuperSync. Read platform via
  `Pebble.getActiveWatchInfo().platform` if we want to skip `aplite` (see §8).
- On PKJS ready with the gate on: build `PresenceClient` with the existing
  `crypto`, `token`, `baseUrl`, `sp_client_id`; `connect()`.
- `onState`: map to `PRESENCE_STATE`, send `MSG_PRESENCE_UPDATE` (reuse
  `sendWithRetry`). Remember `sessionId` for the stop path.
- `onCleared` / state 0: send `MSG_PRESENCE_CLEAR`.
- `MSG_PRESENCE_STOP` from watch → `presenceClient.requestStop(lastSessionId)`.
  UI clears only when the producer's `stopped` arrives as a normal `onState`.
- On settings change turning the gate off: `disconnect()` + `MSG_PRESENCE_CLEAR`.

### 6.3 Config page

Add one checkbox to `lib/pairing-page.js`: **"Show live tracking from other
devices"**, with a line of help text: *"While the watchapp is open, shows what
you're tracking on your desktop or phone and lets you stop it. SuperSync only."*
Mirror it to the watch the same way `TOUCH_NAV_ENABLED` etc. are (optional
field on `MSG_SYNC_STATUS`).

## 7. C screen: `src/c/main.c`

### 7.1 Entry point

Follow the existing **pinned action row** pattern (the `s_pin_tracked_task`
section already inserts a synthetic row at the top of the task list). When
`PRESENCE_STATE != 0`, insert one row at the very top of the main menu:

```
◆ LIVE
  Desktop · Write spec        0:42
```

Selecting it pushes `s_live_window`.

### 7.2 `s_live_window`

- `Window` + a single custom `Layer` (draw) + `ActionBarLayer`.
- Body, three stacked text rows:
  1. State line — "Tracking on Desktop" / "Paused on Desktop" /
     "Was tracking on Desktop" (`PRESENCE_STATE` → string; `PBL_IF_ROUND`
     centering like the rest of the app).
  2. Task title (`PRESENCE_TASK_TITLE`), `GOTHIC_24_BOLD`, 2 lines,
     ellipsized.
  3. Elapsed `H:MM:SS` when tracking; "since HH:MM" style is fine too. Hidden
     for paused / was-tracking.
- Action bar: SELECT icon = stop (■). Shown only when `PRESENCE_CAN_STOP == 1`.
  On click → send `MSG_PRESENCE_STOP`, show a brief "Stopping…" state, disable
  the button. Do **not** pop the window; let the next `MSG_PRESENCE_UPDATE`
  (state 0 / cleared) pop it.
- BACK = pop (normal).

### 7.3 Elapsed ticking

While `s_live_window` is on top and state is tracking: `app_timer` re-armed
every 1000 ms, recomputing from a stored base:

```c
static time_t s_presence_elapsed_base;   // = now - PRESENCE_ELAPSED_S at receipt
// each tick: elapsed = time(NULL) - s_presence_elapsed_base; redraw line 3
```

Cancel the timer on `window_unload` and when state leaves tracking. Clamp
negative / absurd values (guard like the existing `TASK_TIME_SPENT_MS`
overflow handling around `main.c:468`).

### 7.4 Inbox handler

Add to `inbox_received_handler` (`main.c:2639`):

- `MSG_PRESENCE_UPDATE`: stash the fields in statics
  (`s_presence_state`, `s_presence_task[…]`, `s_presence_device[…]`,
  `s_presence_can_stop`, `s_presence_elapsed_base`); refresh the pinned row and,
  if open, `s_live_window`.
- `MSG_PRESENCE_CLEAR`: zero the statics; remove the pinned row; pop
  `s_live_window` if it's on top.

Static footprint: ~2 small char buffers (`s_presence_task[64]`,
`s_presence_device[24]`) + 3 bytes of state. Trivial.

## 8. `aplite`

The heavy cost (WebSocket, Argon2, crypto) is **all phone-side** — the watch
just gains one window and two buffers. So it can ship on every platform.
If RAM proves tight on `aplite` in practice, gate it there the way Finish Day
and pin-tracked-task are (compile-time `PBL_PLATFORM_APLITE` / the phone
skipping the feature for that platform).

## 9. Phase 2 notes (broadcaster — not now)

To make the watch show up as "Tracking on Pebble":

- On watch track-start (the existing time-tracking action), have `index.js`
  mint a `sessionId` (nanoid-ish), set `sinceTs`, send `presence_state`
  `{state:"tracking", deviceLabel:"Pebble", …}`.
- Heartbeat every 60s while connected (server staleness window is 90s).
- On track-stop, send `{state:"stopped"}`. On app close the socket drops; the
  server flips `producerConnected:false` and viewers decay to "Was tracking on
  Pebble" then hide after 30 min — acceptable.
- Handle inbound `presence_cmd` stop: match `sessionId`, stop tracking, ack
  with a `stopped` broadcast.
- Respect the single-active-session takeover rules
  (`_resolveTakeover` in the PR): later `sinceTs` wins, `sessionId` breaks ties.

## 10. Testing

- **JS unit tests** (`npm test`, node): new `scripts/test-presence.js` —
  envelope encode/decode round-trip against `createCrypto`, fail-closed on
  plaintext-with-key, ordinal/seq dedupe, linger + staleness timers (fake
  timers), `requestStop` envelope shape. This is the only automated coverage
  the repo supports.
- **Manual, emulator**: stub a local WS echoing canned `presence_state`
  frames; verify the pinned row, `s_live_window`, elapsed tick, stop flow.
- **Manual, hardware + real desktop** (`pebble install --phone 192.168.1.209`):
  start a timer on the desktop, confirm the watch shows it within a few
  seconds of opening the app; press Stop; confirm the desktop timer stops and
  the watch UI clears on the ack. Test with E2EE on and off.
- Watch for the **Argon2 hazard** (§5.1): first run after a desktop
  password/salt change.

## 11. Effort estimate

| Piece | Rough size |
|---|---|
| `presence-client.js` + tests | ~350 lines JS + ~200 test |
| `index.js` wiring + config checkbox | ~120 lines |
| `main.c` pinned row + `s_live_window` + inbox | ~200 lines C |
| `package.json` keys, docs, manual test passes | small |

Protocol is **experimental** (desktop v18.21.1) and **unverified against a live
account** from this client — expect envelope/field churn and keep the
mirrored-from-PR header comments accurate, exactly as `supersync-client.js`
already does for the HTTP routes.
