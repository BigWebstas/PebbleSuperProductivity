// Exercises the cross-session key-cache persistence wired into
// supersync-client.js's createCrypto() - the optimization that lets a fresh
// pkjs session skip Argon2id for any salt an earlier session already
// derived (see createCrypto's `persistence` param and index.js's
// getCrypto()). Runs exactly one production-parameter Argon2id derivation
// (multi-second), then asserts every later operation reuses it.
//
// Run with: node scripts/test-supersync-crypto.js
'use strict';

const assert = require('assert');
const { createCrypto } = require('../src/pkjs/lib/supersync-client.js');

let failures = 0;
function check(name, fn) {
  try {
    fn();
    console.log(`ok   - ${name}`);
  } catch (err) {
    failures++;
    console.log(`FAIL - ${name}`);
    console.log(`       ${err.stack}`);
  }
}

// A stand-in for the localStorage-backed store index.js passes in.
function makeStore() {
  const keys = {};
  let encryptSalt = null;
  let saveKeyCalls = 0;
  return {
    keys,
    getSaveKeyCalls: () => saveKeyCalls,
    persistence: {
      loadKeys: () => Object.assign({}, keys),
      saveKey: (b64salt, b64key) => {
        saveKeyCalls++;
        keys[b64salt] = b64key;
      },
      loadEncryptSalt: () => encryptSalt,
      saveEncryptSalt: (b64salt) => {
        encryptSalt = b64salt;
      },
    },
  };
}

const PASSWORD = 'webstas@protonmail.com-test-password';
const SECRET = { actionPayload: { title: 'Buy oat milk', isDone: false } };

const store = makeStore();

let wireFormat;
check('session 1: first encrypt derives and persists exactly one key + the salt', () => {
  const t0 = Date.now();
  const crypto1 = createCrypto(PASSWORD, store.persistence);
  wireFormat = crypto1.encrypt(SECRET);
  const elapsed = Date.now() - t0;

  assert.ok(elapsed > 500, `expected a real (slow) derivation, took only ${elapsed}ms`);
  assert.strictEqual(store.getSaveKeyCalls(), 1, 'should persist exactly one derived key');
  assert.strictEqual(Object.keys(store.keys).length, 1);
  assert.ok(store.persistence.loadEncryptSalt(), 'encrypt salt should be persisted');
  assert.deepStrictEqual(crypto1.decrypt(wireFormat), SECRET, 'same instance round-trips');
});

check('session 2: a fresh instance seeded from the store decrypts with no derivation', () => {
  const before = store.getSaveKeyCalls();
  const t0 = Date.now();
  const crypto2 = createCrypto(PASSWORD, store.persistence);
  const decrypted = crypto2.decrypt(wireFormat);
  const elapsed = Date.now() - t0;

  assert.deepStrictEqual(decrypted, SECRET);
  assert.ok(elapsed < 500, `expected cache hit, took ${elapsed}ms`);
  assert.strictEqual(store.getSaveKeyCalls(), before, 'no new key should be derived/persisted');
});

check('session 2: encrypt reuses the persisted salt and its cached key', () => {
  const before = store.getSaveKeyCalls();
  const crypto2 = createCrypto(PASSWORD, store.persistence);
  const t0 = Date.now();
  const out = crypto2.encrypt(SECRET);
  const elapsed = Date.now() - t0;

  assert.ok(elapsed < 500, `expected cache hit, took ${elapsed}ms`);
  assert.strictEqual(store.getSaveKeyCalls(), before, 'no new derivation');
  // First 16 bytes on the wire are the salt - must match session 1's.
  const saltOf = (b64) => Buffer.from(b64, 'base64').slice(0, 16).toString('hex');
  assert.strictEqual(saltOf(out), saltOf(wireFormat), 'encrypt salt should be stable across sessions');
  const crypto1again = createCrypto(PASSWORD, store.persistence);
  assert.deepStrictEqual(crypto1again.decrypt(out), SECRET);
});

check('a corrupt persisted key is ignored, not trusted', () => {
  const corruptStore = makeStore();
  const b64salt = Object.keys(store.keys)[0];
  // Same salt, but a wrong (zeroed) key.
  corruptStore.keys[b64salt] = Buffer.alloc(32).toString('base64');
  const crypto = createCrypto(PASSWORD, corruptStore.persistence);
  // Decrypting session 1's real ciphertext with the bogus cached key must
  // fail loudly (AES-GCM tag check), never silently return garbage.
  assert.throws(() => crypto.decrypt(wireFormat));
});

check('createCrypto still works with no persistence object', () => {
  const crypto = createCrypto(PASSWORD);
  const out = crypto.encrypt(SECRET);
  assert.deepStrictEqual(crypto.decrypt(out), SECRET);
});

console.log('');
if (failures > 0) {
  console.log(`${failures} check(s) FAILED`);
  process.exit(1);
} else {
  console.log('All checks passed.');
}
