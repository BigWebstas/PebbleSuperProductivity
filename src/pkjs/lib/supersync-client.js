// Thin client for the SuperSync REST API (packages/super-sync-server in the
// super-productivity repo), plus the E2EE payload encrypt/decrypt helpers.
//
// ASSUMPTIONS THAT NEED CONFIRMING AGAINST A LIVE SERVER / ACCOUNT
// -------------------------------------------------------------------------
// This was implemented from the sync server's route list and prose
// architecture docs, not from the literal TypeScript contract file, because
// this environment has no way to create a SuperSync account (registration
// requires a real email + clicking a magic link, or WebAuthn) to capture a
// live request/response for reference. Treat the following as best-effort
// and re-verify against packages/shared-schema/src/supersync-http-contract.ts
// and packages/super-sync-server/docs/supersync-encryption-architecture.md
// once you have a live token:
//   1. Exact field names on Operation objects (id/type/entityType/entityId/
//      payload/clientId/timestamp/vectorClock) - `applyOperation()` below.
//   2. The E2EE key derivation (this uses PBKDF2-HMAC-SHA256, 210000
//      iterations, salt = utf8("supersync:" + email) - the real app almost
//      certainly uses a different salt/iteration count, which would produce
//      a different key and fail to decrypt existing data even with the
//      right password).
//   3. The exact shape of the IV/ciphertext/tag encoding inside
//      `operation.payload` when encrypted (assumed: base64 JSON envelope
//      `{iv, ciphertext, tag}`).
//   4. Whether GET /api/sync/restore/:serverSeq returns entities in
//      encrypted-payload form (like ops) or as one encrypted blob.
// None of this affects the crypto primitives themselves (aes-gcm.js,
// sha256.js), which are verified byte-for-byte against Node's `crypto` in
// scripts/test-crypto.js - only the *wire format* around them is a guess.
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
        var err = new Error('SuperSync request failed: ' + method + ' ' + path + ' -> ' + xhr.status);
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
