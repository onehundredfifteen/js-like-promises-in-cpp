# js-like-promises-in-cpp
 Call your C++ methods asynchronously as easy as you do it in JavaScript ES6.

## How to use
Check main.cpp file, it contains examples

## How to install
Just include "async.h" and use *crows* namespace

## TODO

This is a proof of concept for now, so it does have many caveats.

* fix bugs with .then deducing types
* resolve reject callback type assertion
* .catch
* conversion to std::future

### main.cpp example output FYI

main thread ID: 14504
Total Time Taken = 2 ms
method foo waited in thread = 8596 for 777 ms.
method in p2 ready
promise 2 was rejected, then result = 666
method in p1 ready
promise 1 resolved, then result = 115