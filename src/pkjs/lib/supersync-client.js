// Thin client for the SuperSync REST API (packages/super-sync-server in the
// super-productivity repo), plus the E2EE payload encrypt/decrypt helpers.
//
// Routes, auth scheme, and GET /api/sync/ops's response shape are confirmed
// against a live account's real traffic (see README.md's "What is verified
// vs. assumed" section) - see task-store.js for the confirmed Operation
// field names (opType/entityType/isPayloadEncrypted, not
// type/entityType/encrypted, and entries are wrapped as
// { serverSeq, op, receivedAt }).
//
// STILL UNCONFIRMED
// -------------------------------------------------------------------------
//   1. The E2EE key derivation (this uses PBKDF2-HMAC-SHA256, 210000
//      iterations, salt = utf8("supersync:" + email) - unverified, would
//      produce a different key and fail to decrypt existing data even with
//      the right password if wrong).
//   2. The exact byte layout inside an encrypted `op.payload`. Confirmed to
//      be a single base64 string (not a `{iv, ciphertext, tag}` JSON
//      envelope as originally assumed), but the IV/ciphertext/tag split
//      within those bytes is still unknown - decryptPayload() below will
//      throw on real data until this is pinned down.
// GET /api/sync/restore/:serverSeq is confirmed to reject E2EE accounts
// outright (400 ENCRYPTED_OPS_NOT_SUPPORTED) - index.js no longer treats
// that as a sync failure, it falls through to a full ops replay instead.
'use strict';

var aesGcm = require('./aes-gcm.js');
var sha256lib = require('./sha256.js');
var base64 = require('./base64.js');

var DEFAULT_BASE_URL = 'https://sync.super-productivity.com';
var KDF_ITERATIONS = 210000;
var KDF_KEY_LEN = 32; // AES-256

function deriveEncryptionKey(password, email) {
  var salt = sha256lib.utf8ToBytes('supersync:' + email);
  return sha256lib.pbkdf2(sha256lib.utf8ToBytes(password), salt, KDF_ITERATIONS, KDF_KEY_LEN);
}

function randomIv() {
  var iv = new Array(12);
  for (var i = 0; i < 12; i++) {
    iv[i] = Math.floor(Math.random() * 256);
  }
  return iv;
}

function encryptPayload(obj, key) {
  var plaintext = sha256lib.utf8ToBytes(JSON.stringify(obj));
  var iv = randomIv();
  var result = aesGcm.aesGcmEncrypt(key, iv, plaintext, []);
  return {
    iv: base64.bytesToBase64(iv),
    ciphertext: base64.bytesToBase64(result.ciphertext),
    tag: base64.bytesToBase64(result.tag),
  };
}

function decryptPayload(envelope, key) {
  var iv = base64.base64ToBytes(envelope.iv);
  var ciphertext = base64.base64ToBytes(envelope.ciphertext);
  var tag = base64.base64ToBytes(envelope.tag);
  var plaintextBytes = aesGcm.aesGcmDecrypt(key, iv, ciphertext, tag, []);
  var str = bytesToUtf8(plaintextBytes);
  return JSON.parse(str);
}

function bytesToUtf8(bytes) {
  // Minimal UTF-8 decoder (inverse of sha256lib.utf8ToBytes).
  var out = '';
  var i = 0;
  while (i < bytes.length) {
    var b0 = bytes[i];
    if (b0 < 0x80) {
      out += String.fromCharCode(b0);
      i += 1;
    } else if ((b0 & 0xe0) === 0xc0) {
      out += String.fromCharCode(((b0 & 0x1f) << 6) | (bytes[i + 1] & 0x3f));
      i += 2;
    } else if ((b0 & 0xf0) === 0xe0) {
      out += String.fromCharCode(
        ((b0 & 0x0f) << 12) | ((bytes[i + 1] & 0x3f) << 6) | (bytes[i + 2] & 0x3f)
      );
      i += 3;
    } else {
      var cp =
        ((b0 & 0x07) << 18) |
        ((bytes[i + 1] & 0x3f) << 12) |
        ((bytes[i + 2] & 0x3f) << 6) |
        (bytes[i + 3] & 0x3f);
      out += String.fromCodePoint(cp);
      i += 4;
    }
  }
  return out;
}

// ---------------- HTTP ----------------

function request(method, baseUrl, path, token, body) {
  return new Promise(function (resolve, reject) {
    var xhr = new XMLHttpRequest();
    xhr.open(method, baseUrl + path, true);
    xhr.setRequestHeader('Content-Type', 'application/json');
    if (token) {
      xhr.setRequestHeader('Authorization', 'Bearer ' + token);
    }
    xhr.onload = function () {
      var responseBody;
      try {
        responseBody = xhr.responseText ? JSON.parse(xhr.responseText) : null;
      } catch (e) {
        responseBody = xhr.responseText;
      }
      if (xhr.status >= 200 && xhr.status < 300) {
        resolve(responseBody);
      } else {
        // Prefer the server's own explanation (e.g. which field failed
        // validation) over a bare status code - this is the only signal
        // available for debugging the "assumed" wire-format details
        // documented at the top of this file, short of a live account.
        var detail = (responseBody && typeof responseBody === 'object' && responseBody.error) ?
          responseBody.error : (method + ' ' + path);
        var err = new Error(xhr.status + ' ' + detail);
        err.status = xhr.status;
        err.body = responseBody;
        reject(err);
      }
    };
    xhr.onerror = function () {
      reject(new Error('Network error calling ' + path));
    };
    xhr.ontimeout = function () {
      reject(new Error('Timed out calling ' + path));
    };
    xhr.timeout = 15000;
    xhr.send(body ? JSON.stringify(body) : undefined);
  });
}

function SuperSyncClient(opts) {
  this.baseUrl = (opts && opts.baseUrl) || DEFAULT_BASE_URL;
  this.token = opts && opts.token;
}

SuperSyncClient.prototype.downloadOps = function (sinceSeq, excludeClient, limit) {
  var qs = '?sinceSeq=' + encodeURIComponent(sinceSeq);
  if (limit) {
    qs += '&limit=' + limit;
  }
  if (excludeClient) {
    qs += '&excludeClient=' + encodeURIComponent(excludeClient);
  }
  return request('GET', this.baseUrl, '/api/sync/ops' + qs, this.token);
};

SuperSyncClient.prototype.uploadOps = function (ops, clientId, lastKnownServerSeq) {
  return request('POST', this.baseUrl, '/api/sync/ops', this.token, {
    ops: ops,
    clientId: clientId,
    lastKnownServerSeq: lastKnownServerSeq,
  });
};

SuperSyncClient.prototype.getRestorePoints = function (limit) {
  var qs = limit ? '?limit=' + limit : '';
  return request('GET', this.baseUrl, '/api/sync/restore-points' + qs, this.token);
};

SuperSyncClient.prototype.restoreSnapshot = function (serverSeq) {
  return request('GET', this.baseUrl, '/api/sync/restore/' + encodeURIComponent(serverSeq), this.token);
};

SuperSyncClient.prototype.getStatus = function () {
  return request('GET', this.baseUrl, '/api/sync/status', this.token);
};

module.exports = {
  SuperSyncClient: SuperSyncClient,
  deriveEncryptionKey: deriveEncryptionKey,
  encryptPayload: encryptPayload,
  decryptPayload: decryptPayload,
  DEFAULT_BASE_URL: DEFAULT_BASE_URL,
};
