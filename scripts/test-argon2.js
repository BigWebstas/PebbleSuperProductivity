// Cross-checks blake2b.js and argon2id.js against hash-wasm's output (the
// same library Super Productivity's real client uses -
// packages/sync-core/src/encryption/argon2.ts). Node's built-in `crypto`
// module has no BLAKE2b or Argon2 support, so unlike test-crypto.js this
// can't use it as the oracle; these vectors were generated once with
// hash-wasm installed in a throwaway scratch directory and are hardcoded
// here as a permanent regression test.
//
// Includes one full production-parameter case (parallelism=1, iterations=3,
// memorySize=65536 KiB - see supersync-client.js's ARGON2_PARAMS) - this is
// the expensive, multi-second case, kept because it's the one that actually
// matters for real accounts; everything else uses tiny parameters purely to
// exercise the pass/slice/addressing logic quickly.
//
// Run with: node scripts/test-argon2.js
'use strict';

const assert = require('assert');
const { blake2b } = require('../src/pkjs/lib/blake2b.js');
const { argon2id } = require('../src/pkjs/lib/argon2id.js');

function hex(bytes) {
  return bytes.map((b) => b.toString(16).padStart(2, '0')).join('');
}
function utf8(s) {
  return Array.from(Buffer.from(s, 'utf8'));
}
function hexToBytes(h) {
  const out = [];
  for (let i = 0; i < h.length; i += 2) out.push(parseInt(h.substr(i, 2), 16));
  return out;
}

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

// ---------------- BLAKE2b (RFC 7693 test vectors) ----------------

check('blake2b("abc"), 64-byte digest', () => {
  assert.strictEqual(
    hex(blake2b(utf8('abc'), 64)),
    'ba80a53f981c4d0d6a2797b69f12f6e94c212f14685ac4b74b12bb6fdbffa2d17d87c5392aab792dc252d5de4533cc9518d38aa8dbf1925ab92386edd4009923'
  );
});

check('blake2b(""), 64-byte digest', () => {
  assert.strictEqual(
    hex(blake2b(utf8(''), 64)),
    '786a02f742015903c6c6fd852552d272912f4740e15847618a86e217f71f5419d25e1031afee585313896444934eb04b903a685b1448b755d56f701afe9be2ce'
  );
});

check('blake2b("abc"), 32-byte digest (variable output length)', () => {
  assert.strictEqual(
    hex(blake2b(utf8('abc'), 32)),
    'bddd813c634239723171ef3fee98579b94964e3bb1cb3e427262c8c068d52319'
  );
});

// ---------------- Argon2id (cross-checked against hash-wasm) ----------------

const argon2Cases = [
  { label: 'tiny, iterations=1, memory=8 KiB', password: 'super_secret_password', saltHex: '02020202020202020202020202020202'.slice(0, 32), iterations: 1, memorySize: 8, hashLength: 32, expect: '57e86cee9af2d544d15c7c4c2fed27cd8b0ecd673aeba0fe6f2ead2ed3bb3358' },
  { label: 'tiny, iterations=1, memory=8 KiB, short password', password: 'pw', saltHex: '30313233343536373839616263646566', iterations: 1, memorySize: 8, hashLength: 32, expect: '2d27fc7564b6ac383fe503cb888b442eab1aa34a89b65f02d859e617a0dda66b' },
  { label: 'multi-pass, iterations=2, memory=16 KiB (exercises slice0/1 boundary)', password: 'x', saltHex: 'ffffffffffffffffffffffffffffffff', iterations: 2, memorySize: 16, hashLength: 32, expect: '5abdb5493c3157675905f86e9b90ca98d43e0169fd12a07a16ff0d00ffbdb0f5' },
  { label: 'iterations=2, memory=8 KiB minimal', password: 'x', saltHex: 'ffffffffffffffffffffffffffffffff', iterations: 2, memorySize: 8, hashLength: 32, expect: 'fb4b29d4c2774c1d3af8c2945fe7422f50decb469f3feb8798e9327c4899cde2' },
  { label: 'iterations=3, memory=8 KiB', password: 'x', saltHex: 'ffffffffffffffffffffffffffffffff', iterations: 3, memorySize: 8, hashLength: 32, expect: '065ba985b745f80231ce28b7c150f1a367ac354d6a5607be1fce2f245ea115d0' },
  { label: 'iterations=2, memory=32 KiB', password: 'x', saltHex: 'ffffffffffffffffffffffffffffffff', iterations: 2, memorySize: 32, hashLength: 32, expect: 'a9a98a1d752d246ed160cd121298ce7593423732dc5ee36d6954b8d9b6dcfd91' },
  { label: 'iterations=1, memory=32 KiB', password: 'x', saltHex: 'ffffffffffffffffffffffffffffffff', iterations: 1, memorySize: 32, hashLength: 32, expect: 'df7dc47c6d23b2216f996936a24a83349c381dc7e4878a3ed39a6392db00d6e9' },
  { label: 'non-default hashLength=16', password: '63fabd7b日本語0', saltHex: 'cf7a85bdcac655e7309fd996605e84eb', iterations: 1, memorySize: 8, hashLength: 16, expect: '1dbc8faa9b78191e8601b70bc2129653' },
  { label: 'non-default hashLength=64 (exercises H\' extended chaining)', password: '860045ca28d679日本語3', saltHex: '3ad0fe70ccfac892ab6efca036826586', iterations: 1, memorySize: 20, hashLength: 64, expect: '8ce3386095ac2ff8154a4158d3874ae0fd405a7082444336342e52c8d8adefaddfd64723abc394b101a798dad9ee7c429b156cbea7531ec58b1c246848e00118' },
];

for (const c of argon2Cases) {
  check(`argon2id: ${c.label}`, () => {
    const out = hex(argon2id(utf8(c.password), hexToBytes(c.saltHex), {
      iterations: c.iterations,
      memorySize: c.memorySize,
      hashLength: c.hashLength,
    }));
    assert.strictEqual(out, c.expect);
  });
}

check('argon2id: production parameters (parallelism=1, iterations=3, memorySize=65536 KiB) - slow, ~15s', () => {
  const out = hex(argon2id(
    utf8('webstas@protonmail.com-test-password'),
    hexToBytes('6162636465666768696a6b6c6d6e6f70'),
    { iterations: 3, memorySize: 65536, hashLength: 32 }
  ));
  assert.strictEqual(out, '5912887dd92b5a2be65555c109f28ad2a08958b8c736f3c1fee012cca3caa534');
});

console.log('');
if (failures > 0) {
  console.log(`${failures} check(s) FAILED`);
  process.exit(1);
} else {
  console.log('All checks passed.');
}
