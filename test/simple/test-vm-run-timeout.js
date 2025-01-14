

var common = require('../common');
var assert = require('assert');
var vm = require('vm');

// Test 1: Timeout of 100ms executing endless loop
assert.throws(function() {
  vm.runInThisContext('while(true) {}', '', 100);
});

// Test 2: Timeout must be >= 0ms
assert.throws(function() {
  vm.runInThisContext('', '', -1);
});

// Test 3: Timeout of 0ms
vm.runInThisContext('', '', 0);

// Test 4: Timeout of 1000ms, script finishes first
vm.runInThisContext('', '', 1000);

// Test 5: Nested vm timeouts, inner timeout propagates out
try {
  var context = {
    log: console.log,
    runInVM: function(timeout) {
      vm.runInNewContext('while(true) {}', context, '', timeout);
    }
  };
  vm.runInNewContext('runInVM(10)', context, '', 100);
  throw new Error('Test 5 failed');
} catch (e) {
  if (-1 === e.message.search(/Script execution timed out./)) {
    throw e;
  }
}
