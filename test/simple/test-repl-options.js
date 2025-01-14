

var common = require('../common'),
    assert = require('assert'),
    Stream = require('stream'),
    repl = require('repl');

common.globalCheck = false;

// create a dummy stream that does nothing
var stream = new Stream();
stream.write = stream.pause = stream.resume = function(){};
stream.readable = stream.writable = true;

// 1, mostly defaults
var r1 = repl.start({
  input: stream,
  output: stream,
  terminal: true
});
assert.equal(r1.rli.input, stream);
assert.equal(r1.rli.output, stream);
assert.equal(r1.rli.input, r1.inputStream);
assert.equal(r1.rli.output, r1.outputStream);
assert.equal(r1.rli.terminal, true);
assert.equal(r1.useColors, r1.rli.terminal);
assert.equal(r1.useGlobal, false);
assert.equal(r1.ignoreUndefined, false);

// 2
function writer() {}
function evaler() {}
var r2 = repl.start({
  input: stream,
  output: stream,
  terminal: false,
  useColors: true,
  useGlobal: true,
  ignoreUndefined: true,
  eval: evaler,
  writer: writer
});
assert.equal(r2.rli.input, stream);
assert.equal(r2.rli.output, stream);
assert.equal(r2.rli.input, r2.inputStream);
assert.equal(r2.rli.output, r2.outputStream);
assert.equal(r2.rli.terminal, false);
assert.equal(r2.useColors, true);
assert.equal(r2.useGlobal, true);
assert.equal(r2.ignoreUndefined, true);
assert.equal(r2.writer, writer);
