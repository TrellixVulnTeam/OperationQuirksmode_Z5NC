

var common = require('../common');
var assert = require('assert');

var http = require('http');
var net = require('net');

var gotResponse = false;

var server = net.createServer(function(conn) {
  var body = 'Yet another node.js server.';

  var response =
      'HTTP/1.1 200 OK\r\n' +
      'Connection: close\r\n' +
      'Content-Length: ' + body.length + '\r\n' +
      'Content-Type: text/plain;\r\n' +
      ' x-unix-mode=0600;\r\n' +
      ' name=\"hello.txt\"\r\n' +
      '\r\n' +
      body;

  conn.end(response);
  server.close();
});

server.listen(common.PORT, function() {
  http.get({host: '127.0.0.1', port: common.PORT}, function(res) {
    assert.equal(res.headers['content-type'],
                 'text/plain;x-unix-mode=0600;name="hello.txt"');
    gotResponse = true;
    res.destroy();
  });
});

process.on('exit', function() {
  assert.ok(gotResponse);
});
