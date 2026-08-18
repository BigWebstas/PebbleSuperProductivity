// Builds the pairing page opened via Pebble.openURL() from showConfiguration.
// Mirrors config/pairing.html, but inlined as a data: URI instead of hosted,
// since this project has no server of its own and openURL() has no way to
// load a file bundled inside the .pbw. data: URIs don't get a real query
// string the way https URLs do, so baseUrl/email are templated directly
// into the markup instead of read from location.search.
'use strict';

function escapeHtmlAttr(s) {
  return String(s)
    .replace(/&/g, '&amp;')
    .replace(/"/g, '&quot;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;');
}

function buildPairingPageUrl(baseUrl, email) {
  var html = '<!doctype html>\n' +
'<html lang="en">\n' +
'<head>\n' +
'<meta charset="utf-8">\n' +
'<meta name="viewport" content="width=device-width, initial-scale=1">\n' +
'<title>Pair with SuperSync</title>\n' +
'<style>\n' +
'  body { font-family: -apple-system, Roboto, sans-serif; margin: 0; padding: 16px; background: #fff; color: #111; }\n' +
'  h1 { font-size: 18px; }\n' +
'  label { display: block; margin-top: 14px; font-size: 13px; font-weight: 600; }\n' +
'  input { width: 100%; box-sizing: border-box; padding: 10px; font-size: 15px; margin-top: 4px; border: 1px solid #ccc; border-radius: 6px; }\n' +
'  button { width: 100%; padding: 12px; font-size: 15px; margin-top: 16px; border: none; border-radius: 6px; background: #1a73e8; color: #fff; }\n' +
'  button.secondary { background: #eee; color: #111; margin-top: 8px; }\n' +
'  p.hint { font-size: 12px; color: #666; }\n' +
'  p.error { font-size: 13px; color: #c00; }\n' +
'  p.success { font-size: 13px; color: #0a0; }\n' +
'</style>\n' +
'</head>\n' +
'<body>\n' +
'  <h1>Pair Super Productivity with your watch</h1>\n' +
'  <p class="hint">\n' +
'    This app relays over Bluetooth via your phone (Pebble watchapps have no\n' +
'    networking of their own), so pairing happens here in the phone browser,\n' +
'    not on the watch.\n' +
'  </p>\n' +
'\n' +
'  <label for="baseUrl">SuperSync server URL</label>\n' +
'  <input id="baseUrl" type="url" value="' + escapeHtmlAttr(baseUrl) + '">\n' +
'\n' +
'  <label for="email">Account email</label>\n' +
'  <input id="email" type="email" placeholder="you@example.com" value="' + escapeHtmlAttr(email) + '">\n' +
'\n' +
'  <label for="password">Sync encryption password</label>\n' +
'  <input id="password" type="password" placeholder="Only used to decrypt on this phone - never sent anywhere">\n' +
'  <p class="hint">\n' +
'    This is the end-to-end encryption password you set in Super\n' +
'    Productivity\'s sync settings (not your login password, if those\n' +
'    differ). It never leaves this device: it\'s used locally to derive the\n' +
'    AES key that decrypts your tasks.\n' +
'  </p>\n' +
'\n' +
'  <label for="jwt">SuperSync access token</label>\n' +
'  <input id="jwt" type="text" placeholder="Paste the token shown after you log in below">\n' +
'  <p class="hint">\n' +
'    SuperSync accounts are authenticated via an emailed magic link or a\n' +
'    passkey, not a password, so there\'s no login form here. Tap "Open\n' +
'    SuperSync login" below, sign in there, copy the token it shows you\n' +
'    ("Connection Successful - copy this token and paste it in Super\n' +
'    Productivity\'s sync settings"), then come back here and paste it above.\n' +
'    It\'s the same token you\'d paste into Super Productivity\'s own\n' +
'    Settings → Sync → SuperSync screen.\n' +
'  </p>\n' +
'\n' +
'  <button id="openLoginBtn" class="secondary">Open SuperSync login</button>\n' +
'  <p id="status"></p>\n' +
'\n' +
'  <button id="saveBtn">Save &amp; sync</button>\n' +
'  <button id="cancelBtn" class="secondary">Cancel</button>\n' +
'\n' +
'<script>\n' +
'(function () {\n' +
'  function setStatus(msg, isError) {\n' +
'    var el = document.getElementById(\'status\');\n' +
'    el.textContent = msg;\n' +
'    el.className = isError ? \'error\' : \'success\';\n' +
'  }\n' +
'\n' +
'  function returnToWatchApp(data) {\n' +
'    var encoded = encodeURIComponent(JSON.stringify(data));\n' +
'    location.href = \'pebblejs://close#\' + encoded;\n' +
'  }\n' +
'\n' +
'  document.getElementById(\'openLoginBtn\').addEventListener(\'click\', function () {\n' +
'    var baseUrl = document.getElementById(\'baseUrl\').value.replace(/\\/+$/, \'\');\n' +
'    // sync.super-productivity.com\'s own root page is the login/"Connect"\n' +
'    // portal that issues the copyable token - no custom auth flow needed.\n' +
'    window.open(baseUrl + \'/\', \'_blank\');\n' +
'    setStatus(\'Log in there, copy the token it shows you, then paste it below.\', false);\n' +
'  });\n' +
'\n' +
'  document.getElementById(\'saveBtn\').addEventListener(\'click\', function () {\n' +
'    var jwt = document.getElementById(\'jwt\').value.trim();\n' +
'    if (!jwt) {\n' +
'      setStatus(\'Paste your SuperSync token first.\', true);\n' +
'      return;\n' +
'    }\n' +
'    returnToWatchApp({\n' +
'      baseUrl: document.getElementById(\'baseUrl\').value.replace(/\\/+$/, \'\'),\n' +
'      email: document.getElementById(\'email\').value.trim(),\n' +
'      password: document.getElementById(\'password\').value,\n' +
'      jwt: jwt\n' +
'    });\n' +
'  });\n' +
'\n' +
'  document.getElementById(\'cancelBtn\').addEventListener(\'click\', function () {\n' +
'    returnToWatchApp({ cancelled: true });\n' +
'  });\n' +
'})();\n' +
'</script>\n' +
'</body>\n' +
'</html>\n';

  return 'data:text/html;charset=utf-8,' + encodeURIComponent(html);
}

module.exports = {
  buildPairingPageUrl: buildPairingPageUrl
};
