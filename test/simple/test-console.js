




var common = require('../common');
var assert = require('assert');

assert.ok(process.stdout.writable);
assert.ok(process.stderr.writable);
// Support legacy API
assert.equal('number', typeof process.stdout.fd);
assert.equal('number', typeof process.stderr.fd);

// an Object with a custom .inspect() function
var custom_inspect = { foo: 'bar', inspect: function () { return 'inspect'; } };

var stdout_write = global.process.stdout.write;
var strings = [];
global.process.stdout.write = function(string) {
  strings.push(string);
};
console._stderr = process.stdout;

// test console.log()
console.log('foo');
console.log('foo', 'bar');
console.log('%s %s', 'foo', 'bar', 'hop');
console.log({slashes: '\\\\'});
console.log(custom_inspect);

// test console.dir()
console.dir(custom_inspect);

// test console.trace()
console.trace('This is a %j %d', { formatted: 'trace' }, 10, 'foo');


global.process.stdout.write = stdout_write;

assert.equal('foo\n', strings.shift());
assert.equal('foo bar\n', strings.shift());
assert.equal('foo bar hop\n', strings.shift());
assert.equal("{ slashes: '\\\\\\\\' }\n", strings.shift());
assert.equal('inspect\n', strings.shift());
assert.equal("{ foo: 'bar', inspect: [Function] }\n", strings.shift());
assert.equal('Trace: This is a {"formatted":"trace"} 10 foo',
             strings.shift().split('\n').shift());

assert.throws(function () {
  console.timeEnd('no such label');
});

assert.doesNotThrow(function () {
  console.time('label');
  console.timeEnd('label');
});
