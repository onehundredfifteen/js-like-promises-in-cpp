# js-like-promises-in-cpp
 Call your C++ methods asynchronously as easy as you do it in JavaScript ES6.

## How to use
Check main.cpp file, it contains examples

## How to install
Just include "async.h" and use *crows* namespace

## TODO

This is a proof of concept for now, so it does have many caveats.

* resolve reject callback type assertion
* .catch
* conversion to std::future

### main.cpp example output FYI

```
main thread ID: 20268
method foo waited in thread = 16404 for 777 ms.
promise p3 was resolved
promise p3 was resolvedTotal Time Taken =
790 ms
method bar executed in thread = 6512
method in p2 ready
promise 2 was rejected, then result = 666
method in p1 ready
promise 1 resolved, then result = 115
```