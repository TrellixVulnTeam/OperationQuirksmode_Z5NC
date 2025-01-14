

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
  ++serverConnected;
  server.close();
});
server.listen(common.PIPE, function() {
  var options = { rejectUnauthorized: false };
  var client = tls.connect(common.PIPE, options, function() {
    ++clientConnected;
    client.end();
  });
});

process.on('exit', function() {
  assert.equal(clientConnected, 1);
  assert.equal(serverConnected, 1);
});
