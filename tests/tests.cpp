#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "./catch/catch_amalgamated.hpp"
#include "../include/promise.h"
#include "../include/ready_promise.h"

//Test wrappers for Promise<T>.then(resolve, reject)
int wrapThenTypedPromise(pro::Promise<int> &p) {
    int res = 0;
    p.then(
        [&res](int i) mutable { res = i; },
        [&res](int i) mutable { res = i * 2;}
    );
    return res;
}
int wrapThenVoidPromise(pro::Promise<void>& p) {
    int res = 0;
    p.then(
        [&res]() mutable { res = 1; },
        [&res]() mutable { res = 2; }
    );
    return res;
}
//Test wrappers for Promise<T>.then(resolve)
int wrapThenTypedPromise_single(pro::Promise<int>& p) {
    int res = 0;
    p.then(
        [&res](int i) mutable { res = i; }
    );
    return res;
}
int wrapThenVoidPromise_single(pro::Promise<void>& p) {
    int res = 0;
    p.then(
        [&res]() mutable { res = 1; }
    );
    return res;
}
//Test wrapper for Promise<T>.then(resolve, reject, reject_with_exception)
int wrapThenTypedPromise_full(pro::Promise<int>& p) {
    int res = 0;
    p.then(
        [&res](int i) mutable { res = i; },
        [&res](int i) mutable { res = i * 2; },
        [&res](std::exception_ptr p) mutable { res = 3; }
    );
    return res;
}
//Test wrapper for Promise<void>.then(resolve, reject_with_exception)
int wrapThenVoidPromise_full(pro::Promise<void>& p) {
    int res = 0;
    p.then(
        [&res]() mutable { res = 1; },
        [&res](std::exception_ptr p) mutable { res = 2; }
    );
    return res;
}
//Test wrapper for Promise<T>.then(resolve).fail(reject_with_exception), error propagation
int propagateError(pro::Promise<void>& p, bool includeFail) {
    int res = 0;

    if (includeFail) {
        p.then(
            [&res]() mutable { res = 1; }
        ).fail(
            [&res](std::exception_ptr ptr) mutable { res = 2; }
        );
    }
    else {
        p.then(
            [&res]() mutable { res = 1; }
        );
    }
    return res;
}

//thread worker
void worker(std::promise<int>&& stdIntPromise, int a, int b) {
    stdIntPromise.set_value(a * b);
}

TEST_CASE("Promise<T>.then", "[basic]") {
    REQUIRE(wrapThenTypedPromise(pro::Promise<int>([] { return 1; })) == 1);
    REQUIRE(wrapThenTypedPromise(pro::Promise<int>([]() -> int { throw 1; })) == 2);
    REQUIRE(wrapThenTypedPromise(pro::Promise<int>([](int arg) { return arg; }, 10)) == 10);
    REQUIRE(wrapThenTypedPromise(pro::Promise<int>([](int arg) -> int { throw arg; }, 10)) == 20);

    REQUIRE(wrapThenTypedPromise_single(pro::Promise<int>([] { return 1; })) == 1);
    REQUIRE(wrapThenTypedPromise_single(pro::Promise<int>([]() -> int { throw 1; })) == 0);
    REQUIRE(wrapThenTypedPromise_full(pro::Promise<int>([] { return 1; })) == 1);
    REQUIRE(wrapThenTypedPromise_full(pro::Promise<int>([]() -> int { throw std::exception("test"); })) == 3);
}
TEST_CASE("Promise<void>.then", "[basic]") {
    REQUIRE(wrapThenVoidPromise(pro::Promise<void>([]{ })) == 1);
    REQUIRE(wrapThenVoidPromise(pro::Promise<void>([]{ throw 1; })) == 2);
    REQUIRE(wrapThenVoidPromise(pro::Promise<void>([](int arg) { }, 10)) == 1);
    REQUIRE(wrapThenVoidPromise(pro::Promise<void>([](int arg) { throw arg; }, 10)) == 2);

    REQUIRE(wrapThenVoidPromise_single(pro::Promise<void>([] { })) == 1);
    REQUIRE(wrapThenVoidPromise_single(pro::Promise<void>([] { throw 1; })) == 0);
    REQUIRE(wrapThenVoidPromise_full(pro::Promise<void>([] { })) == 1);
    REQUIRE(wrapThenVoidPromise_full(pro::Promise<void>([] { throw 1; })) == 2);
    REQUIRE(wrapThenVoidPromise_full(pro::Promise<void>([] { throw std::exception("test"); })) == 2);
}
TEST_CASE("Promise chain & error propagation", "[basic]") {
    REQUIRE(propagateError(pro::Promise<void>([] {}), true) == 1);
    REQUIRE(propagateError(pro::Promise<void>([] { throw 1; }), true) == 2);
    REQUIRE_NOTHROW(propagateError(pro::Promise<void>([] { throw 1; }), false));
}

TEST_CASE("Promise chaining", "[natural]") {
    int res = 0;
    pro::Promise<int> p([] { return 1; });

    REQUIRE(p.valid() == true);

    auto nextP = p.then(
        [&res](int i) mutable {res = i; throw 7; },
        [&res](int i) mutable {res = i * 2; }
    );

    SECTION("first then, rejecting resolved promise") {
        REQUIRE(res == 1);
        
        nextP.then(
            [&res]() mutable {res = 3; },
            [&res]() mutable {res = 4; }
        );
        SECTION("second then, expecting rejectCallback called") { 
            REQUIRE(res == 4);           
        }
    }

    REQUIRE(p.valid() == false);
    REQUIRE(nextP.valid() == false);
}

TEST_CASE("Promise chaining with .fail", "[natural]") {
    int res = 0;
    pro::Promise<int> p([] { return 1; });

    REQUIRE(p.valid() == true);

    auto nextP = p.then(
        [](int i) -> int { throw 7; },
        [&res](int i) mutable { res = 2; return 0; }
    );

    SECTION("first then, rejecting resolved promise") {
        REQUIRE_FALSE(res == 2);

        auto failP = nextP.fail(
            [&res](int i) -> int { throw std::exception("ex"); },
            [&res](std::exception_ptr ptr) mutable { res = 3; return 4; }
        );
        SECTION(".fail, expecting rejectCallback called") {
            REQUIRE_FALSE(res == 3);

            failP.fail(
                [&res](int i) mutable { res = 5; },
                [&res](std::exception_ptr ptr) mutable { res = 6; }
            );
            SECTION("second .fail, expecting exceptionCallback called") {
                REQUIRE(res == 6);
            }
        }
    }

    REQUIRE(p.valid() == false);
    REQUIRE(nextP.valid() == false);
}

TEST_CASE("Promise chaining with error propagation", "[natural]") {
    int res = 0;
    pro::Promise<void> p([] { throw 7; });

    REQUIRE(p.valid() == true);

    auto nextP = p.then(
        [&res]() mutable { res = 8; }
    );

    SECTION("first then, expecting error catch") {
        REQUIRE_FALSE(res == 8);

        nextP.fail(
            [&res](std::exception_ptr ptr) mutable { res = 9; }
        );
        SECTION(".fail, expecting rejectCallback called") {
            REQUIRE(res == 9);
        }
    }

    REQUIRE(p.valid() == false);
    REQUIRE(nextP.valid() == false);
}

TEST_CASE("Promise with std::future source", "[natural]") {
    int res = 0;
    std::promise<int> std_promise;
    //std::future<int> tresult = std_promise.get_future();
    pro::Promise<int> p(std_promise.get_future());
    
    std::thread t(worker, std::move(std_promise), 2, 3);

    REQUIRE(p.valid() == true);
    SECTION("then, resolved promise") {
        p.then(
            [&res](int i) mutable {res = i; }
        );
        t.join();
        REQUIRE(res == 6);
    }
    
    REQUIRE(p.valid() == false);
    //REQUIRE(nextP.valid() == false);
}

TEST_CASE("Ready Promise - resolve", "[ready]") {
    int res = 0;
    pro::ReadyPromise<int> p([]()->int {  return 7; });
    REQUIRE(p.valid() == true);
    //ensure async
    REQUIRE(p.ready() == false);
    REQUIRE_THROWS_AS(res = p.get(), std::future_error);
    //wait for promise
    while (p.ready() == false);
    //get result
    res = p.get();
    //test result
    REQUIRE(res == 7);
    REQUIRE(p.resolved());
    REQUIRE(p.rejected() == false);
}

TEST_CASE("Ready Promise - reject", "[ready]") {
    int res = 0;
    pro::ReadyPromise<int> p([]()->int { throw 666; });
    REQUIRE(p.valid() == true);
    //ensure async
    REQUIRE(p.ready() == false);
    REQUIRE_THROWS_AS(res = p.get(), std::future_error);
    //wait for promise
    while (p.ready() == false);
    //get result
    res = p.get();
    //test result
    REQUIRE(res == 666);
    REQUIRE(p.rejected());
    REQUIRE(p.resolved() == false);
}

TEST_CASE("Ready Promise - reject with an exception", "[ready]") {
    int res = 0;
    pro::ReadyPromise<int> p([]()->int { throw std::exception("test"); });
    REQUIRE(p.valid() == true);
    //ensure async
    REQUIRE(p.ready() == false);
    REQUIRE_THROWS_AS(res = p.get(), std::future_error);
    //wait for promise
    while (p.ready() == false);
    //get result
    REQUIRE_THROWS_WITH(res = p.get(), "test");
    REQUIRE(p.rejected());
}

TEST_CASE("Ready Promise - chaining", "[ready]") {
    int res = 0, then_res = 0;
    pro::ReadyPromise<int> p([]()->int { return 7; });

    auto nextP = p.then(
        [&then_res]() mutable { then_res = 8; }
    );

    SECTION("then") {
        REQUIRE(then_res == 8);
    }

    //get result
    res = p.get();
    //test result
    REQUIRE(res == 7);
    REQUIRE(p.resolved());
    REQUIRE(p.rejected() == false);
    REQUIRE(p.valid() == false);
    REQUIRE(nextP.valid() == true);
}

TEST_CASE("Promise manipulations", "[constructors]") {
    int res = 0;
    
    SECTION("move constructor") {
        pro::Promise<int> p([] { return 1; });
        pro::Promise<int> new_p(std::move(p));

        REQUIRE(p.valid() == false);
        REQUIRE(new_p.valid() == true);
    }

    SECTION("operator =") {
        pro::Promise<int> p([] { return 1; });
        pro::Promise<int> new_p = std::move(p);

        REQUIRE(p.valid() == false);
        REQUIRE(new_p.valid() == true);
    }

    
}