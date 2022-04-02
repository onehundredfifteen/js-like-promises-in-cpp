#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include <string>
#include "./catch/catch_amalgamated.hpp"
#include "../include/promise.h"
#include "../include/ready_promise.h"
#include "../include/util.h"

//Test wrappers for Promise<T>.then(resolve, reject)
int wrapThenTypedPromise(pro::Promise<int> &p) {
    int res = 0;
    p.then(
        [&res](int i) { res = i; },
        [&res](int i) { res = i * 2;}
    );
    return res;
}
int wrapThenVoidPromise(pro::Promise<void> &p) {
    int res = 0;
    p.then(
        [&res]() { res = 1; },
        [&res]() { res = 2; }
    );
    return res;
}
//Test wrappers for Promise<T>.then(resolve)
int wrapThenTypedPromise_single(pro::Promise<int>& p) {
    int res = 0;
    p.then(
        [&res](int i) { res = i; }
    );
    return res;
}
int wrapThenVoidPromise_single(pro::Promise<void>& p) {
    int res = 0;
    p.then(
        [&res]() { res = 1; }
    );
    return res;
}
//Test wrapper for Promise<T>.then(resolve, reject, reject_with_exception)
int wrapThenTypedPromise_full(pro::Promise<int>& p) {
    int res = 0;
    p.then(
        [&res](int i) { res = i; },
        [&res](int i) { res = i * 2; },
        [&res](std::exception_ptr p) { res = 3; }
    );
    return res;
}
//Test wrapper for Promise<void>.then(resolve, reject_with_exception)
int wrapThenVoidPromise_full(pro::Promise<void>& p) {
    int res = 0;
    p.then(
        [&res]() { res = 1; },
        [&res](std::exception_ptr p) { res = 2; }
    );
    return res;
}

//thread worker
void worker(std::promise<int>&& stdIntPromise, int a, int b) {
    stdIntPromise.set_value(a * b);
}

///////////////////////
//Tests for Promise<T>
//////////////////////

TEST_CASE("Promise constructors", "[basic]") 
{
    SECTION("function constructor with some arguments") {
        int res = 0;
        pro::Promise<int> p([](int a, long b) { return (int)(a + b); }, 115, 5);

        REQUIRE(p.valid() == true);
        p.then([&res](int i) {res = i;});

        REQUIRE(res == 120);
        REQUIRE(p.valid() == false);
    }

    SECTION("move constructor") {
        pro::Promise<void> p([] {});
        pro::Promise<void> new_p(std::move(p));

        REQUIRE(p.valid() == false);
        REQUIRE(new_p.valid() == true);
    }

    SECTION("operator =") {
        pro::Promise<void> p([] {});
        pro::Promise<void> new_p = std::move(p);

        REQUIRE(p.valid() == false);
        REQUIRE(new_p.valid() == true);
    }

    SECTION("future constructor") {
        int res = 0;
        std::promise<int> std_promise;
        pro::Promise<int> p(std_promise.get_future());

        std::thread t(worker, std::move(std_promise), 2, 3);

        REQUIRE(p.valid() == true);
        p.then([&res](int i) {res = i;});
        t.join();
        REQUIRE(res == 6);
        REQUIRE(p.valid() == false);
    }
}

TEST_CASE("Promise.then", "[basic]") 
{
    SECTION("Promise<T> value handling") {
        REQUIRE(wrapThenTypedPromise(pro::Promise<int>([] { return 1; })) == 1);
        REQUIRE(wrapThenTypedPromise(pro::Promise<int>([]() -> int { throw 1; })) == 2);
        //passing some params
        REQUIRE(wrapThenTypedPromise(pro::Promise<int>([](int arg) { return arg; }, 10)) == 10);
        REQUIRE(wrapThenTypedPromise(pro::Promise<int>([](int arg) -> int { throw arg; }, 10)) == 20);

        REQUIRE(wrapThenTypedPromise_single(pro::Promise<int>([] { return 1; })) == 1);
        REQUIRE(wrapThenTypedPromise_single(pro::Promise<int>([]() -> int { throw std::exception(""); })) == 0);
        REQUIRE_NOTHROW(wrapThenTypedPromise_single(pro::Promise<int>([]() -> int { throw std::exception(""); })));

        REQUIRE(wrapThenTypedPromise_full(pro::Promise<int>([] { return 1; })) == 1);
        REQUIRE(wrapThenTypedPromise_full(pro::Promise<int>([]() -> int { throw 2; })) == 4);
        REQUIRE(wrapThenTypedPromise_full(pro::Promise<int>([]() -> int { throw std::exception(""); })) == 3);
        REQUIRE_NOTHROW(wrapThenTypedPromise_full(pro::Promise<int>([]() -> int { throw 2; })));
        REQUIRE_NOTHROW(wrapThenTypedPromise_full(pro::Promise<int>([]() -> int { throw std::exception(""); })));
    }

    SECTION("Promise<void> handlers") {
        REQUIRE(wrapThenVoidPromise(pro::Promise<void>([] {})) == 1);
        REQUIRE(wrapThenVoidPromise(pro::Promise<void>([] { throw 1; })) == 2);
        REQUIRE(wrapThenVoidPromise(pro::Promise<void>([](int arg) {}, 10)) == 1);
        REQUIRE(wrapThenVoidPromise(pro::Promise<void>([](int arg) { throw arg; }, 10)) == 2);

        REQUIRE(wrapThenVoidPromise_single(pro::Promise<void>([] {})) == 1);
        REQUIRE(wrapThenVoidPromise_single(pro::Promise<void>([] { throw 1; })) == 0);
        REQUIRE_NOTHROW(wrapThenVoidPromise_single(pro::Promise<void>([] { throw 1; })));

        REQUIRE(wrapThenVoidPromise_full(pro::Promise<void>([] {})) == 1);
        REQUIRE(wrapThenVoidPromise_full(pro::Promise<void>([] { throw 1; })) == 2);
        REQUIRE(wrapThenVoidPromise_full(pro::Promise<void>([] { throw std::exception(""); })) == 2);
        REQUIRE_NOTHROW(wrapThenVoidPromise_full(pro::Promise<void>([] { throw 1; })) );
        REQUIRE_NOTHROW(wrapThenVoidPromise_full(pro::Promise<void>([] { throw std::exception(""); })));
    }

    SECTION("real async test") {
        auto start = std::chrono::system_clock::now();
        pro::Promise<void> pv([]() { std::this_thread::sleep_for(std::chrono::milliseconds(115)); });
        pro::Promise<int> pi([]() { std::this_thread::sleep_for(std::chrono::milliseconds(115)); return 1; });
        auto pvp = pv.then([] {});
        auto pip = pi.then([](int) {});

        auto end = std::chrono::system_clock::now();
        REQUIRE(std::chrono::duration_cast <std::chrono::milliseconds> (end - start).count() < 20);
    }

    SECTION("sync test") {
        auto start = std::chrono::system_clock::now();
        pro::Promise<void> pv([]() { std::this_thread::sleep_for(std::chrono::milliseconds(115)); });
        pro::Promise<int> pi([]() { std::this_thread::sleep_for(std::chrono::milliseconds(115)); return 1; });
        pv.then([] {});
        pi.then([](int) {});

        auto end = std::chrono::system_clock::now();
        auto time = std::chrono::duration_cast <std::chrono::milliseconds> (end - start).count();
        REQUIRE(time > 115);
        REQUIRE(time < 200);
    }
}

TEST_CASE("Promise chaining", "[basic]") 
{
    SECTION("chain of 3 then on Promise<void>") {
        std::string res = "";
        pro::Promise<void> p([] {});

        auto cba = [&res]() { res.append("A"); };
        auto cbb = [&res]() { res.append("B"); };
        REQUIRE(p.then(cba, cbb).then(cba, cbb).then(cba, cbb).valid() == true);
        REQUIRE(p.valid() == false);
        REQUIRE(res == "AAA");
    }

    SECTION("chain of 3 then on Promise<void> rejected") {
        std::string res = "";
        pro::Promise<void> p([] { throw std::exception(""); });

        auto cba = [&res]() { res.append("A"); };
        auto cbb = [&res]() { res.append("B"); };
        REQUIRE(p.then(cba, cbb).then(cba, cbb).then(cba, cbb).valid() == true);
        REQUIRE(p.valid() == false);
        REQUIRE(res == "BAA");
    }

    SECTION("chain of 3 then on Promise<int>") {
        std::string res = "";
        pro::Promise<int> p([] { return 1; });

        auto cba = [&res](int) { res.append("A"); return 1; };
        auto cbb = [&res](int) { res.append("B"); return 1; };
        REQUIRE(p.then(cba, cbb).then(cba, cbb).then(cba, cbb).valid() == true);
        REQUIRE(p.valid() == false);
        REQUIRE(res == "AAA");
    }

    SECTION("chain of 3 then on Promise<int> rejected with an int") {
        std::string res = "";
        pro::Promise<int> p([]()->int { throw 1; });

        auto cba = [&res](int) { res.append("A"); return 1; };
        auto cbb = [&res](int) { res.append("B"); return 1; };
        auto cbc = [&res](std::exception_ptr) { res.append("C"); return 0; };

        REQUIRE(p.then(cba, cbb, cbc).then(cba, cbb, cbc).then(cba, cbb, cbc).valid() == true);
        REQUIRE(p.valid() == false);
        REQUIRE(res == "BAA");
    }

    SECTION("chain of 3 then on Promise<int> rejected with an exception") {
        std::string res = "";
        pro::Promise<int> p([]()->int { throw std::exception(""); });

        auto cba = [&res](int) { res.append("A"); return 1; };
        auto cbb = [&res](int) { res.append("B"); return 1; };
        auto cbc = [&res](std::exception_ptr) { res.append("C"); return 0; };

        REQUIRE(p.then(cba, cbb, cbc).then(cba, cbb, cbc).then(cba, cbb, cbc).valid() == true);
        REQUIRE(p.valid() == false);
        REQUIRE(res == "CAA");
    }

    SECTION("real async chain test") {
        auto start = std::chrono::system_clock::now();
        auto fnSleep = []() {std::this_thread::sleep_for(std::chrono::milliseconds(115)); };

        pro::Promise<void> pv(fnSleep);
        auto pvp = pv.then(fnSleep).then(fnSleep).then(fnSleep);
        auto end = std::chrono::system_clock::now();

        REQUIRE(std::chrono::duration_cast <std::chrono::milliseconds> (end - start).count() < 20);
    }
}
 
TEST_CASE("Promise error propagation", "[exceptions]") 
{
    SECTION("Promise<void> propagate error") {
        std::string res = "";
        pro::Promise<void> p([] { throw std::exception(""); });

        auto cba = [&res]() { res.append("A"); };
        auto cbc = [&res](std::exception_ptr) { res.append("C"); };
        p.then(cba).then(cba).fail(cbc);
        REQUIRE(p.valid() == false);
        REQUIRE(res == "C");
    }

    SECTION("Promise <int> propagate error") {
        std::string res = "";
        pro::Promise<int> p([]()->int { throw std::exception(""); });

        auto cba = [&res](int) { res.append("A"); return 0; };
        auto cbb = [&res](int) { res.append("B"); return 0; };
        auto cbc = [&res](std::exception_ptr) { res.append("C"); return 0; };
        //in 1st then we handling only T, not exception
        REQUIRE(p.then(cba, cbb).then(cba).fail(cbb, cbc).valid() == true);
        REQUIRE(p.valid() == false);
        REQUIRE(res == "C");
    }

    SECTION("Promise <int> propagate error with T rejection handler") {
        std::string res = "";
        pro::Promise<int> p([]()->int { throw 1; });

        auto cba = [&res](int) { res.append("A"); return 0; };
        auto cbb = [&res](int) { res.append("B"); return 0; };
        auto cbc = [&res](std::exception_ptr) { res.append("C"); return 0; };
        REQUIRE(p.then(cba).then(cba, cbb).fail(cbb, cbc).valid() == true);
        REQUIRE(p.valid() == false);
        REQUIRE(res == "B");
    }

    SECTION("Promise <int> reject, rethrow & catch with different types of exceptions") {
        std::string res = "";
        pro::Promise<int> p([]()->int { throw 1; });

        auto cba = [&res](int) { res.append("A"); return 0; };
        auto cbb = [&res](int) { res.append("B"); return 0; };
        auto cbc = [&res](std::exception_ptr) { res.append("C"); return 0; };

        p.then(cba).fail(cbb, cbc).then(
            //here 1st exception should be stopped and this hndler is cb to resolve
            [&res](int)->int { res.append("D"); throw std::exception(""); }, cbb
        ).then(cba).then(cba, cbb, cbc);

        REQUIRE(p.valid() == false);
        REQUIRE(res == "BDC");
    }

    SECTION("Ensure exception type") {
        std::string res = "";
        std::exception_ptr eptrEx;
        pro::Promise<std::string> p([]()->std::string { throw std::string("Fido"); });

        auto cbb = [](std::string x)->std::string { throw std::invalid_argument(x.c_str()); };
        auto cbc = [&eptrEx](std::exception_ptr ex)->std::string { eptrEx = ex; return "Dido"; };

        p.fail(cbb, cbc).fail(cbb, cbc);

        try {
            std::rethrow_exception(eptrEx);
        }
        catch (std::invalid_argument &ex) {
            res = ex.what();
        }
        catch (...) {
            res = "Dino";
        }
        //Dino can be thrown, if there is no explicit convertion to std::string from char*
        REQUIRE(res == "Fido");
    }
 }

///////////////////////////
//Tests for ReadyPromise<T>
///////////////////////////

TEST_CASE("ReadyPromise constructors", "[rp basic]")
{
    SECTION("function constructor with some arguments") {
        int res = 0;
        pro::ReadyPromise<int> p([](int a, long b) { return (int)(a + b); }, 115, 5);

        REQUIRE(p.valid() == true);
        //wait for result
        while (p.ready() == false);

        REQUIRE(p.get() == 120);
        //ReadyPromise is valid until call of .then
        REQUIRE(p.valid() == true);
    }
    /*
    SECTION("move constructor") {
        pro::ReadyPromise<int> p([] { return 1; });
        pro::ReadyPromise<int> new_p(std::move(p));

        REQUIRE(p.valid() == false);
        REQUIRE(new_p.valid() == true);
    }

    SECTION("operator =") {
        pro::ReadyPromise<int> p([] { return 1; });
        pro::ReadyPromise<int> new_p = std::move(p);

        REQUIRE(p.valid() == false);
        REQUIRE(new_p.valid() == true);
    }
    
    SECTION("future constructor") {
        int res = 0;
        std::promise<int> std_promise;
        pro::ReadyPromise<int> p(std_promise.get_future());

        std::thread t(worker, std::move(std_promise), 2, 3);

        REQUIRE(p.ready() == false);
        t.join();
        REQUIRE(p.ready() == true);
        REQUIRE(p.get() == 6);
        REQUIRE(p.resolved() == true);
    }*/
}

TEST_CASE("ReadyPromise value handling", "[rp basic]") 
{
    SECTION("Resolving ReadyPromise") {
        int res = 0;
        pro::ReadyPromise<int> p([] { return 7; });
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

    SECTION("Rejecting ReadyPromise") {
        int res = 0;
        pro::ReadyPromise<int> p([]()->int { throw 666; });
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

    SECTION("Rejecting ReadyPromise with an exception") {
        int res = 0;
        pro::ReadyPromise<int> p([]()->int { throw std::exception("test"); });
        //ensure async
        REQUIRE(p.ready() == false);
        REQUIRE_THROWS_AS(res = p.get(), std::future_error);
        //wait for promise
        while (p.ready() == false);
        //get result
        REQUIRE_THROWS_WITH(res = p.get(), "test");
        REQUIRE(p.rejected());
    }
}

TEST_CASE("ReadyPromise chaining", "[rp basics]") 
{
    SECTION("Resolve, then perform some operations") {
        std::string res = "";
        pro::ReadyPromise<int> p([] { return 7; });

        auto con = [&res]() { res.append("P"); return 1; };
        auto cba = [&res](int) { res.append("A"); return 1; };
        auto cbb = [&res](int) { res.append("B"); return 1; };
        auto cbc = [&res](std::exception_ptr) { res.append("C"); return 0; };

        REQUIRE(p.valid() == true);
        REQUIRE(p.then(con).then(cba, cbb, cbc).valid() == true);
        REQUIRE(p.valid() == false);
        REQUIRE(res == "PA");

        //test result
        REQUIRE(p.get() == 7);
        REQUIRE(p.resolved());
        REQUIRE(p.rejected() == false);
    }

    SECTION("Reject, then perform some operations") {
        std::string res = "";
        pro::ReadyPromise<int> p([]()->int { throw 7; });

        auto con = [&res]() { res.append("P"); return 1; };
        auto cba = [&res](int) { res.append("A"); return 1; };
        auto cbb = [&res](int) { res.append("B"); return 1; };
        auto cbc = [&res](std::exception_ptr) { res.append("C"); return 0; };

        REQUIRE(p.valid() == true);
        REQUIRE(p.then(con).then(cba, cbb, cbc).valid() == true);
        REQUIRE(p.valid() == false);
        REQUIRE(res == "PA");

        //test result
        REQUIRE(p.get() == 7);
        REQUIRE(p.resolved() == false);
        REQUIRE(p.rejected() == true);
    }
}

///////////////////////////
//Tests for Utils
///////////////////////////

TEST_CASE("Promise all", "[util]") 
{
    SECTION("Result validation") {
        REQUIRE(pro::PromiseAll(pro::Promise<int>([] { return 1; })).valid() == true);
    }

    SECTION("All promises resolved") {
        int res = 0;
        pro::Promise<int> p1([] { return 1; });
        pro::Promise<long> p2([] { return 9L; });

        pro::PromiseAll(p1, p2).then(
            [&res](auto tuple) { res = std::get<0>(tuple) + (int)std::get<1>(tuple); }
        );
        REQUIRE(res == 10);
    }
    /*
    SECTION("real async test") {
        auto start = std::chrono::system_clock::now();
        auto p = pro::PromiseAll(pro::Promise<int> ([] { return 1; }), 
            pro::Promise<char>([] { return 'x'; }),
            pro::Promise<std::string>([] { return std::string("str"); }),
            pro::Promise<int>([] { return 1; })).then(
            [](auto tuple) { }
        );

        auto end = std::chrono::system_clock::now();
        REQUIRE(std::chrono::duration_cast <std::chrono::milliseconds> (end - start).count() < 20);
    }*/

    SECTION("Some promise fails /*and the result comes from the fastest one*/") {
        int res = 0;
        pro::Promise<int> p1([] { return 1; });
        /*pro::Promise<char> p2([]()->char { 
            std::this_thread::sleep_for(std::chrono::milliseconds(666)); 
            throw 'x'; 
        });*/
        pro::Promise<long> p2([]()->long { throw 115L; });

        //res should be never less than 0.
        //Second callback should be never called, because
        //no promise throws a tuple<int, long>.

        pro::PromiseAll(p1, p2).then(
            [&res](auto tuple) { res = -1;},
            [&res](auto tuple) { res = -2;},
            [&res](std::exception_ptr eptr) {
                try {
                    std::rethrow_exception(eptr);
                }
                catch (long v) {
                    res = v;
                }
                catch (...) {
                    res = -3;
                }
            }
        );
        REQUIRE(res > 0);
        REQUIRE(res == 115);

    }


}