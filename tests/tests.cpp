#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "./catch/catch_amalgamated.hpp"
#include "../include/promise.h"

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


TEST_CASE("Promise<T>.then", "[basic]") {
    REQUIRE(wrapThenTypedPromise(pro::Promise<int>([] { return 1; })) == 1);
    REQUIRE(wrapThenTypedPromise(pro::Promise<int>([]() -> int { throw 1; })) == 2);
    REQUIRE(wrapThenTypedPromise(pro::Promise<int>([](int arg) { return arg; }, 10)) == 10);
    REQUIRE(wrapThenTypedPromise(pro::Promise<int>([](int arg) -> int { throw arg; }, 10)) == 20);
}
TEST_CASE("Promises<void>.then", "[basic]") {
    REQUIRE(wrapThenVoidPromise(pro::Promise<void>([]{ })) == 1);
    REQUIRE(wrapThenVoidPromise(pro::Promise<void>([]{ throw 1; })) == 2);
    REQUIRE(wrapThenVoidPromise(pro::Promise<void>([](int arg) { }, 10)) == 1);
    REQUIRE(wrapThenVoidPromise(pro::Promise<void>([](int arg) { throw arg; }, 10)) == 2);
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