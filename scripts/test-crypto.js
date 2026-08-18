// Cross-checks the pure-JS crypto in src/pkjs/lib/ against Node's native
// `crypto` module, which is treated as the correctness oracle. Run with:
//   node scripts/test-crypto.js
'use strict';

const assert = require('assert');
const nodeCrypto = require('crypto');
const sha256lib = require('../src/pkjs/lib/sha256.js');
const aesGcm = require('../src/pkjs/lib/aes-gcm.js');
const base64 = require('../src/pkjs/lib/base64.js');

function randomBytes(n) {
  return Array.from(nodeCrypto.randomBytes(n));
}

function toBuf(byteArray) {
  return Buffer.from(byteArray);
}

let failures = 0;

function check(name, fn) {
  try {
    fn();
    console.log(`ok   - ${name}`);
  } catch (err) {
    failures++;
    console.log(`FAIL - ${name}`);
    console.log(`       ${err.message}`);
  }
}

// ---------------- SHA-256 ----------------

check('sha256 empty string', () => {
  const expected = nodeCrypto.createHash('sha256').update('').digest();
  const actual = toBuf(sha256lib.sha256([]));
  assert.deepStrictEqual(actual, expected);
});

check('sha256 "abc"', () => {
  const input = sha256lib.utf8ToBytes('abc');
  const expected = nodeCrypto.createHash('sha256').update('abc').digest();
  const actual = toBuf(sha256lib.sha256(input));
  assert.deepStrictEqual(actual, expected);
});

check('sha256 random 1000-byte input, 5 trials', () => {
  for (let i = 0; i < 5; i++) {
    const data = randomBytes(1000 + i);
    const expected = nodeCrypto.createHash('sha256').update(toBuf(data)).digest();
    const actual = toBuf(sha256lib.sha256(data));
    assert.deepStrictEqual(actual, expected, `trial ${i}`);
  }
});

check('sha256 handles unicode (utf8ToBytes)', () => {
  const str = 'héllo wörld 🎉 日本語';
  const expected = nodeCrypto.createHash('sha256').update(str, 'utf8').digest();
  const actual = toBuf(sha256lib.sha256(sha256lib.utf8ToBytes(str)));
  assert.deepStrictEqual(actual, expected);
});

// ---------------- HMAC-SHA256 ----------------

check('hmac-sha256 matches node, short key', () => {
  const key = randomBytes(16);
  const msg = randomBytes(37);
  const expected = nodeCrypto.createHmac('sha256', toBuf(key)).update(toBuf(msg)).digest();
  const actual = toBuf(sha256lib.hmacSha256(key, msg));
  assert.deepStrictEqual(actual, expected);
});

check('hmac-sha256 matches node, key longer than block size', () => {
  const key = randomBytes(100);
  const msg = randomBytes(10);
  const expected = nodeCrypto.createHmac('sha256', toBuf(key)).update(toBuf(msg)).digest();
  const actual = toBuf(sha256lib.hmacSha256(key, msg));
  assert.deepStrictEqual(actual, expected);
});

// ---------------- PBKDF2 ----------------

check('pbkdf2-hmac-sha256 matches node', () => {
  const password = sha256lib.utf8ToBytes('correct horse battery staple');
  const salt = randomBytes(16);
  const iterations = 10000;
  const keyLen = 32;
  const expected = nodeCrypto.pbkdf2Sync(toBuf(password), toBuf(salt), iterations, keyLen, 'sha256');
  const actual = toBuf(sha256lib.pbkdf2(password, salt, iterations, keyLen));
  assert.deepStrictEqual(actual, expected);
});

check('pbkdf2 derives keyLen not a multiple of 32', () => {
  const password = sha256lib.utf8ToBytes('pw');
  const salt = randomBytes(8);
  const expected = nodeCrypto.pbkdf2Sync(toBuf(password), toBuf(salt), 1000, 48, 'sha256');
  const actual = toBuf(sha256lib.pbkdf2(password, salt, 1000, 48));
  assert.deepStrictEqual(actual, expected);
});

// ---------------- AES-GCM ----------------

function checkGcmRoundtrip(name, keyLen, ptLen, aadLen) {
  check(name, () => {
    const key = randomBytes(keyLen);
    const iv = randomBytes(12);
    const plaintext = randomBytes(ptLen);
    const aad = randomBytes(aadLen);

    const nodeCipher = nodeCrypto.createCipheriv(`aes-${keyLen * 8}-gcm`, toBuf(key), toBuf(iv));
    nodeCipher.setAAD(toBuf(aad));
    const nodeCiphertext = Buffer.concat([nodeCipher.update(toBuf(plaintext)), nodeCipher.final()]);
    const nodeTag = nodeCipher.getAuthTag();

    const ours = aesGcm.aesGcmEncrypt(key, iv, plaintext, aad);
    assert.deepStrictEqual(toBuf(ours.ciphertext), nodeCiphertext, 'ciphertext mismatch');
    assert.deepStrictEqual(toBuf(ours.tag), nodeTag, 'tag mismatch');

    // Now decrypt Node's ciphertext with our implementation.
    const decrypted = aesGcm.aesGcmDecrypt(key, iv, Array.from(nodeCiphertext), Array.from(nodeTag), aad);
    assert.deepStrictEqual(toBuf(decrypted), toBuf(plaintext), 'decrypt mismatch');

    // And verify Node can decrypt our ciphertext/tag.
    const nodeDecipher = nodeCrypto.createDecipheriv(`aes-${keyLen * 8}-gcm`, toBuf(key), toBuf(iv));
    nodeDecipher.setAAD(toBuf(aad));
    nodeDecipher.setAuthTag(toBuf(ours.tag));
    const nodeDecrypted = Buffer.concat([nodeDecipher.update(toBuf(ours.ciphertext)), nodeDecipher.final()]);
    assert.deepStrictEqual(nodeDecrypted, toBuf(plaintext), 'node could not decrypt our ciphertext');
  });
}

checkGcmRoundtrip('aes-128-gcm roundtrip, 0-byte plaintext', 16, 0, 0);
checkGcmRoundtrip('aes-128-gcm roundtrip, 1-byte plaintext', 16, 1, 0);
checkGcmRoundtrip('aes-128-gcm roundtrip, exactly 16 bytes', 16, 16, 0);
checkGcmRoundtrip('aes-128-gcm roundtrip, 33 bytes + aad', 16, 33, 20);
checkGcmRoundtrip('aes-256-gcm roundtrip, 500 bytes (typical task JSON size) + aad', 32, 500, 16);
checkGcmRoundtrip('aes-256-gcm roundtrip, 4096 bytes', 32, 4096, 0);

check('aes-gcm decrypt rejects tampered ciphertext', () => {
  const key = randomBytes(32);
  const iv = randomBytes(12);
  const plaintext = sha256lib.utf8ToBytes('{"title":"Buy milk","isDone":false}');
  const { ciphertext, tag } = aesGcm.aesGcmEncrypt(key, iv, plaintext, []);
  const tampered = ciphertext.slice();
  tampered[0] ^= 0xff;
  assert.throws(() => aesGcm.aesGcmDecrypt(key, iv, tampered, tag, []));
});

check('aes-gcm decrypt rejects wrong key (wrong password)', () => {
  const key = randomBytes(32);
  const wrongKey = randomBytes(32);
  const iv = randomBytes(12);
  const plaintext = sha256lib.utf8ToBytes('secret task title');
  const { ciphertext, tag } = aesGcm.aesGcmEncrypt(key, iv, plaintext, []);
  assert.throws(() => aesGcm.aesGcmDecrypt(wrongKey, iv, ciphertext, tag, []));
});

// ---------------- base64 ----------------

check('base64 matches node for various lengths', () => {
  [0, 1, 2, 3, 4, 5, 16, 17, 100, 255].forEach((len) => {
    const data = randomBytes(len);
    const expected = toBuf(data).toString('base64');
    const actual = base64.bytesToBase64(data);
    assert.strictEqual(actual, expected, `encode len=${len}`);
    const decoded = base64.base64ToBytes(actual);
    assert.deepStrictEqual(toBuf(decoded), toBuf(data), `decode len=${len}`);
  });
});

console.log('');
if (failures > 0) {
  console.log(`${failures} check(s) FAILED`);
  process.exit(1);
} else {
  console.log('All checks passed.');
}
