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
pro::promise<std::string> p1([]()->std::string {
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
or just
```cpp
pro::make_promise<std::string>([]()->std::string {
  return "Success!";
}).then([](std::string value) { 
	std::cout << value << std::endl; // Success!
});
```

## Chaining promises
You can chain consecutive promises. **.then()** and **.fail()** methods are returning a new promise object.
Promise result type is evaluated by the return type of the passed callback.
Exceptions are propagating down the stream unless handled by a proper callback method.
```cpp
pro::promise<int> p([]()->int { return 1; });

p.then([](int value)->float { 
	return value * 3.14159;
}).then([](float value) { 
	std::cout << value << std::endl; 
}); 
```

### .then overloads
| Method overload | Description | Promise Type |
|-----------------|-------------|--------------|
|promise<Result> then(Cb&& callback, RCb&& rejectCallback, ExCb&& exceptionCallback)|If the promise resolves, _callback_ is called with the promise outcome as a parameter. If it rejects with an exception of type **T**, _rejectCallback_ is invoked with exception's value as a parameter. Otherwise, an _exceptionCallback_ is called with **std_exception_ptr** as a parameter. All callback methods must declare a same return type (evaluated to **Result**). Both _callback_ and _rejectCallback_ must accept a parameter of type **T**. _exceptionCallback_ must accept a **std::exception_ptr** parameter. | T |
|promise<Result> then(Cb&& callback, RCb&& rejectCallback)|If the promise resolves, _callback_ is called with the promise outcome as a parameter. If it rejects with an exception of type **T**, _rejectCallback_ is invoked with exception's value as a parameter. Otherwise, an exception is propagated. All callback methods must declare a same return type (evaluated to **Result**). Both _callback_ and _rejectCallback_ must accept a parameter of type **T**. | T
|promise<Result> then(Cb&& callback)|If the promise resolves, _callback_ is called with the promise outcome as a parameter. If it rejects, an exception is propagated. Callback return type is evaluated to **Result**. | T

### Resolving and rejecting
To resolve a promise, just return a value from your promise method. 
To reject it, just throw something (type or exception). 
Result (returned or thrown) will be passed to corresponding callback methods.

If you are throwing an exception, it can be handled only by exceptionCallback.

Check main.cpp file, for more examples.


## How to install
Just download and include "promise.h" or "ready_promise.h" in your project (use *pro* namespace)

This is a proof of concept for now, so it does have many caveats.


### main.cpp example output

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