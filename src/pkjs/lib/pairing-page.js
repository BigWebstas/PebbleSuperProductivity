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
  var laterToday = !!options.laterToday;
  var todayOnly = !!options.todayOnly;
  var hideDoneTasks = !!options.hideDoneTasks;
  var autoMarkParentDone = !!options.autoMarkParentDone;
  var autoSyncOnComplete = !!options.autoSyncOnComplete;
  var hasPassword = !!options.hasPassword;
  var hasToken = !!options.hasToken;
  var defaultProjectId = options.defaultProjectId || '';
  var defaultTaskEstimateMin = options.defaultTaskEstimateMin || 0;
  var projects = options.projects || [];
  var enableHabits = options.enableHabits !== false;
  var habitStreakNudge = !!options.habitStreakNudge;
  var enableReflect = !!options.enableReflect;
  var enableAddTask = options.enableAddTask !== false;
  var enableProjects = options.enableProjects !== false;
  var enableStats = options.enableStats !== false;
  var yesterdayStats = !!options.yesterdayStats;
  var statsMarkdown = options.statsMarkdown || '';
  var showStatsExport = enableStats && !!statsMarkdown;
  var enableSchedule = options.enableSchedule !== false;
  var enableUpcoming = options.enableUpcoming !== false;
  var enableNotesPage = !!options.enableNotesPage;
  var enableSearch = !!options.enableSearch;
  var enableTags = options.enableTags === true; // default off, unlike the others
  var touchNav = !!options.touchNav;
  var overtimeNotify = !!options.overtimeNotify;
  var overtimeRepeat = !!options.overtimeRepeat;
  var audibleNotifications = options.audibleNotifications != null
    ? !!options.audibleNotifications : !!options.overtimeSound;
  var audibleVolume = options.audibleVolume != null ? options.audibleVolume : 80;
  var breakReminderMin = options.breakReminderMin || 0;
  var idleReminderMin = options.idleReminderMin || 0;
  var dueReminderMin = options.dueReminderMin || 0;
  var liveTracking = !!options.liveTracking;
  var stopAtMidnight = !!options.stopAtMidnight;
  var enableTimeline = !!options.enableTimeline;
  var focusLenMin = options.focusLenMin || 25;
  var focusType = options.focusType || (options.usePomodoroCfg ? 'pomodoro' : 'countdown');
  var appVersion = options.appVersion || '';
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
  var taskEstimateOptions = [
    [0, 'None'],
    [5, '5 minutes'],
    [10, '10 minutes'],
    [15, '15 minutes'],
    [30, '30 minutes'],
    [45, '45 minutes'],
    [60, '1 hour'],
    [120, '2 hours'],
  ].map(function (opt) {
    var selected = opt[0] === defaultTaskEstimateMin ? ' selected' : '';
    return '<option value="' + opt[0] + '"' + selected + '>' + opt[1] + '</option>';
  }).join('\n');
  var dueReminderOptions = [
    [0, 'Off'],
    [5, '5 minutes before'],
    [10, '10 minutes before'],
    [15, '15 minutes before'],
    [30, '30 minutes before'],
  ].map(function (opt) {
    var selected = opt[0] === dueReminderMin ? ' selected' : '';
    return '<option value="' + opt[0] + '"' + selected + '>' + opt[1] + '</option>';
  }).join('\n');
  var breakReminderOptions = [
    [0, 'Off'],
    [30, 'Every 30 minutes of tracking'],
    [45, 'Every 45 minutes of tracking'],
    [60, 'Every hour of tracking'],
    [90, 'Every 90 minutes of tracking'],
  ].map(function (opt) {
    var selected = opt[0] === breakReminderMin ? ' selected' : '';
    return '<option value="' + opt[0] + '"' + selected + '>' + opt[1] + '</option>';
  }).join('\n');
  var idleReminderOptions = [
    [0, 'Off'],
    [10, 'Every 10 minutes not tracking'],
    [20, 'Every 20 minutes not tracking'],
    [30, 'Every 30 minutes not tracking'],
    [60, 'Every 60 minutes not tracking'],
  ].map(function (opt) {
    var selected = opt[0] === idleReminderMin ? ' selected' : '';
    return '<option value="' + opt[0] + '"' + selected + '>' + opt[1] + '</option>';
  }).join('\n');
  var focusLenOptions = [
    [10, '10 minutes'],
    [15, '15 minutes'],
    [20, '20 minutes'],
    [25, '25 minutes'],
    [30, '30 minutes'],
    [45, '45 minutes'],
    [60, '60 minutes'],
  ].map(function (opt) {
    var selected = opt[0] === focusLenMin ? ' selected' : '';
    return '<option value="' + opt[0] + '"' + selected + '>' + opt[1] + '</option>';
  }).join('\n');
  var focusTypeOptions = [
    ['countdown', 'Countdown - one session, buzz at zero'],
    ['pomodoro', 'Pomodoro - work / break, looping'],
    ['flowtime', 'Flowtime - counts up, you end it'],
  ].map(function (opt) {
    var selected = opt[0] === focusType ? ' selected' : '';
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
'<meta name="color-scheme" content="light dark">\n' +
'<title>Pair with SuperSync</title>\n' +
'<style>\n' +
'  :root { --bg: #fff; --fg: #111; --hint: #666; --field-bg: #fff; --field-border: #ccc;\n' +
'          --btn-2-bg: #eee; --btn-2-fg: #111; --danger: #c00; --danger-bg: #fff; --ok: #0a0; }\n' +
'  @media (prefers-color-scheme: dark) {\n' +
'    :root { --bg: #1c1c1e; --fg: #e6e6e9; --hint: #9a9aa0; --field-bg: #2c2c2e; --field-border: #48484a;\n' +
'            --btn-2-bg: #2c2c2e; --btn-2-fg: #e6e6e9; --danger: #ff6b6b; --danger-bg: #1c1c1e; --ok: #4ade80; }\n' +
'  }\n' +
'  body { font-family: -apple-system, Roboto, sans-serif; margin: 0; padding: 16px; background: var(--bg); color: var(--fg); }\n' +
'  h1 { font-size: 18px; }\n' +
'  h2 { font-size: 15px; margin-top: 24px; }\n' +
'  label { display: block; margin-top: 14px; font-size: 13px; font-weight: 600; }\n' +
'  input, select { width: 100%; box-sizing: border-box; padding: 10px; font-size: 15px; margin-top: 4px; border: 1px solid var(--field-border); border-radius: 6px; background: var(--field-bg); color: var(--fg); }\n' +
'  .checkbox-row { display: flex; align-items: center; gap: 8px; margin-top: 14px; }\n' +
'  .checkbox-row input { width: auto; margin: 0; }\n' +
'  .checkbox-row label { display: inline; margin: 0; font-weight: normal; }\n' +
'  .checkbox-row.sub { margin-left: 26px; margin-top: 8px; }\n' +
'  p.hint.sub, label.sub { margin-left: 26px; }\n' +
'  select.sub { width: calc(100% - 26px); margin-left: 26px; }\n' +
'  input[type=range] { padding: 0; border: none; background: none; width: calc(100% - 26px); margin-left: 26px; }\n' +
'  input:disabled, input[type=range]:disabled { opacity: 0.4; }\n' +
'  button { width: 100%; padding: 12px; font-size: 15px; margin-top: 16px; border: none; border-radius: 6px; background: #1a73e8; color: #fff; }\n' +
'  button.secondary { background: var(--btn-2-bg); color: var(--btn-2-fg); margin-top: 8px; }\n' +
'  button.danger { background: var(--danger-bg); color: var(--danger); border: 1px solid var(--danger); }\n' +
'  p.hint { font-size: 12px; color: var(--hint); }\n' +
'  p.error { font-size: 13px; color: var(--danger); }\n' +
'  p.success { font-size: 13px; color: var(--ok); }\n' +
'  textarea { width: 100%; box-sizing: border-box; height: 160px; margin-top: 4px; padding: 10px;\n' +
'    font-family: ui-monospace, Menlo, Consolas, monospace; font-size: 12px; white-space: pre;\n' +
'    border: 1px solid var(--field-border); border-radius: 6px; background: var(--field-bg); color: var(--fg); }\n' +
'</style>\n' +
'</head>\n' +
'<body>\n' +
'  <h1>Pair Super Productivity with your watch</h1>\n' +
'  <p class="hint">\n' +
'    Pebble watchapps have no network of their own, so pairing happens here\n' +
'    in the phone browser and syncs over Bluetooth.\n' +
'  </p>\n' +
'\n' +
'  <h2>Account</h2>\n' +
'  <label for="baseUrl">SuperSync server URL</label>\n' +
'  <input id="baseUrl" type="url" value="' + escapeHtmlAttr(baseUrl) + '">\n' +
'\n' +
'  <label for="email">Account email</label>\n' +
'  <input id="email" type="email" placeholder="you@example.com" value="' + escapeHtmlAttr(email) + '">\n' +
'\n' +
'  <label for="password">Sync encryption password</label>\n' +
'  <input id="password" type="password" placeholder="' + escapeHtmlAttr(passwordPlaceholder) + '">\n' +
'  <p class="hint">\n' +
'    The end-to-end encryption password from Super Productivity\'s sync\n' +
'    settings. Stays on this device - used to derive the key that decrypts\n' +
'    your tasks.' + (hasPassword ? ' Already saved; re-enter only if it changed.' : '') + '\n' +
'  </p>\n' +
'\n' +
'  <label for="jwt">SuperSync access token</label>\n' +
'  <input id="jwt" type="text" placeholder="' + escapeHtmlAttr(jwtPlaceholder) + '">\n' +
'  <p class="hint">\n' +
'    SuperSync logs in by magic link / passkey, not a password. Tap "Open\n' +
'    SuperSync login", sign in, copy the token it shows, and paste it\n' +
'    above.' + (hasToken ? ' Already saved; re-enter only if it changed or expired.' : '') + '\n' +
'  </p>\n' +
'\n' +
'  <button id="openLoginBtn" class="secondary">Open SuperSync login</button>\n' +
'  <p id="status"></p>\n' +
'\n' +
'  <h2>Watch list</h2>\n' +
'  <div class="checkbox-row">\n' +
'    <input id="groupByProject" type="checkbox"' + (groupByProject ? ' checked' : '') + '>\n' +
'    <label for="groupByProject">Group tasks by project</label>\n' +
'  </div>\n' +
'  <p class="hint">Green project headers above each group, instead of one flat list.</p>\n' +
'\n' +
'  <div class="checkbox-row">\n' +
'    <input id="laterToday" type="checkbox"' + (laterToday ? ' checked' : '') + '>\n' +
'    <label for="laterToday">Later Today section</label>\n' +
'  </div>\n' +
'  <p class="hint">With grouping on, tasks scheduled later than right now move into one group at the bottom, sorted by time - like the desktop.</p>\n' +
'\n' +
'  <div class="checkbox-row">\n' +
'    <input id="todayOnly" type="checkbox"' + (todayOnly ? ' checked' : '') + '>\n' +
'    <label for="todayOnly">Only show today\'s tasks</label>\n' +
'  </div>\n' +
'  <p class="hint">Hide undated, overdue and future-dated tasks.</p>\n' +
'\n' +
'  <div class="checkbox-row">\n' +
'    <input id="hideDoneTasks" type="checkbox"' + (hideDoneTasks ? ' checked' : '') + '>\n' +
'    <label for="hideDoneTasks">Hide completed tasks</label>\n' +
'  </div>\n' +
'  <p class="hint">Drop completed tasks instead of dimming them. One you complete on the watch lingers ~10s first.</p>\n' +
'\n' +
'  <div class="checkbox-row">\n' +
'    <input id="autoMarkParentDone" type="checkbox"' + (autoMarkParentDone ? ' checked' : '') + '>\n' +
'    <label for="autoMarkParentDone">Complete main task when all subtasks are done</label>\n' +
'  </div>\n' +
'  <p class="hint">Completing the last open subtask completes its parent. Never reopens one.</p>\n' +
'\n' +
'  <h2>Extra rows</h2>\n' +
'  <p class="hint">Optional rows on the watch\'s main list. Not available on aplite.</p>\n' +
'  <div class="checkbox-row">\n' +
'    <input id="enableHabits" type="checkbox"' + (enableHabits ? ' checked' : '') + '>\n' +
'    <label for="enableHabits">Habits</label>\n' +
'  </div>\n' +
'  <p class="hint">Habit tracking synced to the watch.</p>\n' +
'\n' +
'  <div class="checkbox-row">\n' +
'    <input id="enableProjects" type="checkbox"' + (enableProjects ? ' checked' : '') + '>\n' +
'    <label for="enableProjects">Projects</label>\n' +
'  </div>\n' +
'  <p class="hint">Browse every project\'s tasks and backlog.</p>\n' +
'\n' +
'  <div class="checkbox-row">\n' +
'    <input id="enableStats" type="checkbox"' + (enableStats ? ' checked' : '') + '>\n' +
'    <label for="enableStats">Stats</label>\n' +
'  </div>\n' +
'  <p class="hint">Time remaining and worked today, current session, per-project counts.</p>\n' +
'  <div class="checkbox-row sub">\n' +
'    <input id="yesterdayStats" type="checkbox"' + (yesterdayStats ? ' checked' : '') + '>\n' +
'    <label for="yesterdayStats">Include yesterday</label>\n' +
'  </div>\n' +
'  <p class="hint sub">Hold Up / Down on the Stats page to flip to yesterday.</p>\n' +
'\n' +
'  <div class="checkbox-row">\n' +
'    <input id="enableSchedule" type="checkbox"' + (enableSchedule ? ' checked' : '') + '>\n' +
'    <label for="enableSchedule">Schedule</label>\n' +
'  </div>\n' +
'  <p class="hint">Today\'s timed tasks in time order. Select toggles done, long-press tracks.</p>\n' +
'\n' +
'  <div class="checkbox-row">\n' +
'    <input id="enableUpcoming" type="checkbox"' + (enableUpcoming ? ' checked' : '') + '>\n' +
'    <label for="enableUpcoming">Upcoming</label>\n' +
'  </div>\n' +
'  <p class="hint">Future-dated tasks grouped by day.</p>\n' +
'\n' +
'  <div class="checkbox-row">\n' +
'    <input id="enableNotesPage" type="checkbox"' + (enableNotesPage ? ' checked' : '') + '>\n' +
'    <label for="enableNotesPage">Notes</label>\n' +
'  </div>\n' +
'  <p class="hint">Today-pinned notes. Long-Select dictates an append.</p>\n' +
'\n' +
'  <div class="checkbox-row">\n' +
'    <input id="enableTags" type="checkbox"' + (enableTags ? ' checked' : '') + '>\n' +
'    <label for="enableTags">Tags</label>\n' +
'  </div>\n' +
'  <p class="hint">Each tag with its open-task count. Off by default.</p>\n' +
'\n' +
'  <div class="checkbox-row">\n' +
'    <input id="enableReflect" type="checkbox"' + (enableReflect ? ' checked' : '') + '>\n' +
'    <label for="enableReflect">Day review on Finish Day</label>\n' +
'  </div>\n' +
'  <p class="hint">Select on Finish Day for a quick energy / rating / improvement review, synced to the desktop. Long-press still archives.</p>\n' +
'\n' +
'  <h2>Voice</h2>\n' +
'  <p class="hint">Dictation rows - microphone watches only (Pebble Time 2).</p>\n' +
'  <div class="checkbox-row">\n' +
'    <input id="enableAddTask" type="checkbox"' + (enableAddTask ? ' checked' : '') + '>\n' +
'    <label for="enableAddTask">Add Task</label>\n' +
'  </div>\n' +
'  <p class="hint">Dictate a new task from the watch.</p>\n' +
'  <label for="defaultProjectId" class="sub">New task goes to</label>\n' +
'  <select class="sub" id="defaultProjectId">\n' +
'    <option value="">Inbox (default)</option>\n' +
projectOptions + '\n' +
'  </select>\n' +
'  <label for="defaultTaskEstimateMin" class="sub">New task estimate</label>\n' +
'  <select class="sub" id="defaultTaskEstimateMin">\n' +
taskEstimateOptions + '\n' +
'  </select>\n' +
'\n' +
'  <div class="checkbox-row">\n' +
'    <input id="enableSearch" type="checkbox"' + (enableSearch ? ' checked' : '') + '>\n' +
'    <label for="enableSearch">Search</label>\n' +
'  </div>\n' +
'  <p class="hint">Dictate a few words; the phone searches every task\'s title (all projects, backlog, future, done) and lists matches with their project.</p>\n' +
'\n' +
'  <h2>Time tracking &amp; focus</h2>\n' +
'  <label for="focusType">Focus timer</label>\n' +
'  <select id="focusType">\n' +
focusTypeOptions + '\n' +
'  </select>\n' +
'  <p class="hint">Hold Up / Down on the tracking page to start / end a session. Countdown uses the length below; Pomodoro loops your desktop work / break; Flowtime counts up. Not on aplite.</p>\n' +
'  <label for="focusLenMin" class="sub">Countdown length</label>\n' +
'  <select class="sub" id="focusLenMin">\n' +
focusLenOptions + '\n' +
'  </select>\n' +
'\n' +
'  <div class="checkbox-row">\n' +
'    <input id="overtimeNotify" type="checkbox"' + (overtimeNotify ? ' checked' : '') + '>\n' +
'    <label for="overtimeNotify">Notify when a task runs over its estimate</label>\n' +
'  </div>\n' +
'  <p class="hint">Buzz + banner the moment tracked time reaches the estimate. Not on aplite.</p>\n' +
'  <div class="checkbox-row sub">\n' +
'    <input id="overtimeRepeat" type="checkbox"' + (overtimeRepeat ? ' checked' : '') + (overtimeNotify ? '' : ' disabled') + '>\n' +
'    <label for="overtimeRepeat">Repeat every 5 minutes</label>\n' +
'  </div>\n' +
'  <p class="hint sub">Keep buzzing while the tracked task stays over. Stops when you stop tracking.</p>\n' +
'\n' +
'  <label for="breakReminderMin">Remind me to take a break</label>\n' +
'  <select id="breakReminderMin">\n' +
breakReminderOptions + '\n' +
'  </select>\n' +
'  <p class="hint">Buzz once watch-tracked time reaches this without a 5-min pause. App-open only. Not on aplite.</p>\n' +
'\n' +
'  <label for="idleReminderMin">Remind me when idle</label>\n' +
'  <select id="idleReminderMin">\n' +
idleReminderOptions + '\n' +
'  </select>\n' +
'  <p class="hint">Buzz "not tracking N min" after this long idle, then every interval. App-open only. Not on aplite.</p>\n' +
'\n' +
'  <div class="checkbox-row">\n' +
'    <input id="stopAtMidnight" type="checkbox"' + (stopAtMidnight ? ' checked' : '') + '>\n' +
'    <label for="stopAtMidnight">Stop tracking at midnight</label>\n' +
'  </div>\n' +
'  <p class="hint">Stop a task or habit timer running past local midnight; a watch timer logs only up to 00:00.</p>\n' +
'\n' +
'  <div class="checkbox-row">\n' +
'    <input id="liveTracking" type="checkbox"' + (liveTracking ? ' checked' : '') + '>\n' +
'    <label for="liveTracking">Show live tracking from other devices</label>\n' +
'  </div>\n' +
'  <p class="hint">See what you\'re tracking on desktop / phone in the pinned TRACKING section, and stop it from the watch. SuperSync only, app-open only. Not on aplite.</p>\n' +
'\n' +
'  <h2>Notifications</h2>\n' +
'  <label for="dueReminderMin">Notify before a task is due</label>\n' +
'  <select id="dueReminderMin">\n' +
dueReminderOptions + '\n' +
'  </select>\n' +
'  <p class="hint">Buzz "Due at ..." this far before the next timed task. One per task time, app-open only. Not on aplite.</p>\n' +
'\n' +
'  <div class="checkbox-row">\n' +
'    <input id="habitStreakNudge" type="checkbox"' + (habitStreakNudge ? ' checked' : '') + '>\n' +
'    <label for="habitStreakNudge">Nudge me about unfinished streaks</label>\n' +
'  </div>\n' +
'  <p class="hint">From 6pm, once a day, buzz if a habit with a 2+ day streak still isn\'t done. Not on aplite.</p>\n' +
'\n' +
'  <div class="checkbox-row">\n' +
'    <input id="audibleNotifications" type="checkbox"' + (audibleNotifications ? ' checked' : '') + '>\n' +
'    <label for="audibleNotifications">Play a sound with banners</label>\n' +
'  </div>\n' +
'  <p class="hint">A short ping alongside every banner. Speaker on Pebble Time 2 only; respects system mute.</p>\n' +
'  <label for="audibleVolume" class="sub">Ping volume: <span id="audibleVolumeOut">' + audibleVolume + '</span></label>\n' +
'  <input class="sub" id="audibleVolume" type="range" min="0" max="100" step="5" value="' + audibleVolume + '"' + (audibleNotifications ? '' : ' disabled') + '>\n' +
'\n' +
'  <h2>Timeline</h2>\n' +
'  <div class="checkbox-row">\n' +
'    <input id="enableTimeline" type="checkbox"' + (enableTimeline ? ' checked' : '') + '>\n' +
'    <label for="enableTimeline">Add scheduled tasks to the timeline</label>\n' +
'  </div>\n' +
'  <p class="hint">A PebbleOS timeline pin for each timed task (next 2 weeks), reminding with the lead time above. Title + project reach Rebble\'s timeline unencrypted, so off by default. Needs timeline enabled in the Rebble dev portal.</p>\n' +
'\n' +
'  <h2>Appearance &amp; input</h2>\n' +
'  <label for="backlightMode">Backlight</label>\n' +
'  <select id="backlightMode">\n' +
backlightOptions + '\n' +
'  </select>\n' +
'  <p class="hint">"System default" leaves the backlight alone. Any other option overrides it while the app is open, relighting on each button press. Not on aplite.</p>\n' +
'\n' +
'  <div class="checkbox-row">\n' +
'    <input id="touchNav" type="checkbox"' + (touchNav ? ' checked' : '') + '>\n' +
'    <label for="touchNav">Touch navigation</label>\n' +
'  </div>\n' +
'  <p class="hint">Experimental Pebble Time 2 touch: swipe to scroll, tap to select, long-press to act, swipe right for back. Buttons still work. Off by default - the current touch firmware misreads edge taps.</p>\n' +
'\n' +
(showStatsExport ?
'  <h2>Stats export</h2>\n' +
'  <p class="hint">Your Stats page as Markdown + mermaid charts (today/yesterday table, week hours, project pie, habit streaks). Copy it anywhere that renders mermaid. Rebuilt each time you open this page.</p>\n' +
'  <textarea id="statsExport" readonly>' + escapeHtmlAttr(statsMarkdown) + '</textarea>\n' +
'  <button id="copyStatsBtn" class="secondary">Copy to clipboard</button>\n'
: '') +
'  <h2>Sync</h2>\n' +
'  <div class="checkbox-row">\n' +
'    <input id="autoSyncOnComplete" type="checkbox"' + (autoSyncOnComplete ? ' checked' : '') + '>\n' +
'    <label for="autoSyncOnComplete">Sync automatically after a watch change</label>\n' +
'  </div>\n' +
'  <p class="hint">Pull from the server right after a watch change, instead of waiting for a manual Resync. Uses a little more battery / data.</p>\n' +
'\n' +
'  <h2>Danger zone</h2>\n' +
'  <p class="hint">Wipe this device\'s cached list + resync position and re-download everything. Account, token and password are untouched. Use it if the list looks stuck, not to unpair.</p>\n' +
'  <button id="clearDataBtn" class="danger">Clear all data &amp; resync</button>\n' +
'\n' +
'  <p class="hint">Wipe only the watch\'s offline copy of the task / habit / project lists. It re-downloads from the phone at once. Use it if the watch keeps showing a stale list.</p>\n' +
'  <button id="wipeWatchCacheBtn" class="danger">Wipe watch cache</button>\n' +
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
'  // "Repeat every 5 minutes" only makes sense while the parent "Notify when a\n' +
'  // task runs over its estimate" is on - grey it out (and clear it) otherwise.\n' +
'  (function () {\n' +
'    var parent = document.getElementById(\'overtimeNotify\');\n' +
'    var sub = document.getElementById(\'overtimeRepeat\');\n' +
'    function sync() {\n' +
'      sub.disabled = !parent.checked;\n' +
'      if (!parent.checked) { sub.checked = false; }\n' +
'    }\n' +
'    parent.addEventListener(\'change\', sync);\n' +
'    sync();\n' +
'  })();\n' +
'\n' +
'  // Live volume readout; the slider only applies while audible notifications\n' +
'  // are on, so grey it with the checkbox.\n' +
'  (function () {\n' +
'    var toggle = document.getElementById(\'audibleNotifications\');\n' +
'    var slider = document.getElementById(\'audibleVolume\');\n' +
'    var out = document.getElementById(\'audibleVolumeOut\');\n' +
'    slider.addEventListener(\'input\', function () { out.textContent = slider.value; });\n' +
'    toggle.addEventListener(\'change\', function () { slider.disabled = !toggle.checked; });\n' +
'  })();\n' +
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
'      laterToday: document.getElementById(\'laterToday\').checked,\n' +
'      todayOnly: document.getElementById(\'todayOnly\').checked,\n' +
'      hideDoneTasks: document.getElementById(\'hideDoneTasks\').checked,\n' +
'      autoMarkParentDone: document.getElementById(\'autoMarkParentDone\').checked,\n' +
'      autoSyncOnComplete: document.getElementById(\'autoSyncOnComplete\').checked,\n' +
'      defaultProjectId: document.getElementById(\'defaultProjectId\').value,\n' +
'      defaultTaskEstimateMin: parseInt(document.getElementById(\'defaultTaskEstimateMin\').value, 10) || 0,\n' +
'      enableHabits: document.getElementById(\'enableHabits\').checked,\n' +
'      habitStreakNudge: document.getElementById(\'habitStreakNudge\').checked,\n' +
'      enableAddTask: document.getElementById(\'enableAddTask\').checked,\n' +
'      enableProjects: document.getElementById(\'enableProjects\').checked,\n' +
'      enableStats: document.getElementById(\'enableStats\').checked,\n' +
'      yesterdayStats: document.getElementById(\'yesterdayStats\').checked,\n' +
'      enableSchedule: document.getElementById(\'enableSchedule\').checked,\n' +
'      enableUpcoming: document.getElementById(\'enableUpcoming\').checked,\n' +
'      enableNotesPage: document.getElementById(\'enableNotesPage\').checked,\n' +
'      enableSearch: document.getElementById(\'enableSearch\').checked,\n' +
'      enableReflect: document.getElementById(\'enableReflect\').checked,\n' +
'      enableTags: document.getElementById(\'enableTags\').checked,\n' +
'      touchNav: document.getElementById(\'touchNav\').checked,\n' +
'      overtimeNotify: document.getElementById(\'overtimeNotify\').checked,\n' +
'      overtimeRepeat: document.getElementById(\'overtimeRepeat\').checked,\n' +
'      audibleNotifications: document.getElementById(\'audibleNotifications\').checked,\n' +
'      audibleVolume: parseInt(document.getElementById(\'audibleVolume\').value, 10),\n' +
'      breakReminderMin: parseInt(document.getElementById(\'breakReminderMin\').value, 10) || 0,\n' +
'      idleReminderMin: parseInt(document.getElementById(\'idleReminderMin\').value, 10) || 0,\n' +
'      dueReminderMin: parseInt(document.getElementById(\'dueReminderMin\').value, 10) || 0,\n' +
'      liveTracking: document.getElementById(\'liveTracking\').checked,\n' +
'      stopAtMidnight: document.getElementById(\'stopAtMidnight\').checked,\n' +
'      enableTimeline: document.getElementById(\'enableTimeline\').checked,\n' +
'      focusLenMin: parseInt(document.getElementById(\'focusLenMin\').value, 10) || 25,\n' +
'      focusType: document.getElementById(\'focusType\').value,\n' +
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
'  document.getElementById(\'wipeWatchCacheBtn\').addEventListener(\'click\', function () {\n' +
'    if (!window.confirm(\'Wipe the watch\\\'s stored task, habit and project lists? The watch re-downloads them from the phone right away. This does not touch your account or the phone\\\'s data.\')) {\n' +
'      return;\n' +
'    }\n' +
'    returnToWatchApp({ wipeWatchCache: true });\n' +
'  });\n' +
'\n' +
'  var copyStatsBtn = document.getElementById(\'copyStatsBtn\');\n' +
'  if (copyStatsBtn) {\n' +
'    copyStatsBtn.addEventListener(\'click\', function () {\n' +
'      var ta = document.getElementById(\'statsExport\');\n' +
'      var done = function () { setStatus(\'Stats copied.\', false); };\n' +
'      var fail = function () {\n' +
'        ta.focus(); ta.select();\n' +
'        setStatus(\'Press and hold the box, then Copy.\', true);\n' +
'      };\n' +
'      try {\n' +
'        if (navigator.clipboard && navigator.clipboard.writeText) {\n' +
'          navigator.clipboard.writeText(ta.value).then(done, function () {\n' +
'            try { ta.focus(); ta.select(); document.execCommand(\'copy\') ? done() : fail(); }\n' +
'            catch (e2) { fail(); }\n' +
'          });\n' +
'        } else {\n' +
'          ta.focus(); ta.select();\n' +
'          document.execCommand(\'copy\') ? done() : fail();\n' +
'        }\n' +
'      } catch (e) { fail(); }\n' +
'    });\n' +
'  }\n' +
'\n' +
'  document.getElementById(\'cancelBtn\').addEventListener(\'click\', function () {\n' +
'    returnToWatchApp({ cancelled: true });\n' +
'  });\n' +
'})();\n' +
'</script>\n' +
'  <p class="hint" style="text-align:center;margin-top:24px">Super Productivity for Pebble' +
  (appVersion ? ' &middot; v' + escapeHtmlAttr(appVersion) : '') + '</p>\n' +
'</body>\n' +
'</html>\n';

  return 'data:text/html;charset=utf-8,' + encodeURIComponent(html);
}

module.exports = {
  buildPairingPageUrl: buildPairingPageUrl
};
