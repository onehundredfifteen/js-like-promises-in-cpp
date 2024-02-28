# C++ Promises
Call your C++ methods asynchronously as easy as you do it in Java or JavaScript ES6.

This library defines a **pro::promise** object implementation that
+ is basically a wrapper over a **std::future** object
+ provides a __continuation__ and __chaining__ functionalities
+ introduces out-of-the-box aynchronous features in your application
+ allows to handle exceptions easily

## Quick overview
If you are familiar with JAVA [CompletableFuture](https://www.baeldung.com/java-completablefuture) or javascript [Promises](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Promise/then) such as: 
```javascript
var p1 = new Promise((resolve, reject) => {
  resolve('Success!'); // or reject(new Error("Error!"));
});

p1.then(value => {
  console.log(value); // Success!
}, reason => {
  console.error(reason); // Error!
});
```
You would love the C++ way to write it down:
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
You may find waiting methods handy:
```cpp
pro::promise<int> p1([] { return 13; });
pro::promise<std::string> p2([] { return std::string("promised string"); });

pro::PromiseAll(p1, p2).then(
     [](auto tuple) { std::cout << std::get<0>(tuple) << std::get<1>(tuple) << std::endl; }
);
```

## Promise object
A **pro::promise** object is a wrapper over a **std::future**.
You can create one using any __invocable__ entity which you want to run such as a method, a lambda expression or a **std::function** wrapper. \
Provided method will launch immediately and you can define how to receive the result - in [sync or async](#blocking) way.
If your method accepts some parameters, pass them in during **promise** construction.

These promises are equivalent:
```cpp
int myAddMethod(int a, int b){
  return a + b;
}

pro::promise<int> p(myAddMethod, 1, 8);
pro::promise<int> p1([](int a, int b){return a + b;}, 1, 8);
pro::promise<int> p2 = pro::make_promise<int>(myAddMethod, 1, 8);
```

### Getting a result
The only way to receive a result of the passed method is to call one of continuable methods - [.then()](#then) or [.fail()](#fail).
That's why it is called a promise instead of future.

A given promise can yield the result only once. Through the __.valid()__ method you can check whether a promise is sane.
```cpp
pro::promise<int> p(myAddMethod, 1, 8);
bool check = p.valid(); //true
p.then(...);
check = p.valid(); //false
```

### I want to resolve/reject promises using an object!
I'm aware that my implementation differs from Javascript' _Promise(resolve, reject) => {}_ approach and it can't be enough sometimes.
That's why a special constructor was introduced to cover such cases. \
Passing a method accepting a **std::promise&lt;T&gt;** as a parameter allows you to resolve or reject the promise from anywhere inside the method's body:

```cpp
pro::promise<int>::resolver_fn_type fun = [](std::promise<int> resolver) {
     auto inner = [&resolver](int a) {
          resolver.set_value(a); //to resolve
          //resolver.set_exception(std::make_exception_ptr(a)); //to reject
     };
     inner(115);
};
pro::promise<int> p(fun); //will resolve with 115
```
**pro::promise&lt;int&gt;::resolver_fn_type** stands for **std::function&lt;void(std::promise&lt;int&gt;)&gt;** \
Remember to set promise value only once. Further attempts will yield _'promise already satisfied'_ error!

### Constructor overloads and operators
You can create a promise from given **std::future** therefore from a **std::promise**:
```cpp
int myMethod(){
  return 5;
}
std::promise<int> std_promise;
pro::promise<int> p(std_promise.get_future());
std::thread t(myMethod, std::move(std_promise));
```
And get **std::future** future back:
```cpp
pro::promise<int> p(myMethod);
std::future<int> fut = p;
```
Or define an already fulfilled / rejected promise:
```cpp
pro::promise<int> p_fulfilled(115);
pro::promise<int> p_rejected(std::make_exception_ptr(115));
```

## <a name="then"></a>Chaining promises
You can chain consecutive promises. **.then()** and **.fail()** methods are returning a new promise object.
Promise result type is evaluated by the return type of the passed callback. \
Exceptions are propagating down the stream unless handled by a proper callback method. \
Promise would _move_ your object through instead of copying it, so don't bother using references as your function parameters.

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
|promise&lt;Result&gt; then(Cb&& callback, RCb&& rejectCallback, ExCb&& exceptionCallback)|If the promise resolves, _callback_ is called with the promise outcome as a parameter. If it rejects with an exception of type **T**, _rejectCallback_ is invoked with exception's value as a parameter. Otherwise an _exceptionCallback_ is called with **std::exception_ptr** as a parameter. All callback methods must declare a same return type (evaluated to **Result**). Both _callback_ and _rejectCallback_ must accept a parameter of type **T**. _exceptionCallback_ must accept a **std::exception_ptr** parameter. | promise&lt;T&gt; |
|promise&lt;Result&gt; then(Cb&& callback, RCb&& rejectCallback)|If the promise resolves, _callback_ is called with the promise outcome as a parameter. If it rejects with an exception of type **T**, _rejectCallback_ is invoked with exception's value as a parameter. Otherwise, an exception is propagated. All callback methods must declare a same return type (evaluated to **Result**). Both _callback_ and _rejectCallback_ must accept a parameter of type **T**. | promise&lt;T&gt; |
|promise&lt;Result&gt; then(Cb&& callback)|If the promise resolves, _callback_ is called with the promise outcome as a parameter. If it rejects, an exception is propagated. Callback return type is evaluated to **Result**. | promise&lt;T&gt; |
|promise&lt;Result&gt; then(Cb&& callback, RCb&& rejectCallback, ExCb&& exceptionCallback)|If the promise resolves, _callback_ is called with no parameter. If it rejects with an exception of type other than **std::exception**, _rejectCallback_ is invoked. Otherwise, an _exceptionCallback_ is called with **std::exception_ptr** as a parameter. All callback methods must declare a same return type (evaluated to **Result**. Both _callback_ and _rejectCallback_ must accept no parameter. _exceptionCallback_ must accept a **std::exception_ptr** parameter. | promise&lt;void&gt; |
|promise&lt;Result&gt; then(Cb&& callback, RCb&& rejectCallback)|If the promise resolves, _callback_ is called. If it rejects, _rejectCallback_ is invoked. Otherwise, an exception is propagated. All callback methods must declare a same return type (evaluated to **Result**). Both _callback_ and _rejectCallback_ must accept no parameter. | promise&lt;void&gt; |
|promise&lt;Result&gt; then(Cb&& callback)|If the promise resolves, _callback_ is called. If it rejects, an exception is propagated. Callback return type is evaluated to **Result**. | promise&lt;void&gt; |

## <a name="fail"></a>Exception handling
By design, a promise can reject in two ways - by throwning a value of type **T** or an exception.
Using **std::exception_ptr** is not handy, because you'll need to rethrow it.
You can handle these cases separately in **.then** callbacks or by the **.fail** method which as well handles all previously propagated exceptions. Such an exception can be thrown by a callback.

```cpp
int getHttpRequestCode(std::string url){
  ///some code
}

pro::promise<int> p(getHttpRequestCode, "https://github.com/");

p.then([](int code) { 
	if(code == 404)
    throw code;
  return code;
}).then([](int c) { 
	std::cout << "My promise resolved with the code:" << c << std::endl; 
  throw std::invalid_argument("test");
},[](int c) { 
	std::cout << "My promise rejected with the code:"  << c << std::endl; 
}).fail([](std::exception_ptr eptr){
  try {
    std::rethrow_exception(eptr);
  }
  catch (std::invalid_argument &ex) {
      std::cout << "Some promise rejected with an invalid_argument exception: " << ex.what() << std::endl;
  }
  catch (...) {
     std::cout << "Some promise rejected with an unknown exception" << std::endl;
  }
}); 
```
Code example above will output:
```
My promise resolved with the code 200
Some promise rejected with an invalid_argument exception: test
```
or if _getHttpRequestCode_ returns 404:
```
My promise rejected with the code 404
```

### .fail overloads
| Method overload | Description | Promise Type |
|-----------------|-------------|--------------|
|promise&lt;Result&gt; fail(RCb&& rejectCallback, ExCb&& exceptionCallback)|If the promise rejects with an exception or there is an exception in the chain of type **T**, _rejectCallback_ is invoked with exception's value as a parameter. Otherwise an _exceptionCallback_ is called with **std::exception_ptr** as a parameter. All callback methods must declare a same return type (evaluated to **Result**). _rejectCallback_ must accept a parameter of type **T**. _exceptionCallback_ must accept a **std::exception_ptr** parameter. | promise&lt;T&gt; |
|promise&lt;Result&gt; fail(ExCb&& exceptionCallback)|If the promise rejects with an exception or there is an exception in the chain, _exceptionCallback_ is invoked with exception's pointer value as a parameter. Callback return type is evaluated to **Result** | promise&lt;T&gt; |
|promise&lt;Result&gt; fail(ExCb&& exceptionCallback)|If the promise rejects with an exception or there is an exception in the chain, _exceptionCallback_ is invoked with exception's pointer value as a parameter. Callback return type is evaluated to **Result** |  promise&lt;void&gt; |

## Resolving, rejecting and cancelling a promise
A promise fulfills when its method returns. It rejects when its method throws something.
There is another way to achieve this by invoking _.resolve_ and _.reject_ method on a promise object. This way you can as well cancel a long running promise method, but you'll have to provide **executor&lt;T&gt;** object instance.

Check test.cpp file for more examples.

## pro::promise static methods
(include file "util.h")  \
You may have to convert more promises into a single one:

### PromiseAll
The PromiseAll static method takes an iterable of promises&lt;T&gt; as input and returns a single promise&lt;T&gt;. This returned promise fulfills when all of the input's promises fulfill (including when an empty iterable is passed), with an std::vector&lt;T&gt; of the fulfillment values. It rejects when any of the input's promises rejects, with this first rejection reason.

```cpp
std::vector<pro::promise<int>> v;
v.emplace_back(pro::make_promise<int>([] { return 115; });
v.emplace_back(pro::make_promise<int>([] { return 420; });
       
pro::PromiseAll(v).then(
          [](auto vec) { 
              int result = std::reduce(vec.begin(), vec.end()); //result will be 535
          }
);
```

### PromiseAll for different types of promises
The second version of PromiseAll static method takes any number of different promises as inputs and returns a single promise&lt;std::tuple&lt;Args...&gt;&gt;. This returned promise fulfills when all of the input's promises fulfill (including when an empty iterable is passed), with an &lt;std::tuple&lt;Args...&gt;&gt; of the fulfillment values. It rejects when any of the input's promises rejects, with this first rejection reason.

```cpp
pro::promise<int> p1([] { return 6; });
pro::promise<long> p2([] { return 9L; });

pro::PromiseAll(p1, p2).then(
    (auto tuple) { int result = std::get<0>(tuple) + (int)std::get<1>(tuple); } //result will be 69
);
```

### PromiseAny
The PromiseAny() static method takes an iterable of promises&lt;T&gt; as input and returns a single promise&lt;T&gt;. This returned promise fulfills when any of the input's promises fulfills, with this first fulfillment value. It rejects when all of the input's promises reject (including when an empty iterable is passed), with an **AggregateException** containing an array of rejection reasons.

### PromiseRace
The PromiseRace() static method takes an iterable of promises&lt;T&gt; as input and returns a single promise&lt;T&gt;. This returned promise settles with the eventual state of the first promise that settles.

## <a name="blocking"></a>Blocking problem
To run your method asynchronously you have to store the last promise object from the chain in the same scope, because it'll block until can be destroyed.
However you can delegate it using _.async_ method. \
PromiseAll methods even in blocking mode can process passed iterables asynchronously.

```cpp
void sleepAndReturn(int sleep) {
    std::this_thread::sleep_for(std::chrono::milliseconds(sleep));
}

pro::promise<void> p1(sleepAndReturn, 115);
auto pp = p1.then(...); //this is async

pro::promise<void> p2(sleepAndReturn, 115);
p2.then(...).async(); //this is async

pro::promise<void> p3(sleepAndReturn, 115);
p3.then(...); //this is blocking 
```

## How to install
Just download and include the file **"promise.h"** in your project (use *pro* namespace). \
To use static methods like _PromiseAll_, include "util.h". \ 
If you want to try an experimental promise with a state and subscribers, include "ready_promise.h".

```cpp
#include "promise.h" //promise objects
#include "util.h" //PromiseAll, PromiseAny, PromiseRace
```

This is a proof of concept for now, so it does have some caveats.

For code samples, check **tests.cpp**. \
There are 169 assertions in 12 test cases.