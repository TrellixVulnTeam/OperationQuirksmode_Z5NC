

var common = require('../common');
var assert = require('assert');

// Fix the memory explosion that happens when writing to a http request
// where the server has destroyed the socket on us between a successful
// first request, and a subsequent request that reuses the socket.
//
// This test should not be ported to v0.10 and higher, because the
// problem is fixed by not ignoring ECONNRESET in the first place.

var http = require('http');
var net = require('net');
var server = http.createServer(function(req, res) {
  // simulate a server that is in the process of crashing or something
  // it only crashes after the first request, but before the second,
  // which reuses the connection.
  res.end('hallo wereld\n', function() {
    setTimeout(function() {
      req.connection.destroy();
    }, 100);
  });
});

var gotFirstResponse = false;
var gotFirstData = false;
var gotFirstEnd = false;
server.listen(common.PORT, function() {

  var gotFirstResponse = false;
  var first = http.request({
    port: common.PORT,
    path: '/'
  });
  first.on('response', function(res) {
    gotFirstResponse = true;
    res.on('data', function(chunk) {
      gotFirstData = true;
    });
    res.on('end', function() {
      gotFirstEnd = true;
    })
  });
  first.end();
  second();

  function second() {
    var sec = http.request({
      port: common.PORT,
      path: '/',
      method: 'POST'
    });

    var timer = setTimeout(write, 200);
    var writes = 0;
    var sawFalseWrite;

    function write() {
      if (++writes === 64) {
        clearTimeout(timer);
        sec.end();
        test();
      } else {
        timer = setTimeout(write);
        var writeRet = sec.write(new Buffer('hello'));

        // Once we find out that the connection is destroyed, every
        // write() returns false
        if (sawFalseWrite)
          assert.equal(writeRet, false);
        else
          sawFalseWrite = writeRet === false;
      }
    }

    assert.equal(first.connection, sec.connection,
                 'should reuse connection');

    sec.on('response', function(res) {
      res.on('data', function(chunk) {
        console.error('second saw data: ' + chunk);
      });
      res.on('end', function() {
        console.error('second saw end');
      });
    });

    function test() {
      server.close();
      assert(sec.connection.destroyed);
      if (sec.output.length || sec.outputEncodings.length)
        console.error('bad happened', sec.output, sec.outputEncodings);
      assert.equal(sec.output.length, 0);
      assert.equal(sec.outputEncodings, 0);
      assert(sawFalseWrite);
      assert(gotFirstResponse);
      assert(gotFirstData);
      assert(gotFirstEnd);
      console.log('ok');
    }
  }
});
