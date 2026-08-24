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

function buildPairingPageUrl(baseUrl, email, options) {
  options = options || {};
  var groupByProject = !!options.groupByProject;
  var todayOnly = !!options.todayOnly;
  var hideDoneTasks = !!options.hideDoneTasks;
  var autoMarkParentDone = !!options.autoMarkParentDone;
  var autoSyncOnComplete = !!options.autoSyncOnComplete;
  var autoSyncIntervalMin = options.autoSyncIntervalMin || 0;
  var hasPassword = !!options.hasPassword;
  var hasToken = !!options.hasToken;
  var defaultProjectId = options.defaultProjectId || '';
  var projects = options.projects || [];
  var enableHabits = options.enableHabits !== false;
  var enableAddTask = options.enableAddTask !== false;
  var habitSortDoneLast = !!options.habitSortDoneLast;
  var hideDoneHabits = !!options.hideDoneHabits;
  var backlightMode = options.backlightMode || 0;
  var passwordPlaceholder = hasPassword
    ? 'Already saved - leave blank to keep it'
    : 'Only used to decrypt on this phone - never sent anywhere';
  var jwtPlaceholder = hasToken
    ? 'Already saved - leave blank to keep it'
    : 'Paste the token shown after you log in below';
  // Project titles are arbitrary user text (unlike every other value
  // templated into this page so far) - must be escaped same as
  // baseUrl/email, or a title containing a stray '"' or '<' could break out
  // of the <option> markup.
  var projectOptions = projects.map(function (p) {
    var selected = p.id === defaultProjectId ? ' selected' : '';
    return '<option value="' + escapeHtmlAttr(p.id) + '"' + selected + '>' + escapeHtmlAttr(p.title) + '</option>';
  }).join('\n');
  var autoSyncIntervalOptions = [
    [0, 'Off'],
    [5, 'Every 5 minutes'],
    [15, 'Every 15 minutes'],
    [30, 'Every 30 minutes'],
    [60, 'Every hour'],
  ].map(function (opt) {
    var selected = opt[0] === autoSyncIntervalMin ? ' selected' : '';
    return '<option value="' + opt[0] + '"' + selected + '>' + opt[1] + '</option>';
  }).join('\n');
  var backlightOptions = [
    [0, 'System default'],
    [5, '5 seconds after a button press'],
    [15, '15 seconds after a button press'],
    [30, '30 seconds after a button press'],
    [60, '60 seconds after a button press'],
    [-1, 'Always on (uses much more battery)'],
  ].map(function (opt) {
    var selected = opt[0] === backlightMode ? ' selected' : '';
    return '<option value="' + opt[0] + '"' + selected + '>' + opt[1] + '</option>';
  }).join('\n');
  var html = '<!doctype html>\n' +
'<html lang="en">\n' +
'<head>\n' +
'<meta charset="utf-8">\n' +
'<meta name="viewport" content="width=device-width, initial-scale=1">\n' +
'<title>Pair with SuperSync</title>\n' +
'<style>\n' +
'  body { font-family: -apple-system, Roboto, sans-serif; margin: 0; padding: 16px; background: #fff; color: #111; }\n' +
'  h1 { font-size: 18px; }\n' +
'  h2 { font-size: 15px; margin-top: 24px; }\n' +
'  label { display: block; margin-top: 14px; font-size: 13px; font-weight: 600; }\n' +
'  input, select { width: 100%; box-sizing: border-box; padding: 10px; font-size: 15px; margin-top: 4px; border: 1px solid #ccc; border-radius: 6px; }\n' +
'  .checkbox-row { display: flex; align-items: center; gap: 8px; margin-top: 14px; }\n' +
'  .checkbox-row input { width: auto; margin: 0; }\n' +
'  .checkbox-row label { display: inline; margin: 0; font-weight: normal; }\n' +
'  button { width: 100%; padding: 12px; font-size: 15px; margin-top: 16px; border: none; border-radius: 6px; background: #1a73e8; color: #fff; }\n' +
'  button.secondary { background: #eee; color: #111; margin-top: 8px; }\n' +
'  button.danger { background: #fff; color: #c00; border: 1px solid #c00; }\n' +
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
'  <input id="password" type="password" placeholder="' + escapeHtmlAttr(passwordPlaceholder) + '">\n' +
'  <p class="hint">\n' +
'    This is the end-to-end encryption password you set in Super\n' +
'    Productivity\'s sync settings (not your login password, if those\n' +
'    differ). It never leaves this device: it\'s used locally to derive the\n' +
'    AES key that decrypts your tasks.' + (hasPassword ? ' Already saved on this\n' +
'    watch/phone - only re-enter it here if it changed.' : '') + '\n' +
'  </p>\n' +
'\n' +
'  <label for="jwt">SuperSync access token</label>\n' +
'  <input id="jwt" type="text" placeholder="' + escapeHtmlAttr(jwtPlaceholder) + '">\n' +
'  <p class="hint">\n' +
'    SuperSync accounts are authenticated via an emailed magic link or a\n' +
'    passkey, not a password, so there\'s no login form here. Tap "Open\n' +
'    SuperSync login" below, sign in there, copy the token it shows you\n' +
'    ("Connection Successful - copy this token and paste it in Super\n' +
'    Productivity\'s sync settings"), then come back here and paste it above.\n' +
'    It\'s the same token you\'d paste into Super Productivity\'s own\n' +
'    Settings → Sync → SuperSync screen.' + (hasToken ? ' Already saved on this\n' +
'    watch/phone - only re-enter it here if it changed or expired.' : '') + '\n' +
'  </p>\n' +
'\n' +
'  <button id="openLoginBtn" class="secondary">Open SuperSync login</button>\n' +
'  <p id="status"></p>\n' +
'\n' +
'  <h2>Watch display</h2>\n' +
'  <div class="checkbox-row">\n' +
'    <input id="groupByProject" type="checkbox"' + (groupByProject ? ' checked' : '') + '>\n' +
'    <label for="groupByProject">Group tasks by project</label>\n' +
'  </div>\n' +
'  <p class="hint">\n' +
'    Shows a bold, underlined project name on a green header above each\n' +
'    group of tasks on the watch, instead of one flat list.\n' +
'  </p>\n' +
'\n' +
'  <div class="checkbox-row">\n' +
'    <input id="todayOnly" type="checkbox"' + (todayOnly ? ' checked' : '') + '>\n' +
'    <label for="todayOnly">Only show today\'s tasks</label>\n' +
'  </div>\n' +
'  <p class="hint">\n' +
'    Only shows tasks planned for today - hides undated, overdue, and\n' +
'    future-dated tasks.\n' +
'  </p>\n' +
'\n' +
'  <div class="checkbox-row">\n' +
'    <input id="hideDoneTasks" type="checkbox"' + (hideDoneTasks ? ' checked' : '') + '>\n' +
'    <label for="hideDoneTasks">Hide completed tasks</label>\n' +
'  </div>\n' +
'  <p class="hint">\n' +
'    Removes a completed task from the watch\'s list entirely instead of\n' +
'    showing it dimmed. A completed subtask under a still-open task is\n' +
'    hidden the same way; a completed task with subtasks is hidden along\n' +
'    with all of them. A task you complete on the watch itself stays\n' +
'    visible for about 5 seconds first, so you can see it happen before it\n' +
'    disappears.\n' +
'  </p>\n' +
'\n' +
'  <div class="checkbox-row">\n' +
'    <input id="autoMarkParentDone" type="checkbox"' + (autoMarkParentDone ? ' checked' : '') + '>\n' +
'    <label for="autoMarkParentDone">Complete main task when all subtasks are done</label>\n' +
'  </div>\n' +
'  <p class="hint">\n' +
'    Completing the last remaining open subtask (from the watch) also\n' +
'    completes its main task, matching Super Productivity\'s own "Automatically\n' +
'    mark parent task done" setting. Only fires forward - undoing a subtask\n' +
'    never reopens an already-completed main task.\n' +
'  </p>\n' +
'\n' +
'  <label for="backlightMode">Backlight</label>\n' +
'  <select id="backlightMode">\n' +
backlightOptions + '\n' +
'  </select>\n' +
'  <p class="hint">\n' +
'    "System default" never touches the backlight - it behaves exactly as\n' +
'    your watch\'s own Settings say. Any other option overrides that while\n' +
'    this app is open, relighting the screen on every button press (Select,\n' +
'    long-select, or scrolling) and keeping it lit for the chosen duration\n' +
'    (or indefinitely, for "Always on") before handing control back. Not\n' +
'    available on original Pebble/Pebble Steel (aplite) - too little free\n' +
'    memory left on that hardware for this; it always uses your watch\'s\n' +
'    own Settings regardless of this option.\n' +
'  </p>\n' +
'\n' +
'  <label for="defaultProjectId">Default project for "Add Task"</label>\n' +
'  <select id="defaultProjectId">\n' +
'    <option value="">Inbox (default)</option>\n' +
projectOptions + '\n' +
'  </select>\n' +
'  <p class="hint">\n' +
'    A task you dictate on the watch (the "Add Task" row, on watches with a\n' +
'    microphone) is filed into this project.\n' +
'  </p>\n' +
'\n' +
'  <h2>Features</h2>\n' +
'  <div class="checkbox-row">\n' +
'    <input id="enableHabits" type="checkbox"' + (enableHabits ? ' checked' : '') + '>\n' +
'    <label for="enableHabits">Enable Habits</label>\n' +
'  </div>\n' +
'  <p class="hint">\n' +
'    Shows the Habits row on the watch and syncs habit data to it. Turn off\n' +
'    if you don\'t use Super Productivity\'s habit tracking.\n' +
'  </p>\n' +
'\n' +
'  <div class="checkbox-row">\n' +
'    <input id="habitSortDoneLast" type="checkbox"' + (habitSortDoneLast ? ' checked' : '') + '>\n' +
'    <label for="habitSortDoneLast">Show completed habits last</label>\n' +
'  </div>\n' +
'  <p class="hint">\n' +
'    By default habits sort alphabetically regardless of today\'s progress.\n' +
'    Turn this on to group not-yet-done habits before done ones (each group\n' +
'    still alphabetical) - a habit\'s position will jump once it hits its\n' +
'    goal for the day.\n' +
'  </p>\n' +
'\n' +
'  <div class="checkbox-row">\n' +
'    <input id="hideDoneHabits" type="checkbox"' + (hideDoneHabits ? ' checked' : '') + '>\n' +
'    <label for="hideDoneHabits">Hide completed habits</label>\n' +
'  </div>\n' +
'  <p class="hint">\n' +
'    Removes a habit from the watch\'s list entirely once it hits its goal\n' +
'    for the day, instead of just showing it as done. Makes "Show completed\n' +
'    habits last" above moot, since there\'s nothing done left to sort.\n' +
'  </p>\n' +
'\n' +
'  <div class="checkbox-row">\n' +
'    <input id="enableAddTask" type="checkbox"' + (enableAddTask ? ' checked' : '') + '>\n' +
'    <label for="enableAddTask">Enable Add Task (voice)</label>\n' +
'  </div>\n' +
'  <p class="hint">\n' +
'    Shows the microphone "Add Task" row on watches with a microphone.\n' +
'    Watches without a microphone never show this row regardless of this\n' +
'    setting.\n' +
'  </p>\n' +
'\n' +
'  <h2>Sync</h2>\n' +
'  <div class="checkbox-row">\n' +
'    <input id="autoSyncOnComplete" type="checkbox"' + (autoSyncOnComplete ? ' checked' : '') + '>\n' +
'    <label for="autoSyncOnComplete">Sync automatically after a watch change</label>\n' +
'  </div>\n' +
'  <p class="hint">\n' +
'    Pulls the latest changes from the server right after you complete a\n' +
'    task, track time, adjust a habit, or add a task on the watch, instead\n' +
'    of waiting for the next manual Resync or app launch. Uses a bit more\n' +
'    battery/data per action.\n' +
'  </p>\n' +
'\n' +
'  <label for="autoSyncIntervalMin">Sync automatically on a timer</label>\n' +
'  <select id="autoSyncIntervalMin">\n' +
autoSyncIntervalOptions + '\n' +
'  </select>\n' +
'  <p class="hint">\n' +
'    Keeps the watch\'s list fresh on this schedule even when the app isn\'t\n' +
'    open, in addition to any manual Resync and the "after a watch change"\n' +
'    option above. While the app IS open, this runs quietly on the phone\n' +
'    side; while it\'s closed, the watch briefly wakes itself up, syncs,\n' +
'    and returns you to whatever was on screen before - expect a short\n' +
'    screen flash each time, not a fully invisible background refresh.\n' +
'    Uses more battery the more often it\'s set to run.\n' +
'  </p>\n' +
'\n' +
'  <h2>Danger zone</h2>\n' +
'  <p class="hint">\n' +
'    Wipes this watch/phone\'s locally cached task list and resync position,\n' +
'    then re-downloads everything from the server from scratch. Your\n' +
'    SuperSync account, saved token, and password are untouched - use this\n' +
'    if the watch\'s list ever looks stuck or out of sync, not to unpair.\n' +
'  </p>\n' +
'  <button id="clearDataBtn" class="danger">Clear all data &amp; resync</button>\n' +
'\n' +
'  <button id="saveBtn">Save &amp; sync</button>\n' +
'  <button id="cancelBtn" class="secondary">Cancel</button>\n' +
'\n' +
'<script>\n' +
'(function () {\n' +
'  var hasToken = ' + (hasToken ? 'true' : 'false') + ';\n' +
'\n' +
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
'    // A never-paired watch has no saved token to fall back to, so a token\n' +
'    // is required then; once one is saved, leaving this field blank just\n' +
'    // means "keep the one already on the phone" (see webviewclosed in\n' +
'    // index.js), so settings-only changes don\'t force re-pasting it.\n' +
'    if (!jwt && !hasToken) {\n' +
'      setStatus(\'Paste your SuperSync token first.\', true);\n' +
'      return;\n' +
'    }\n' +
'    returnToWatchApp({\n' +
'      baseUrl: document.getElementById(\'baseUrl\').value.replace(/\\/+$/, \'\'),\n' +
'      email: document.getElementById(\'email\').value.trim(),\n' +
'      password: document.getElementById(\'password\').value,\n' +
'      jwt: jwt,\n' +
'      groupByProject: document.getElementById(\'groupByProject\').checked,\n' +
'      todayOnly: document.getElementById(\'todayOnly\').checked,\n' +
'      hideDoneTasks: document.getElementById(\'hideDoneTasks\').checked,\n' +
'      autoMarkParentDone: document.getElementById(\'autoMarkParentDone\').checked,\n' +
'      autoSyncOnComplete: document.getElementById(\'autoSyncOnComplete\').checked,\n' +
'      autoSyncIntervalMin: parseInt(document.getElementById(\'autoSyncIntervalMin\').value, 10) || 0,\n' +
'      defaultProjectId: document.getElementById(\'defaultProjectId\').value,\n' +
'      enableHabits: document.getElementById(\'enableHabits\').checked,\n' +
'      enableAddTask: document.getElementById(\'enableAddTask\').checked,\n' +
'      habitSortDoneLast: document.getElementById(\'habitSortDoneLast\').checked,\n' +
'      hideDoneHabits: document.getElementById(\'hideDoneHabits\').checked,\n' +
'      backlightMode: parseInt(document.getElementById(\'backlightMode\').value, 10) || 0\n' +
'    });\n' +
'  });\n' +
'\n' +
'  document.getElementById(\'clearDataBtn\').addEventListener(\'click\', function () {\n' +
'    if (!window.confirm(\'Wipe the cached task list on this watch/phone and re-download everything from the server? This does not affect your account.\')) {\n' +
'      return;\n' +
'    }\n' +
'    returnToWatchApp({ clearData: true });\n' +
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
