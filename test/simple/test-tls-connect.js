

if (!process.versions.openssl) {
  console.error('Skipping because node compiled without OpenSSL.');
  process.exit(0);
}

var common = require('../common');
var assert = require('assert');
var fs = require('fs');
var tls = require('tls');
var path = require('path');

// https://github.com/joyent/node/issues/1218
// uncatchable exception on TLS connection error
(function() {
  var cert = fs.readFileSync(path.join(common.fixturesDir, 'test_cert.pem'));
  var key = fs.readFileSync(path.join(common.fixturesDir, 'test_key.pem'));

  var errorEmitted = false;

  process.on('exit', function() {
    assert.ok(errorEmitted);
  });

  var conn = tls.connect({cert: cert, key: key, port: common.PORT}, function() {
    assert.ok(false); // callback should never be executed
  });

  conn.on('error', function() {
    errorEmitted = true;
  });
})();
