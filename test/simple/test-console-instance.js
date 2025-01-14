


var common = require('../common');
var assert = require('assert');
var Stream = require('stream');
var Console = require('console').Console;
var called = false;

// ensure the Console instance doesn't write to the
// process' "stdout" or "stderr" streams
process.stdout.write = process.stderr.write = function() {
  throw new Error('write() should not be called!');
};

// make sure that the "Console" function exists
assert.equal('function', typeof Console);

// make sure that the Console constructor throws
// when not given a writable stream instance
assert.throws(function () {
  new Console();
}, /Console expects a writable stream/);

var out = new Stream();
var err = new Stream();
out.writable = err.writable = true;
out.write = err.write = function(d) {};

var c = new Console(out, err);

out.write = err.write = function(d) {
  assert.equal(d, 'test\n');
  called = true;
};

assert(!called);
c.log('test');
assert(called);

called = false;
c.error('test');
assert(called);

out.write = function(d) {
  assert.equal('{ foo: 1 }\n', d);
  called = true;
};

called = false;
c.dir({ foo: 1 });
assert(called);

// ensure that the console functions are bound to the console instance
called = 0;
out.write = function(d) {
  called++;
  assert.equal(d, called + ' ' + (called - 1) + ' [ 1, 2, 3 ]\n');
};
[1, 2, 3].forEach(c.log);
assert.equal(3, called);
