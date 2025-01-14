

var common = require('../common');
var assert = require('assert');
var spawn = require('child_process').spawn;

var args = [ common.fixturesDir + '/clustered-server/app.js' ];
var child = spawn(process.execPath, args);
var outputLines = [];
var outputTimerId;
var waitingForDebuggers = false;

child.stderr.on('data', function(data) {
  var lines = data.toString().replace(/\r/g, '').trim().split('\n');
  var line = lines[0];

  lines.forEach(function(ln) { console.log('> ' + ln) } );

  if (outputTimerId !== undefined)
    clearTimeout(outputTimerId);

  if (waitingForDebuggers) {
    outputLines = outputLines.concat(lines);
    outputTimerId = setTimeout(onNoMoreLines, 200);
  } else if (line === 'all workers are running') {
    waitingForDebuggers = true;
    process._debugProcess(child.pid);
  }
});

function onNoMoreLines() {
  assertOutputLines();
  process.exit();
}

setTimeout(function testTimedOut() {
  assert(false, 'test timed out.');
}, 3000);

process.on('exit', function onExit() {
    child.kill();
});

function assertOutputLines() {
  var startLog = process.platform === 'win32'
                 ? 'Starting debugger agent.'
                 : 'Hit SIGUSR1 - starting debugger agent.';

  var expectedLines = [
    startLog,
    'debugger listening on port ' + 5858,
    startLog,
    'debugger listening on port ' + 5859,
    startLog,
    'debugger listening on port ' + 5860,
  ];

  // Do not assume any particular order of output messages,
  // since workers can take different amout of time to
  // start up
  outputLines.sort();
  expectedLines.sort();

  assert.equal(outputLines.length, expectedLines.length);
  for (var i = 0; i < expectedLines.length; i++)
    assert.equal(outputLines[i], expectedLines[i]);
}
