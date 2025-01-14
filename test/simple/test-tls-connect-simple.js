

var common = require('../common');
var assert = require('assert');
var tls = require('tls');
var fs = require('fs');

var clientConnected = 0;
var serverConnected = 0;

var options = {
  key: fs.readFileSync(common.fixturesDir + '/keys/agent1-key.pem'),
  cert: fs.readFileSync(common.fixturesDir + '/keys/agent1-cert.pem')
};

var server = tls.Server(options, function(socket) {
  if (++serverConnected === 2) {
    server.close();
  }
});

server.listen(common.PORT, function() {
  var client1 = tls.connect({
    port: common.PORT,
    rejectUnauthorized: false
  }, function() {
    ++clientConnected;
    client1.end();
  });

  var client2 = tls.connect({
    port: common.PORT,
    rejectUnauthorized: false
  });
  client2.on('secureConnect', function() {
    ++clientConnected;
    client2.end();
  });
});

process.on('exit', function() {
  assert.equal(clientConnected, 2);
  assert.equal(serverConnected, 2);
});
