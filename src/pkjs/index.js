// PebbleKit JS component: runs inside the Pebble mobile app and is the only
// part of this project with real internet access (see README.md for why -
// the Pebble C SDK has no networking API of its own). Bridges AppMessage
// to/from the watch and the SuperSync REST API.
'use strict';

var supersync = require('./lib/supersync-client.js');
var store = require('./lib/task-store.js');
var pairingPage = require('./lib/pairing-page.js');

// Keep in sync with the enums at the top of src/c/main.c.
var MSG_TASK_SYNC_START = 1;
var MSG_TASK_ITEM = 2;
var MSG_TASK_SYNC_END = 3;
var MSG_SYNC_STATUS = 4;
var MSG_REQUEST_SYNC = 5;
var MSG_TASK_TOGGLE = 6;
var MSG_TRACK_TIME_STOP = 7;

var STATUS_OK = 0;
var STATUS_SYNCING = 1;
var STATUS_NOT_PAIRED = 2;
var STATUS_ERROR = 3;

var MAX_TASKS = 30;

// Matches the real client's CURRENT_SCHEMA_VERSION (schema-version.ts) at
// time of writing. The server only validates this field's range when it's
// present (validation.service.ts treats a missing schemaVersion as "skip the
// check"), so omitting it isn't what was breaking uploads - but every op the
// real app writes carries it, and the field is required (non-optional,
// non-nullable) in both the shared request schema and the operations table,
// so it belongs on outgoing ops for correctness regardless.
var SCHEMA_VERSION = 4;

// ---------------- local storage helpers ----------------

function loadConfig() {
  try {
    return JSON.parse(localStorage.getItem('sp_config') || 'null');
  } catch (e) {
    return null;
  }
}

function saveConfig(config) {
  localStorage.setItem('sp_config', JSON.stringify(config));
}

function loadState() {
  try {
    return JSON.parse(localStorage.getItem('sp_entities') || 'null') || store.emptyState();
  } catch (e) {
    return store.emptyState();
  }
}

function saveState(state) {
  localStorage.setItem('sp_entities', JSON.stringify(state));
}

function loadLastSeq() {
  var v = localStorage.getItem('sp_last_seq');
  return v ? parseInt(v, 10) : 0;
}

function saveLastSeq(seq) {
  localStorage.setItem('sp_last_seq', String(seq));
}

function getOrCreateClientId() {
  var id = localStorage.getItem('sp_client_id');
  if (!id) {
    id = 'pebble-' + Date.now().toString(36) + '-' + Math.floor(Math.random() * 1e9).toString(36);
    localStorage.setItem('sp_client_id', id);
  }
  return id;
}

// A vector clock ({ clientId: counter, ... }) is REQUIRED on every uploaded
// op - confirmed by reading the real server's source
// (packages/super-sync-server/src/sync/services/validation.service.ts's
// sanitizeVectorClock(), called from validateOp()): a missing/non-object
// vectorClock fails validation outright (INVALID_VECTOR_CLOCK) and the
// whole op upload is rejected, never stored, so no other client - including
// the real desktop app - ever sees it. This project's watch-toggle upload
// didn't send one at all until this was found; see handleTaskToggle().
//
// Tracked locally (not recomputed from scratch each time) so an upload's
// clock reflects both our own prior increments and whatever other clients'
// components we've observed in downloaded ops - mirroring (in simplified
// form) VectorClockService.getCurrentVectorClock()/incrementVectorClock()
// in the real client's src/app/core/util/vector-clock.ts. A perfectly
// pruned/merged clock isn't required for the server to ACCEPT the op
// (sanitizeVectorClock only checks shape/size, not completeness) - just a
// valid plain object - but keeping ours reasonably accurate avoids
// needlessly flagging every one of our uploads as CONCURRENT with
// everything else during the server's conflict comparison.
function loadVectorClock() {
  try {
    return JSON.parse(localStorage.getItem('sp_vector_clock') || 'null') || {};
  } catch (e) {
    return {};
  }
}

function saveVectorClock(clock) {
  localStorage.setItem('sp_vector_clock', JSON.stringify(clock));
}

function mergeVectorClocks(a, b) {
  var merged = Object.assign({}, a);
  Object.keys(b || {}).forEach(function (id) {
    merged[id] = Math.max(merged[id] || 0, b[id] || 0);
  });
  return merged;
}

function incrementVectorClock(clock, clientId) {
  var next = Object.assign({}, clock);
  next[clientId] = (next[clientId] || 0) + 1;
  return next;
}

// The Argon2id KDF is multi-second at production parameters (see
// argon2id.js), and its own derived keys are cached per-salt inside the
// crypto object - but that cache is worthless if we throw the whole object
// away and rebuild it on every doSync() call, re-deriving the same salt's
// key every sync. Cache the crypto object itself at module scope instead,
// for the lifetime of this pkjs session, invalidating only when the
// password actually changes (re-pairing).
var cachedCrypto = null;
var cachedPassword = null;

function getCrypto() {
  var password = localStorage.getItem('sp_password');
  if (!password) {
    return null;
  }
  if (!cachedCrypto || cachedPassword !== password) {
    cachedCrypto = supersync.createCrypto(password);
    cachedPassword = password;
  }
  return cachedCrypto;
}

function generateOpId() {
  return 'op-' + Date.now().toString(36) + '-' + Math.floor(Math.random() * 1e9).toString(36);
}

// ---------------- AppMessage out ----------------

function sendStatus(code, message) {
  var dict = { MSG_TYPE: MSG_SYNC_STATUS, STATUS_CODE: code };
  if (message) {
    dict.STATUS_MSG = String(message).slice(0, 60);
  }
  Pebble.sendAppMessage(dict, function () {}, function (e) {
    console.log('[pkjs] sendStatus failed: ' + JSON.stringify(e));
  });
}

function sendTaskListToWatch(tasks) {
  var start = { MSG_TYPE: MSG_TASK_SYNC_START, TASK_TOTAL: tasks.length };
  Pebble.sendAppMessage(start, function () {
    sendTaskAt(tasks, 0);
  }, function (e) {
    console.log('[pkjs] failed to send TASK_SYNC_START: ' + JSON.stringify(e));
  });
}

function sendTaskAt(tasks, index) {
  if (index >= tasks.length) {
    Pebble.sendAppMessage({ MSG_TYPE: MSG_TASK_SYNC_END }, function () {}, function (e) {
      console.log('[pkjs] failed to send TASK_SYNC_END: ' + JSON.stringify(e));
    });
    return;
  }
  var t = tasks[index];
  var dict = {
    MSG_TYPE: MSG_TASK_ITEM,
    TASK_INDEX: index,
    TASK_ID: String(t.id),
    TASK_TITLE: String(t.title).slice(0, 63),
    TASK_DONE: t.isDone ? 1 : 0,
    TASK_PROJECT: String(t.project || '').slice(0, 31),
  };
  if (t.dueWithTime) {
    // Minutes since local midnight, not the raw ms timestamp or a
    // pre-formatted string - the watch has its own 12h/24h clock
    // preference (clock_is_24h_style()) and no reliable timezone info of
    // its own, so formatting happens on the C side from this, using the
    // phone's local time (matching todayStr()'s own local-day convention).
    var dueDate = new Date(t.dueWithTime);
    dict.TASK_DUE_MIN = dueDate.getHours() * 60 + dueDate.getMinutes();
  }
  if (t.timeSpent) {
    // AppMessage ints are 32-bit signed - cap well under the ~24.8 days
    // that would overflow, rather than let a very-long-lived task's total
    // wrap into a negative/garbage duration on the watch.
    dict.TASK_TIME_SPENT_MS = Math.min(t.timeSpent, 2000000000);
  }
  Pebble.sendAppMessage(dict, function () {
    sendTaskAt(tasks, index + 1);
  }, function (e) {
    console.log('[pkjs] failed to send TASK_ITEM ' + index + ': ' + JSON.stringify(e));
    // Keep going rather than stalling the whole sync on one dropped message.
    sendTaskAt(tasks, index + 1);
  });
}

// ---------------- sync engine ----------------

var syncInFlight = false;

function doSync() {
  if (syncInFlight) {
    return Promise.resolve();
  }
  var config = loadConfig();
  if (!config || !config.jwt) {
    sendStatus(STATUS_NOT_PAIRED);
    return Promise.resolve();
  }

  syncInFlight = true;
  sendStatus(STATUS_SYNCING);

  var client = new supersync.SuperSyncClient({ baseUrl: config.baseUrl, token: config.jwt });
  var crypto = getCrypto();
  var state = loadState();
  var lastSeq = loadLastSeq();
  var vectorClock = loadVectorClock();
  var isFirstSync = lastSeq === 0 && Object.keys(state.task).length === 0;
  // Set right before pullPage()'s loop actually starts (below), not here -
  // bootstrapFromSnapshot() can still move lastSeq forward first on a first
  // sync, and that jump shouldn't count against the progress range.
  var startSeq;

  var pullPage = function () {
    // Deliberately NOT passing clientId as excludeClient here (unlike every
    // other wire-format/route detail in this file, that query param was
    // never checked against live traffic - see supersync-client.js's
    // downloadOps). A completed-task change made on another device wasn't
    // coming back down on the next sync; skipping ops by clientId, if the
    // real server's matching semantics differ at all from this guess, is
    // exactly the kind of bug that would produce that symptom silently.
    // Downloading (and re-replaying) this client's own already-applied ops
    // instead is harmless: applyOperations()'s merges are idempotent, and
    // saveLastSeq() after each of our own uploads already keeps us from
    // requesting them again in the first place.
    return client.downloadOps(lastSeq, null, 500).then(function (res) {
      store.applyOperations(res.ops || [], state, crypto);
      (res.ops || []).forEach(function (entry) {
        if (entry.op && entry.op.vectorClock) {
          vectorClock = mergeVectorClocks(vectorClock, entry.op.vectorClock);
        }
      });
      // NOT res.latestSeq: confirmed against the real server source
      // (operation-download.service.ts's getOpsSinceWithSeq - `const
      // latestSeq = seqRow?.lastSeq ?? 0`) that field is the ACCOUNT'S
      // overall high-water mark, completely unrelated to pagination
      // position - it is NOT "the seq to resume from". Using it as the
      // next page's sinceSeq was confirmed live (against a real ~3600-op
      // account) to jump straight to the account's current end on the
      // very first page whenever hasMore is true, silently skipping every
      // op in between forever (lastSeq gets persisted at that
      // artificially-inflated value, so the skipped range is never
      // revisited on any later sync either). The correct resume point is
      // the highest serverSeq actually seen in THIS page.
      (res.ops || []).forEach(function (entry) {
        if (entry.serverSeq > lastSeq) {
          lastSeq = entry.serverSeq;
        }
      });
      // Progress feedback for a large first sync (or any multi-page catch-
      // up): res.latestSeq is the account's overall high-water mark (see
      // the note above) - exactly the right denominator for "how far
      // through this account's history are we", since ops are strictly
      // sequential by serverSeq. Capped below 100 while hasMore is still
      // true so a concurrent upload nudging latestSeq forward mid-sync
      // can't make this read 100% before the sync actually finishes.
      if (res.hasMore && res.latestSeq > startSeq) {
        var percent = Math.min(99, Math.floor(100 * (lastSeq - startSeq) / (res.latestSeq - startSeq)));
        sendStatus(STATUS_SYNCING, percent + '%');
      }
      if (res.hasMore) {
        return pullPage();
      }
    });
  };

  var bootstrapFromSnapshot = function () {
    return client.getRestorePoints(1).then(function (res) {
      var points = res && res.restorePoints;
      if (!points || points.length === 0) {
        return; // Brand-new account with nothing synced yet - not an error.
      }
      return client.restoreSnapshot(points[0].serverSeq).then(function (snapshot) {
        var payload = snapshot && snapshot.encrypted && crypto ? crypto.decrypt(snapshot.payload) : snapshot;
        if (payload && payload.task) {
          state.task = payload.task.entities || payload.task;
        }
        lastSeq = points[0].serverSeq;
      });
    }).catch(function (err) {
      // Confirmed against a live account: the server flatly refuses to hand
      // out a server-side snapshot for E2EE accounts (400
      // ENCRYPTED_OPS_NOT_SUPPORTED - "Use the client app's Sync Now button
      // to decrypt and restore locally"). That's the only supported path
      // for an encrypted account, which is exactly what pullPage() below
      // does by replaying the full op history from lastSeq 0 - so this
      // isn't a sync failure, just a signal to skip straight to that.
      if (err && err.body && err.body.errorCode === 'ENCRYPTED_OPS_NOT_SUPPORTED') {
        return;
      }
      throw err;
    });
  };

  var work = (isFirstSync ? bootstrapFromSnapshot() : Promise.resolve()).then(function () {
    startSeq = lastSeq;
    return pullPage();
  });

  // Returned so callers that trigger a sync as a side effect (e.g.
  // handleTaskToggle's autoSyncOnComplete) can tell once it's actually
  // finished, instead of it racing whatever status message they send next.
  return work
    .then(function () {
      saveState(state);
      saveLastSeq(lastSeq);
      saveVectorClock(vectorClock);
      var tasks = store.getActiveTasks(state, MAX_TASKS, !!config.groupByProject, !!config.todayOnly);
      sendTaskListToWatch(tasks);
      sendStatus(STATUS_OK);
    })
    .catch(function (err) {
      console.log('[pkjs] sync failed: ' + (err && err.message));
      sendStatus(STATUS_ERROR, err && err.message);
    })
    .then(function () {
      syncInFlight = false;
    });
}

// Uploads exactly one op and normalizes success/failure regardless of HTTP
// status. A rejected op still comes back as an HTTP 200 - the server
// reports per-op acceptance in res.results[].accepted, not the HTTP status
// (confirmed against sync.routes.ops-handler.ts / operation-upload.service.ts:
// a conflict, quota, or duplicate-op-id rejection still calls
// reply.send(...), never reply.status(4xx)). Only checking res.latestSeq
// used to mean an accepted upload and a rejected one were indistinguishable
// from this code's point of view - this was very likely THE actual cause of
// "completed tasks aren't syncing to the desktop app": a real rejection
// (e.g. a vector-clock conflict against a newer desktop-side edit) was
// silently swallowed as success, forever, with nothing to ever retry it.
// Resolves on acceptance (after saving lastSeq); rejects with an Error
// otherwise. On a conflict rejection specifically, the server reports the
// entity's real current vector clock (existingClock) precisely so a client
// can build a dominating clock next time instead of colliding the same way
// again - merged in here even on failure, before rejecting, so the very
// next op against this entity starts from a clock that beats what the
// server actually has.
function uploadSingleOp(op, config, clientId) {
  var client = new supersync.SuperSyncClient({ baseUrl: config.baseUrl, token: config.jwt });
  return client
    .uploadOps([op], clientId, loadLastSeq())
    .then(function (res) {
      var result = res && res.results && res.results[0];
      if (result && !result.accepted) {
        var err = new Error((result.errorCode || 'REJECTED') + ': ' + (result.error || 'op rejected by server'));
        err.rejectedResult = result;
        throw err;
      }
      // Deliberately NOT saveLastSeq(res.latestSeq) here: confirmed against
      // the real server source that this response's latestSeq is the
      // ACCOUNT'S overall high-water mark (same field/meaning as
      // downloadOps's own latestSeq - see pullPage()'s comment), not "how
      // far this upload's piggybacked res.newOps actually got us". Saving
      // it jumps our cursor to the account's current end without ever
      // downloading/applying anything in between - including res.newOps
      // itself, which this code never even looked at - silently losing any
      // ops from other clients that arrived since our last real sync,
      // every single time this ran. lastSeq now only ever advances via
      // pullPage()'s own per-page-max tracking; runAutoSyncAfterOp's
      // follow-up sync (on by default) picks up whatever res.newOps would
      // have piggybacked, correctly this time.
    })
    .catch(function (err) {
      if (err && err.rejectedResult && err.rejectedResult.existingClock) {
        saveVectorClock(mergeVectorClocks(loadVectorClock(), err.rejectedResult.existingClock));
      }
      throw err;
    });
}

// Runs after a single-op upload (success or failure) when the setting is
// on. Uploading only pushes that one op - it doesn't pull whatever else has
// changed server-side, nor re-derive/re-send the watch's own task list
// (which matters once todayOnly or backlog membership makes a just-changed
// task's visibility change). Defaults on since "the watch's change reaches
// the desktop" is the behavior actually being asked for; runs best-effort
// even after a failed upload so at least the pull side stays current,
// matching doSync()'s own error handling.
function runAutoSyncAfterOp(config, failureMsg) {
  if (config.autoSyncOnComplete === false) {
    return;
  }
  var syncPromise = doSync();
  if (failureMsg && syncPromise && typeof syncPromise.then === 'function') {
    // doSync() ends by sending its own STATUS_OK/STATUS_ERROR - without
    // this, an op that was actually rejected would show "Failed: ..." for
    // a moment and then get silently overwritten by the follow-up sync's
    // routine STATUS_OK, hiding the exact failure this whole mechanism
    // exists to surface.
    syncPromise.then(function () {
      sendStatus(STATUS_ERROR, failureMsg);
    });
  }
}

function handleTaskToggle(taskId, done) {
  var config = loadConfig();
  if (!config || !config.jwt) {
    sendStatus(STATUS_NOT_PAIRED);
    return;
  }

  var state = loadState();
  state.task[taskId] = Object.assign({}, state.task[taskId], { isDone: done });
  saveState(state);

  var crypto = getCrypto();
  // Matches the real wire shape confirmed by decrypting live ops: the
  // decrypted payload is { actionPayload: {...} }, and for
  // "[Task Shared] updateTask" specifically, actionPayload is
  // { task: Update<Task> } i.e. { id, changes } (task-shared.actions.ts) -
  // task-store.js's applyTaskAction() reads exactly this nesting on the
  // way back down. Uploading the old flat { isDone } payload (this
  // project's original, unverified assumption, same category of bug
  // already fixed on the read side - see README) meant a watch toggle
  // wasn't decodable by any real client's replay, including our own next
  // sync: sync only actually worked in the download direction.
  var payload = { actionPayload: { task: { id: taskId, changes: { isDone: done } } } };
  var clientId = getOrCreateClientId();
  // REQUIRED by the real server (confirmed by reading
  // validation.service.ts's sanitizeVectorClock(), called from
  // validateOp()): a missing/non-object vectorClock fails validation
  // outright and the whole op is rejected before it's ever stored - this
  // was the actual cause of watch-completed tasks never reaching any other
  // client, desktop included. Saved immediately (not gated on upload
  // success) since the local completion already causally happened whether
  // or not this particular upload attempt succeeds - see loadVectorClock's
  // comment.
  var newVectorClock = incrementVectorClock(loadVectorClock(), clientId);
  saveVectorClock(newVectorClock);
  var op = {
    id: generateOpId(),
    opType: 'UPD',
    actionType: '[Task Shared] updateTask',
    entityType: 'TASK',
    entityId: taskId,
    payload: crypto ? crypto.encrypt(payload) : payload,
    isPayloadEncrypted: !!crypto,
    vectorClock: newVectorClock,
    clientId: clientId,
    timestamp: Date.now(),
    schemaVersion: SCHEMA_VERSION,
  };

  var toggleFailureMsg = null;
  uploadSingleOp(op, config, clientId)
    .catch(function (err) {
      // MVP: log and leave the local optimistic update in place; the next
      // full sync will reconcile. A persisted retry queue for offline use
      // is a known gap, called out in README.md.
      toggleFailureMsg = (err && err.message) || 'upload failed, will retry next sync';
      console.log('[pkjs] failed to upload task toggle: ' + toggleFailureMsg);
      sendStatus(STATUS_ERROR, toggleFailureMsg);
    })
    .then(function () {
      runAutoSyncAfterOp(config, toggleFailureMsg);
    });
}

function handleTrackTimeStop(taskId, trackedMs) {
  var config = loadConfig();
  if (!config || !config.jwt) {
    sendStatus(STATUS_NOT_PAIRED);
    return;
  }
  if (!taskId || !trackedMs || trackedMs <= 0) {
    return;
  }

  var today = store.todayStr();
  // Deliberately NOT an optimistic local bump here, unlike
  // handleTaskToggle's isDone set. That bump is a plain replace (applying
  // it twice is harmless), but time-tracking is additive - bumping
  // timeSpentOnDay here, before upload, guarantees the same delta gets
  // counted a SECOND time moments later when this exact op comes back down
  // from the server and gets replayed by the normal additive-merge path
  // (task-store.js's '[TimeTracking] Sync time spent' case). And if the
  // upload never succeeds, the bump is never reverted either - it just
  // strands there permanently, since nothing here tracks "this local delta
  // is still unconfirmed." Both were real: found via live data showing a
  // task's watch-displayed total far exceeding the sum of everything the
  // server had actually ever accepted from it. The watch's own C-side
  // optimistic bump (stop_tracking_and_report) is safe by comparison - it
  // gets wholesale-replaced (not additively merged) by the next
  // TASK_SYNC_END, which runAutoSyncAfterOp's follow-up doSync() below
  // already triggers within moments, so skipping the equivalent bump here
  // only costs a brief instant of staleness in the phone's own cache, never
  // permanent drift.
  var crypto = getCrypto();
  // Confirmed against time-tracking.actions.ts's syncTimeSpent action
  // creator: the payload is exactly { taskId, date, duration } - duration
  // is the DELTA in ms for that calendar day, not a cumulative total and
  // not the full timeSpentOnDay map (see validation.service.ts's
  // TASK_TIME_DELTA_ACTION_TYPE check, which validates precisely this
  // shape for the unencrypted case).
  var payload = { actionPayload: { taskId: taskId, date: today, duration: trackedMs } };
  var clientId = getOrCreateClientId();
  var newVectorClock = incrementVectorClock(loadVectorClock(), clientId);
  saveVectorClock(newVectorClock);
  var op = {
    id: generateOpId(),
    opType: 'UPD',
    actionType: '[TimeTracking] Sync time spent',
    entityType: 'TASK',
    entityId: taskId,
    payload: crypto ? crypto.encrypt(payload) : payload,
    isPayloadEncrypted: !!crypto,
    vectorClock: newVectorClock,
    clientId: clientId,
    timestamp: Date.now(),
    schemaVersion: SCHEMA_VERSION,
  };

  var trackFailureMsg = null;
  uploadSingleOp(op, config, clientId)
    .catch(function (err) {
      trackFailureMsg = (err && err.message) || 'upload failed, will retry next sync';
      console.log('[pkjs] failed to upload tracked time: ' + trackFailureMsg);
      sendStatus(STATUS_ERROR, trackFailureMsg);
    })
    .then(function () {
      runAutoSyncAfterOp(config, trackFailureMsg);
    });
}

// ---------------- Pebble event wiring ----------------

Pebble.addEventListener('ready', function () {
  console.log('[pkjs] ready');
  doSync();
});

Pebble.addEventListener('appmessage', function (e) {
  var payload = e.payload;
  switch (payload.MSG_TYPE) {
    case MSG_REQUEST_SYNC:
      doSync();
      break;
    case MSG_TASK_TOGGLE:
      handleTaskToggle(payload.TASK_ID, payload.TASK_DONE === 1);
      break;
    case MSG_TRACK_TIME_STOP:
      handleTrackTimeStop(payload.TASK_ID, payload.TRACKED_MS);
      break;
    default:
      break;
  }
});

Pebble.addEventListener('showConfiguration', function () {
  var config = loadConfig() || {};
  var url = pairingPage.buildPairingPageUrl(
    config.baseUrl || supersync.DEFAULT_BASE_URL,
    config.email || '',
    {
      groupByProject: !!config.groupByProject,
      todayOnly: !!config.todayOnly,
      // Undefined (never configured before) defaults to on - see the
      // matching comment in handleTaskToggle for why.
      autoSyncOnComplete: config.autoSyncOnComplete !== false,
      // Lets the pairing page leave the password/token fields blank on a
      // settings-only visit instead of demanding they be re-pasted - see
      // webviewclosed below for the other half of this.
      hasPassword: !!localStorage.getItem('sp_password'),
      hasToken: !!config.jwt,
    }
  );
  Pebble.openURL(url);
});

Pebble.addEventListener('webviewclosed', function (e) {
  if (!e.response) {
    return;
  }
  var result;
  try {
    result = JSON.parse(decodeURIComponent(e.response));
  } catch (err) {
    console.log('[pkjs] could not parse config page response: ' + err.message);
    return;
  }
  if (result.cancelled) {
    return;
  }

  // The pairing page's own "Clear all data & resync" button, separate from
  // Save & sync - wipes the local replay cache without touching credentials/
  // options, then does a fresh full resync (isFirstSync in doSync() keys off
  // exactly this state: lastSeq 0 and no cached tasks).
  if (result.clearData) {
    localStorage.removeItem('sp_entities');
    localStorage.removeItem('sp_last_seq');
    localStorage.removeItem('sp_vector_clock');
    doSync();
    return;
  }

  var existingConfig = loadConfig() || {};
  var previousPassword = localStorage.getItem('sp_password');
  // Blank jwt/password fields mean "keep what's already saved" (the
  // pairing page only requires jwt on a first-ever pairing - see hasToken
  // there), not "clear it" - so a settings-only visit doesn't force
  // re-pasting either one.
  var newJwt = result.jwt || existingConfig.jwt;
  var jwtChanged = !!result.jwt && result.jwt !== existingConfig.jwt;
  var passwordChanged = !!result.password && result.password !== previousPassword;

  saveConfig({
    baseUrl: result.baseUrl || supersync.DEFAULT_BASE_URL,
    email: result.email,
    jwt: newJwt,
    groupByProject: !!result.groupByProject,
    todayOnly: !!result.todayOnly,
    autoSyncOnComplete: !!result.autoSyncOnComplete,
  });

  if (result.password) {
    localStorage.setItem('sp_password', result.password);
  }
  // The Argon2id derived-key cache in getCrypto() is keyed off this same
  // password, but comparing by value there isn't enough on its own to
  // notice "same password string, different account" - clearing it here
  // whenever pairing completes is cheap insurance either way.
  cachedCrypto = null;
  cachedPassword = null;

  // Only an actual credential change (new account/token or changed
  // password) invalidates what's cached locally - a settings-only save
  // (e.g. toggling todayOnly) used to wipe and fully re-download the task
  // list every time, which is exactly what the new clearData path above is
  // for now; this path should be quiet unless something that changes what
  // the replay log means actually changed.
  if (jwtChanged || passwordChanged) {
    localStorage.removeItem('sp_entities');
    localStorage.removeItem('sp_last_seq');
    localStorage.removeItem('sp_vector_clock');
  }

  doSync();
});
