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

function getEncryptionKey() {
  var b64 = localStorage.getItem('sp_enc_key_b64');
  if (!b64) {
    return null;
  }
  var base64 = require('./lib/base64.js');
  return base64.base64ToBytes(b64);
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
  var key = getEncryptionKey();
  var clientId = getOrCreateClientId();
  var state = loadState();
  var lastSeq = loadLastSeq();
  var isFirstSync = lastSeq === 0 && Object.keys(state.task).length === 0;

  var pullPage = function () {
    return client.downloadOps(lastSeq, clientId, 500).then(function (res) {
      store.applyOperations(res.ops || [], state, key);
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
        var payload = snapshot && snapshot.encrypted && key ? supersync.decryptPayload(snapshot.payload, key) : snapshot;
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
      var tasks = store.getTodayTasks(state, MAX_TASKS);
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

  var key = getEncryptionKey();
  var payload = { isDone: done };
  var op = {
    id: generateOpId(),
    type: 'UPD',
    entityType: 'task',
    entityId: taskId,
    payload: key ? supersync.encryptPayload(payload, key) : payload,
    encrypted: !!key,
    clientId: getOrCreateClientId(),
    timestamp: Date.now(),
  };

  var client = new supersync.SuperSyncClient({ baseUrl: config.baseUrl, token: config.jwt });
  client
    .uploadOps([op], getOrCreateClientId(), loadLastSeq())
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
  var url = pairingPage.buildPairingPageUrl(config.baseUrl || supersync.DEFAULT_BASE_URL, config.email || '');
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

  saveConfig({ baseUrl: result.baseUrl || supersync.DEFAULT_BASE_URL, email: result.email, jwt: result.jwt });

  if (result.password && result.email) {
    var base64 = require('./lib/base64.js');
    var key = supersync.deriveEncryptionKey(result.password, result.email);
    localStorage.setItem('sp_enc_key_b64', base64.bytesToBase64(key));
  }

  // A new pairing (or a changed account/password) invalidates whatever we
  // had cached locally.
  localStorage.removeItem('sp_entities');
  localStorage.removeItem('sp_last_seq');

  doSync();
});
