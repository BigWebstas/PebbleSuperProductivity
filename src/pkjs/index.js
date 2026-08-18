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

var STATUS_OK = 0;
var STATUS_SYNCING = 1;
var STATUS_NOT_PAIRED = 2;
var STATUS_ERROR = 3;

var MAX_TASKS = 30;

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
    return;
  }
  var config = loadConfig();
  if (!config || !config.jwt) {
    sendStatus(STATUS_NOT_PAIRED);
    return;
  }

  syncInFlight = true;
  sendStatus(STATUS_SYNCING);

  var client = new supersync.SuperSyncClient({ baseUrl: config.baseUrl, token: config.jwt });
  var crypto = getCrypto();
  var state = loadState();
  var lastSeq = loadLastSeq();
  var vectorClock = loadVectorClock();
  var isFirstSync = lastSeq === 0 && Object.keys(state.task).length === 0;

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
      lastSeq = res.latestSeq != null ? res.latestSeq : lastSeq;
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

  var work = isFirstSync ? bootstrapFromSnapshot().then(pullPage) : pullPage();

  work
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
  };

  var client = new supersync.SuperSyncClient({ baseUrl: config.baseUrl, token: config.jwt });
  client
    .uploadOps([op], clientId, loadLastSeq())
    .then(function (res) {
      if (res && res.latestSeq != null) {
        saveLastSeq(res.latestSeq);
      }
    })
    .catch(function (err) {
      // MVP: log and leave the local optimistic update in place; the next
      // full sync will reconcile. A persisted retry queue for offline use
      // is a known gap, called out in README.md.
      console.log('[pkjs] failed to upload task toggle: ' + (err && err.message));
      sendStatus(STATUS_ERROR, 'upload failed, will retry next sync');
    })
    .then(function () {
      // Uploading only pushes this one op - it doesn't pull whatever else
      // has changed server-side, nor re-derive/re-send the watch's own task
      // list (which matters once todayOnly or backlog membership makes a
      // just-toggled task's visibility change). Defaults on since "toggle
      // on the watch reaches the desktop" is the behavior actually being
      // asked for; runs best-effort even after a failed upload so at least
      // the pull side stays current, matching doSync()'s own error handling.
      if (config.autoSyncOnComplete !== false) {
        doSync();
      }
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
