

var common = require('../common');
var assert = require('assert');
var Script = require('vm').Script;

common.globalCheck = false;

common.debug('run a string');
var script = new Script('\'passed\';');
common.debug('script created');
var result1 = script.runInNewContext();
var result2 = script.runInNewContext();
assert.equal('passed', result1);
assert.equal('passed', result2);

common.debug('thrown error');
script = new Script('throw new Error(\'test\');');
assert.throws(function() {
  script.runInNewContext();
});



common.debug('undefined reference');
var error;
script = new Script('foo.bar = 5;');
try {
  script.runInNewContext();
} catch (e) {
  error = e;
}
assert.ok(error);
assert.ok(error.message.indexOf('not defined') >= 0);

common.debug('error.message: ' + error.message);


hello = 5;
script = new Script('hello = 2');
script.runInNewContext();
assert.equal(5, hello);


common.debug('pass values in and out');
code = 'foo = 1;' +
       'bar = 2;' +
       'if (baz !== 3) throw new Error(\'test fail\');';
foo = 2;
obj = { foo: 0, baz: 3 };
script = new Script(code);
var baz = script.runInNewContext(obj);
assert.equal(1, obj.foo);
assert.equal(2, obj.bar);
assert.equal(2, foo);

common.debug('call a function by reference');
script = new Script('f()');
function changeFoo() { foo = 100 }
script.runInNewContext({ f: changeFoo });
assert.equal(foo, 100);

common.debug('modify an object by reference');
script = new Script('f.a = 2');
var f = { a: 1 };
script.runInNewContext({ f: f });
assert.equal(f.a, 2);

common.debug('invalid this');
assert.throws(function() {
  script.runInNewContext.call('\'hello\';');
});


