

var common = require('../common');
var assert = require('assert');

var complete = 0;

process.nextTick(function() {
  complete++;
  process.nextTick(function() {
    complete++;
    process.nextTick(function() {
      complete++;
    });
  });
});

setTimeout(function() {
  process.nextTick(function() {
    complete++;
  });
}, 50);

process.nextTick(function() {
  complete++;
});

process.on('exit', function() {
  assert.equal(5, complete);
  process.nextTick(function() {
    throw new Error('this should not occur');
  });
});
