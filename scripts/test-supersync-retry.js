// Retry / timeout behaviour of supersync-client.js's request() against a fake
// XMLHttpRequest. Delays are collapsed to 0 so the test runs instantly.
var assert = require('assert');
var realSetTimeout = setTimeout;
global.setTimeout = function (fn) { return realSetTimeout(fn, 0); };

var script = [];   // per-attempt outcomes: 'error' | 'timeout' | <status number>
var seenTimeouts = [];
function FakeXHR() {}
FakeXHR.prototype.open = function () {};
FakeXHR.prototype.setRequestHeader = function () {};
FakeXHR.prototype.send = function () {
  var self = this;
  seenTimeouts.push(self.timeout);
  var outcome = script.shift();
  realSetTimeout(function () {
    if (outcome === 'error') { self.onerror(); }
    else if (outcome === 'timeout') { self.ontimeout(); }
    else { self.status = outcome; self.responseText = '{"ok":true}'; self.onload(); }
  }, 0);
};
global.XMLHttpRequest = FakeXHR;

var sync = require('../src/pkjs/lib/supersync-client.js');
var client = new sync.SuperSyncClient({ baseUrl: 'http://x', token: 't' });
var results = [];
function check(name, ok) { results.push(name); console.log('ok   - ' + name); }

function run(outcomes) {
  script = outcomes.slice();
  seenTimeouts = [];
  return client.getStatus();
}

run(['error', 'timeout', 503, 200]).then(function () {
  assert.strictEqual(seenTimeouts.length, 4);
  check('recovers after network error, timeout and 503');
  assert.deepStrictEqual(seenTimeouts, [30000, 45000, 60000, 60000]);
  check('timeout grows each attempt');
  return run(['error', 'error', 'error', 'error']).then(function () {
    assert.fail('should reject');
  }, function (err) {
    assert.strictEqual(seenTimeouts.length, 4);
    assert.ok(/Network error/.test(err.message));
    check('gives up after 4 attempts with the last error');
  });
}).then(function () {
  return run([401, 200]).then(function () { assert.fail('should reject'); }, function (err) {
    assert.strictEqual(err.status, 401);
    assert.strictEqual(seenTimeouts.length, 1);
    check('a 401 is not retried');
  });
}).then(function () {
  return run([429, 200]).then(function () {
    assert.strictEqual(seenTimeouts.length, 2);
    check('429 is retried');
  });
}).then(function () {
  console.log('\nAll checks passed.');
}).catch(function (e) { console.error(e); process.exit(1); });
