

// test convenience methods with and without options supplied

var common = require('../common.js');
var assert = require('assert');
var zlib = require('zlib');

var hadRun = 0;

var expect = 'blahblahblahblahblahblah';
var opts = {
  level: 9,
  chunkSize: 1024,
};

[
  ['gzip', 'gunzip'],
  ['gzip', 'unzip'],
  ['deflate', 'inflate'],
  ['deflateRaw', 'inflateRaw'],
].forEach(function(method) {

  zlib[method[0]](expect, opts, function(err, result) {
    zlib[method[1]](result, opts, function(err, result) {
      assert.equal(result, expect,
        'Should get original string after ' +
        method[0] + '/' + method[1] + ' with options.');
      hadRun++;
    });
  });

  zlib[method[0]](expect, function(err, result) {
    zlib[method[1]](result, function(err, result) {
      assert.equal(result, expect,
        'Should get original string after ' +
        method[0] + '/' + method[1] + ' without options.');
      hadRun++;
    });
  });

});

process.on('exit', function() {
  assert.equal(hadRun, 8, 'expect 8 compressions');
});
