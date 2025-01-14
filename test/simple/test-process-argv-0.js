
var util = require('util');
var path = require('path');
var assert = require('assert');
var spawn = require('child_process').spawn;
var common = require('../common');

console.error('argv=%j', process.argv);
console.error('exec=%j', process.execPath);

if (process.argv[2] !== "child") {
  var child = spawn('./node', [__filename, "child"], {
    cwd: path.dirname(process.execPath)
  });

  var childArgv0 = '';
  var childErr = '';
  child.stdout.on('data', function(chunk) {
    childArgv0 += chunk;
  });
  child.stderr.on('data', function(chunk) {
    childErr += chunk;
  });
  child.on('exit', function () {
    console.error('CHILD: %s', childErr.trim().split('\n').join('\nCHILD: '));
    if (process.platform === 'win32') {
      // On Windows argv[0] is not expanded into full path
      assert.equal(childArgv0, './node');
    } else {
      assert.equal(childArgv0, process.execPath);
    }
  });
}
else {
  process.stdout.write(process.argv[0]);
}
