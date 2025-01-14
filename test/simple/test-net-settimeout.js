

// This example sets a timeout then immediately attempts to disable the timeout
// https://github.com/joyent/node/pull/2245

var common = require('../common');
var net = require('net');
var assert = require('assert');

var T = 100;

var server = net.createServer(function(c) {
  c.write('hello');
});
server.listen(common.PORT);

var killers = [0, Infinity, NaN];

var left = killers.length;
killers.forEach(function(killer) {
  var socket = net.createConnection(common.PORT, 'localhost');

  socket.setTimeout(T, function() {
    socket.destroy();
    if (--left === 0) server.close();
    assert.ok(killer !== 0);
    clearTimeout(timeout);
  });

  socket.setTimeout(killer);

  var timeout = setTimeout(function() {
    socket.destroy();
    if (--left === 0) server.close();
    assert.ok(killer === 0);
  }, T * 2);
});
