# js-like-promises-in-cpp
 Call your C++ methods asynchronously as easy as you do it in JavaScript ES6.

## How to use

If you are familiar with javascript Promises such as:
```javascript
var p1 = new Promise((resolve, reject) => {
  resolve('Success!');
  // or
  // reject(new Error("Error!"));
});

p1.then(value => {
  console.log(value); // Success!
}, reason => {
  console.error(reason); // Error!
});
```
[example source](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Promise/then)

You will like the C++ way to do it:
```cpp
pro::Promise<std::string> p1([] {
  return "Success!";
  // or
  // throw "Error!";
  // or
  // throw std::exception("Another Error");
});

p1.then([](std::string value) { 
	std::cout << value << std::endl; // Success!
},
[](std::string reason) { 
	std::cout << reason << std::endl; // Error!
});
```

### Resolving and rejecting
To resolve a promise, just return a value. To reject, just throw something (type or exception). If you are throwing an exception, it can be handled by exceptionCallback.

Check main.cpp file, it contains examples.

## How to install
Just download and include "promise.h" or "ready_promise.h" in your project (use *pro* namespace)

This is a proof of concept for now, so it does have many caveats.


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