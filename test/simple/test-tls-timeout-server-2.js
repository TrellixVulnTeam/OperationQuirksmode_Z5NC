

if (!process.versions.openssl) process.exit();

var common = require('../common');
var assert = require('assert');
var tls = require('tls');
var fs = require('fs');

var options = {
  key: fs.readFileSync(common.fixturesDir + '/keys/agent1-key.pem'),
  cert: fs.readFileSync(common.fixturesDir + '/keys/agent1-cert.pem')
};

var server = tls.createServer(options, function(cleartext) {
  cleartext.setTimeout(50, function() {
    cleartext.destroy();
    server.close();
  });
});

server.listen(common.PORT, function() {
  tls.connect({
    host: '127.0.0.1',
    port: common.PORT,
    rejectUnauthorized: false
  });
});
