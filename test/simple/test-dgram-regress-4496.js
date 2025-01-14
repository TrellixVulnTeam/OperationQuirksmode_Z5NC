

// Remove this test once we support sending strings.

var common = require('../common');
var assert = require('assert');
var dgram = require('dgram');

// Should throw but not crash.
var socket = dgram.createSocket('udp4');
assert.throws(function() { socket.send('BAM', 0, 1, 1, 'host') }, TypeError);
assert.throws(function() { socket.sendto('BAM', 0, 1, 1, 'host') }, TypeError);
socket.close();
