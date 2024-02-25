#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file

#include <string>
#include "./catch/catch_amalgamated.hpp"
#include "../include/promise.h"
#include "../include/ready_promise.h"
#include "../include/util.h"

//Test wrappers for promise<T>.then(resolve, reject)
int wrapThenTypedPromise(pro::promise<int> &p) {
    int res = 0;
    p.then(
        [&res](int i) { res = i; },
        [&res](int i) { res = i * 2;}
    );
    return res;
}
int wrapThenVoidPromise(pro::promise<void> &p) {
    int res = 0;
    p.then(
        [&res]() { res = 1; },
        [&res]() { res = 2; }
    );
    return res;
}
//Test wrappers for promise<T>.then(resolve)
int wrapThenTypedPromise_single(pro::promise<int>& p) {
    int res = 0;
    p.then(
        [&res](int i) { res = i; }
    );
    return res;
}
int wrapThenVoidPromise_single(pro::promise<void>& p) {
    int res = 0;
    p.then(
        [&res]() { res = 1; }
    );
    return res;
}
//Test wrapper for promise<T>.then(resolve, reject, reject_with_exception)
int wrapThenTypedPromise_full(pro::promise<int>& p) {
    int res = 0;
    p.then(
        [&res](int i) { res = i; },
        [&res](int i) { res = i * 2; },
        [&res](std::exception_ptr p) { res = 3; }
    );
    return res;
}
//Test wrapper for promise<void>.then(resolve, reject, reject_with_exception)
int wrapThenVoidPromise_full(pro::promise<void>& p) {
    int res = 0;
    p.then(
        [&res]() { res = 1; },
        [&res]() { res = 2; },
        [&res](std::exception_ptr p) { res = 4; }
    );
    return res;
}
//Container wrapper
/*
[&res](auto vec) {
    res = std::reduce(vec.begin(), vec.end());
}*/

//thread worker
void worker(std::promise<int>&& stdIntPromise, int a, int b) {
    stdIntPromise.set_value(a * b);
}

//promise helper methods
int returnInt(int arg) {
    return arg;
}
void sleepAndReturn(int sleep) {
    std::this_thread::sleep_for(std::chrono::milliseconds(sleep));
}
int sleepAndReturnInt(int sleep, int arg) {
    sleepAndReturn(sleep);
    return arg;
}

//dummy methods
void dummy(){}
auto dummyHandler = [](auto) {};
int get115() { return 115; }


//Copy counter class
struct CopyCounter {
    int copied;
    int moved;
    int constructed;

    CopyCounter() : copied(0), moved(0), constructed(1) {}
    CopyCounter(const CopyCounter& cc) : 
        copied(cc.copied + 1), moved(cc.moved), constructed(cc.constructed) {
    }
    CopyCounter(CopyCounter&& cc) noexcept :
        copied(std::exchange(cc.copied, 0)),
        moved(std::exchange(cc.moved, 0)),
        constructed(std::exchange(cc.constructed, 0)){
        moved += 1;
    }
    CopyCounter& operator=(const CopyCounter& cc) {
        copied = cc.copied + 1;
        moved = cc.moved;
        constructed = cc.constructed;
        return *this;
    }
    CopyCounter& operator=(CopyCounter&& cc) noexcept {
        copied = std::exchange(cc.copied, 0);
        moved = std::exchange(cc.moved, 0);
        constructed = std::exchange(cc.constructed, 0);
        moved += 1;
        return *this;
    }  
};

CopyCounter returnCounter() {
    return CopyCounter();
}


///////////////////////
//Tests for promise<T>
//////////////////////

TEST_CASE("Promise constructors", "[basic]") 
{
    SECTION("promise constructor with a lamba returning string") {
        std::string res = "";
        pro::promise<std::string> p([]()->std::string {
            return std::string("promised string");
        });

        REQUIRE(p.valid() == true);
        p.then([&res](const std::string &i) {res = i; });

        REQUIRE(res == "promised string");
        REQUIRE(p.valid() == false);
    }

    SECTION("promise with a std::function") {
        int res = 0;
        std::function<int(void)> fun = get115;
        pro::promise<int> p(fun);

        REQUIRE(p.valid() == true);
        p.then([&res](const int& i) {res = i; });

        REQUIRE(res == get115());
        REQUIRE(p.valid() == false);
    }
    
    SECTION("promise constructor with arguments for function") {
        int res = 0;
        pro::promise<int> p([](int a, long b) { 
                return (int)(a + b); 
            }, 
            115, 5
        );

        REQUIRE(p.valid() == true);
        p.then([&res](int i) {res = i;});

        REQUIRE(res == 120);
        REQUIRE(p.valid() == false);
    }

    SECTION("move constructor") {
        pro::promise<void> p(dummy);
        pro::promise<void> new_p(std::move(p));

        REQUIRE(p.valid() == false);
        REQUIRE(new_p.valid() == true);
    }

    SECTION("operator =") {
        pro::promise<void> p([] {});
        pro::promise<void> new_p = std::move(p);

        REQUIRE(p.valid() == false);
        REQUIRE(new_p.valid() == true);
    }

    SECTION("std::future constructor") {
        int res = 0;
        std::promise<int> std_promise;
        pro::promise<int> p(std_promise.get_future());

        std::thread t(worker, std::move(std_promise), 2, 3);

        REQUIRE(p.valid() == true);
        p.then([&res](int i) {res = i;});
        t.join();
        REQUIRE(res == 6);
        REQUIRE(p.valid() == false);
    }

    SECTION("externally resolvable constructor") {
        int res = 0;
        std::function<void(std::promise<int>)> fun = [](std::promise<int> resolver) {
            auto inner = [&resolver](int a) {
                resolver.set_value(a);
            };
            inner(115);
        };
        pro::promise<int> p(fun);

        REQUIRE(p.valid() == true);
        p.then([&res](int i) {res = i; });

        REQUIRE(res == 115);
        REQUIRE(p.valid() == false);
    }

    SECTION("making a promise") {
        int res = 0;
        auto p = pro::make_promise<int>([](int a, long b) {
            return (int)(a + b);
        }, 115, 5);
        
        REQUIRE(p.valid() == true);
        p.then([&res](int i) {res = i; });

        REQUIRE(res == 120);
        REQUIRE(p.valid() == false);
    }

    SECTION("making a rejected promise") {
        int res = 0;
        auto p = pro::make_rejected_promise<int>(115);

        REQUIRE(p.valid() == true);
        p.fail([&res](int i) {
            res = i;
        }, [&res](std::exception_ptr eptr) {
            res = 420;
        });

        REQUIRE(res == 115);
        REQUIRE(p.valid() == false);
    }

    SECTION("converting to std::future") {
        int res = 0;
        pro::promise<int> p(sleepAndReturnInt, 200, 1);
        std::future<int> fut = p;

        REQUIRE(p.valid() == false);
        
        res = fut.get();
        REQUIRE(res == 1);
    }

    SECTION("fixed value resolving constructor") {
        int res = 0;
        pro::promise<int> p(115);

        REQUIRE(p.valid() == true);

        std::future<int> fut = p;
        res = fut.get();
        REQUIRE(res == 115);
    }

    SECTION("fixed value rejecting constructor") {
        pro::promise<int> p(std::make_exception_ptr(115));

        REQUIRE(p.valid() == true);

        std::future<int> fut = p;
        REQUIRE_THROWS_AS(fut.get(), int);
    }
}

TEST_CASE("Promise.then", "[basic]") 
{
    SECTION("Promise<T> value handling") {
        REQUIRE(wrapThenTypedPromise(pro::promise<int>(returnInt, 1)) == 1);
        REQUIRE(wrapThenTypedPromise(pro::promise<int>([]() -> int { throw 1; })) == 2);
        //passing some params
        REQUIRE(wrapThenTypedPromise(pro::promise<int>([](int arg) { return arg; }, 10)) == 10);
        REQUIRE(wrapThenTypedPromise(pro::promise<int>([](int arg) -> int { throw arg; }, 10)) == 20);

        REQUIRE(wrapThenTypedPromise_single(pro::promise<int>(returnInt, 1)) == 1);
        REQUIRE(wrapThenTypedPromise_single(pro::promise<int>([]() -> int { throw std::exception(""); })) == 0);
        REQUIRE_NOTHROW(wrapThenTypedPromise_single(pro::promise<int>([]() -> int { throw std::exception(""); })));

        REQUIRE(wrapThenTypedPromise_full(pro::promise<int>(returnInt, 1)) == 1);
        REQUIRE(wrapThenTypedPromise_full(pro::promise<int>([]() -> int { throw 2; })) == 4);
        REQUIRE(wrapThenTypedPromise_full(pro::promise<int>([]() -> int { throw std::exception(""); })) == 3);
        REQUIRE_NOTHROW(wrapThenTypedPromise_full(pro::promise<int>([]() -> int { throw 2; })));
        REQUIRE_NOTHROW(wrapThenTypedPromise_full(pro::promise<int>([]() -> int { throw std::exception(""); })));
    }

    SECTION("Promise<void> handlers") {
        REQUIRE(wrapThenVoidPromise(pro::promise<void>(dummy)) == 1);
        REQUIRE(wrapThenVoidPromise(pro::promise<void>([] { throw 1; })) == 2);
        REQUIRE(wrapThenVoidPromise(pro::promise<void>([](int arg) {}, 10)) == 1);
        REQUIRE(wrapThenVoidPromise(pro::promise<void>([](int arg) { throw arg; }, 10)) == 2);

        REQUIRE(wrapThenVoidPromise_single(pro::promise<void>(dummy)) == 1);
        REQUIRE(wrapThenVoidPromise_single(pro::promise<void>([] { throw 1; })) == 0);
        REQUIRE_NOTHROW(wrapThenVoidPromise_single(pro::promise<void>([] { throw 1; })));

        REQUIRE(wrapThenVoidPromise_full(pro::promise<void>(dummy)) == 1);
        REQUIRE(wrapThenVoidPromise_full(pro::promise<void>([] { throw 1; })) == 2);
        REQUIRE(wrapThenVoidPromise_full(pro::promise<void>([] { throw std::exception(""); })) == 4);
        REQUIRE_NOTHROW(wrapThenVoidPromise_full(pro::promise<void>([] { throw 1; })) );
        REQUIRE_NOTHROW(wrapThenVoidPromise_full(pro::promise<void>([] { throw std::exception(""); })));
    }
}

TEST_CASE("Promise chaining", "[basic]") 
{
    SECTION("chain of 3 then on Promise<void>") {
        std::string res = "";
        pro::promise<void> p([] {});

        auto cba = [&res]() { res.append("A"); };
        auto cbb = [&res]() { res.append("B"); };
        REQUIRE(p.then(cba, cbb).then(cba, cbb).then(cba, cbb).valid() == true);
        REQUIRE(p.valid() == false);
        REQUIRE(res == "AAA");
    }

    SECTION("chain of 3 then on Promise<void> rejected") {
        std::string res = "";
        pro::promise<void> p([] { throw std::exception(""); });

        auto cba = [&res]() { res.append("A"); };
        auto cbb = [&res]() { res.append("B"); };
        REQUIRE(p.then(cba, cbb).then(cba, cbb).then(cba, cbb).valid() == true);
        REQUIRE(p.valid() == false);
        REQUIRE(res == "BAA");
    }

    SECTION("chain of 3 then on Promise<int>") {
        std::string res = "";
        pro::promise<int> p([] { return 1; });

        auto cba = [&res](int) { res.append("A"); return 1; };
        auto cbb = [&res](int) { res.append("B"); return 1; };
        REQUIRE(p.then(cba, cbb).then(cba, cbb).then(cba, cbb).valid() == true);
        REQUIRE(p.valid() == false);
        REQUIRE(res == "AAA");
    }

    SECTION("chain of 3 then on Promise<int> rejected with an int") {
        std::string res = "";
        pro::promise<int> p([]()->int { throw 1; });

        auto cba = [&res](int) { res.append("A"); return 1; };
        auto cbb = [&res](int) { res.append("B"); return 1; };
        auto cbc = [&res](std::exception_ptr) { res.append("C"); return 0; };

        REQUIRE(p.then(cba, cbb, cbc).then(cba, cbb, cbc).then(cba, cbb, cbc).valid() == true);
        REQUIRE(p.valid() == false);
        REQUIRE(res == "BAA");
    }

    SECTION("chain of 3 then on Promise<int> rejected with an exception") {
        std::string res = "";
        pro::promise<int> p([]()->int { throw std::exception(""); });

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

        pro::promise<void> pv(fnSleep);
        auto pvp = pv.then(fnSleep).then(fnSleep).then(fnSleep);
        auto end = std::chrono::system_clock::now();

        REQUIRE(std::chrono::duration_cast <std::chrono::milliseconds> (end - start).count() < 20);
    }
}
 
TEST_CASE("Promise error propagation", "[exceptions]") 
{
    SECTION("Promise<void> propagate error") {
        std::string res = "";
        pro::promise<void> p([] { throw std::exception(""); });

        auto cba = [&res]() { res.append("A"); };
        auto cbc = [&res](std::exception_ptr) { res.append("C"); };
        p.then(cba).then(cba).fail(cbc);
        REQUIRE(p.valid() == false);
        REQUIRE(res == "C");
    }

    SECTION("Promise <int> propagate error") {
        std::string res = "";
        pro::promise<int> p([]()->int { throw std::exception(""); });

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
        pro::promise<int> p([]()->int { throw 1; });

        auto cba = [&res](int) { res.append("A"); return 0; };
        auto cbb = [&res](int) { res.append("B"); return 0; };
        auto cbc = [&res](std::exception_ptr) { res.append("C"); return 0; };
        REQUIRE(p.then(cba).then(cba, cbb).fail(cbb, cbc).valid() == true);
        REQUIRE(p.valid() == false);
        REQUIRE(res == "B");
    }

    SECTION("Promise <int> reject, rethrow & catch with different types of exceptions") {
        std::string res = "";
        pro::promise<int> p([]()->int { throw 1; });

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
        pro::promise<std::string> p([]()->std::string { throw std::string("Fido"); });

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

TEST_CASE("Promise async", "[async]")
{
    /*tests should take no longer than 20ms, 
      despite promise sleeping for much longer time
     */
    SECTION("Promise is asynchronous") {
        auto start = std::chrono::system_clock::now();
        pro::promise<void> p(sleepAndReturn, 115);

        auto pp = p.then(dummy);

        auto end = std::chrono::system_clock::now();
        REQUIRE(20 > std::chrono::duration_cast <std::chrono::milliseconds> (end - start).count());
    }

    SECTION("Promise can be blocking") {
        auto start = std::chrono::system_clock::now();
        pro::promise<void> p(sleepAndReturn, 115);

        p.then(dummy);

        auto end = std::chrono::system_clock::now();
        REQUIRE(115 <= std::chrono::duration_cast <std::chrono::milliseconds> (end - start).count());
    }

    SECTION("Promise is asynchronous") {
        auto start = std::chrono::system_clock::now();
        pro::promise<void> p(sleepAndReturn, 115);

        p.then(dummy).async();

        auto end = std::chrono::system_clock::now();
        REQUIRE(20 > std::chrono::duration_cast <std::chrono::milliseconds> (end - start).count());
    }
    
    SECTION("Promise can be resolved with a different value later") {
        int res = 0;
        auto start = std::chrono::system_clock::now();
        pro::promise<int> p(sleepAndReturnInt, 500, 666);
        p.resolve(420);
        p.then([&res](auto i) { res = i; });
             
        auto end = std::chrono::system_clock::now();
        REQUIRE(res == 420);
        REQUIRE(20 > std::chrono::duration_cast <std::chrono::milliseconds> (end - start).count());
    }

    SECTION("Promise can be rejected with a different value later") {
        int res = 0;
        auto start = std::chrono::system_clock::now();
        pro::promise<int> p(sleepAndReturnInt, 500, 666);
        p.reject(420);
        p.then([&res](auto i) { res = i; }, [&res](auto i) { res = i; });

        auto end = std::chrono::system_clock::now();
        REQUIRE(res == 420);
        REQUIRE(200 > std::chrono::duration_cast <std::chrono::milliseconds> (end - start).count());
    }

    SECTION("Promise can be rejected with a custom exception later") {
        int res = 0;
        auto start = std::chrono::system_clock::now();
        std::exception test_exception("testing reject");
        pro::promise<int> p(sleepAndReturnInt, 500, 666);
        p.reject(std::make_exception_ptr(test_exception));
        p.then([&res](auto i) { res = i; }).fail(
            [&res](std::exception_ptr eptr) {
            try {
                std::rethrow_exception(eptr);
            }
            catch (std::exception &) {
                res = 420;
            }
            catch (...) {
                res = 115;
            }
        });

        auto end = std::chrono::system_clock::now();
        REQUIRE(res == 420);
        REQUIRE(200 > std::chrono::duration_cast <std::chrono::milliseconds> (end - start).count());
    }

    SECTION("Promise<void> can be resolved - somehow cancelled") {
        auto start = std::chrono::system_clock::now();
        pro::promise<void> p(sleepAndReturn, 500);
        p.resolve();

        auto end = std::chrono::system_clock::now();
        REQUIRE(p.valid());
        REQUIRE(20 > std::chrono::duration_cast <std::chrono::milliseconds> (end - start).count());
    }
}

///////////////////////////
//Tests for readypromise<T>
///////////////////////////

TEST_CASE("ReadyPromise constructors", "[rp basic]")
{
    SECTION("function constructor with some arguments") {
        pro::readypromise<int> p([](int a, long b) { return (int)(a + b); }, 115, 5);

        //ReadyPromise is valid until call of .then or .get
        REQUIRE(p.valid() == true);
        REQUIRE(p.pending() == true);
        REQUIRE(p.get() == 120);
        REQUIRE(p.resolved() == true);
        REQUIRE(p.valid() == false);
        REQUIRE(p.pending() == false);
        //ensure get again
        REQUIRE(p.get() == 120);
    }
    /*
    SECTION("move constructor") {
        pro::readypromise<int> p(returnInt, 1);
        pro::readypromise<int> new_p(std::move(p));

        REQUIRE(p.valid() == false);
        REQUIRE(new_p.valid() == true);
        REQUIRE(new_p.get() == 1);
    }
    /*
    SECTION("operator =") {
        pro::readypromise<int> p(returnInt, 1);
        pro::readypromise<int> new_p = std::move(p);

        REQUIRE(p.valid() == false);
        REQUIRE(new_p.valid() == true);
        REQUIRE(new_p.get() == 1);
    }*/
    
    SECTION("future constructor") {
        int res = 0;
        std::promise<int> std_promise;
        pro::readypromise<int> p(std_promise.get_future());

        std::thread t(worker, std::move(std_promise), 2, 3);

        REQUIRE(p.pending() == true);
        t.join();
        REQUIRE(p.pending() == false);
        REQUIRE(p.get() == 6);
        REQUIRE(p.resolved() == true);
    }
}

TEST_CASE("ReadyPromise value handling", "[rp basic]") 
{
    SECTION("Resolving ReadyPromise") {
        int res = 0;
        int subscribed = 0;
        std::exception_ptr res_eptr = nullptr;

        pro::readypromise<int> p(returnInt, 115);
        p.onResolve([&subscribed](int s) {subscribed = s;});

        p.then([&res, &res_eptr](int a, std::exception_ptr eptr) {
            res = a;
            res_eptr = eptr;
        });

        CHECK(res == 115);
        CHECK(res_eptr == nullptr);
        REQUIRE(p.resolved() == true);
        REQUIRE(p.pending() == false);
        REQUIRE(p.get() == res);
        REQUIRE(subscribed == res);
    }
    
    SECTION("Rejecting ReadyPromise") {
        int res = 0;
        int subscribed = 0;
        std::exception_ptr res_eptr = nullptr;

        pro::readypromise<int> p([]()->int { throw 666; });
        p.onResolve([&subscribed](int s) {subscribed = 2 * s; });
        p.onReject([&subscribed](int s) {subscribed = s; });

        p.then([&res, &res_eptr](int a, std::exception_ptr eptr) {
            res = a;
            res_eptr = eptr;
        });

        CHECK(res == 666);
        CHECK(res_eptr != nullptr);
        REQUIRE(p.rejected() == true);
        REQUIRE(p.pending() == false);
        REQUIRE(p.get() == res);
        REQUIRE_THROWS_AS(std::rethrow_exception(res_eptr), int);
    }
    
    SECTION("Rejecting ReadyPromise with an exception") {
        int res = 0;
        int subscribed = 0;
        std::exception_ptr res_eptr = nullptr;

        pro::readypromise<int> p([]()->int { throw std::exception("test"); });
        p.onResolve([&subscribed](int s) {subscribed = 2 * s; });
        p.onReject([&subscribed](int s) {subscribed = s; });

        p.then([&res, &res_eptr](int a, std::exception_ptr eptr) {
            res = a;
            res_eptr = eptr;
        });

        CHECK(res == 0);
        CHECK(res_eptr != nullptr);
        REQUIRE(subscribed == 0);
        REQUIRE(p.rejected() == true);
        REQUIRE(p.pending() == false);
        REQUIRE_THROWS_AS(std::rethrow_exception(res_eptr), std::exception);
    }
}

///////////////////////////
//Tests for Utils
///////////////////////////

TEST_CASE("PromiseAll on a different type list", "[util]")
{
    SECTION("Result validation") {
        REQUIRE(pro::PromiseAll(pro::promise<int>([] { return 1; })).valid() == true);
    }

    SECTION("All promises resolved") {
        int res = 0;
        pro::promise<int> p1(returnInt, 1);
        pro::promise<long> p2([] { return 9L; });

        pro::PromiseAll(p1, p2).then(
            [&res](auto tuple) { res = std::get<0>(tuple) + (int)std::get<1>(tuple); }
        );

        REQUIRE(res == 10);
    }

    SECTION("Some promises are rejected") {
        int res = 0;
        pro::promise<int> p1(returnInt, 1);
        pro::promise<int> p2([]()->int { throw 115; });

        pro::PromiseAll(p1, p2).then(
            [&res](auto tuple) {
            res = std::get<0>(tuple) + (int)std::get<1>(tuple);
        }).fail(
            [&res](std::exception_ptr eptr) {
            try {
                std::rethrow_exception(eptr);
            }
            catch (int v) {
                res = v;
            }
            catch (...) {
                res = 420;
            }
        });

        REQUIRE(res == 115);
    }

    SECTION("PromiseAll itself is asynchronous") {
        auto start = std::chrono::system_clock::now();

        //test should take no logner than 20ms, 
        //despite promise sleeping for 115ms 

        pro::promise<int> p1(returnInt, 1);

        auto p = pro::PromiseAll(p1,
            pro::promise<long>([] { return 9L; }),
            pro::make_promise<char>([] {
            std::this_thread::sleep_for(std::chrono::milliseconds(115));
            return 'x'; })
        ).then([](auto) {});

        auto end = std::chrono::system_clock::now();
        REQUIRE(20 > std::chrono::duration_cast <std::chrono::milliseconds> (end - start).count());
    }

    SECTION("Sync PromiseAll resolve all its methods asynchronously") {
        auto start = std::chrono::system_clock::now();

        //test should take no logner than 1000ms, 
        //despite promises sleeping for 666ms each 

        pro::promise<int> p1(sleepAndReturnInt, 666, 1);
        pro::PromiseAll(p1,
            pro::promise<long>([] { return 9L; }),
            pro::promise<char>([] {
            std::this_thread::sleep_for(std::chrono::milliseconds(666));
            return 'x'; })
        ).then(dummyHandler);

        auto end = std::chrono::system_clock::now();
        REQUIRE(1000 > std::chrono::duration_cast <std::chrono::milliseconds> (end - start).count());
    }

    SECTION("Two promise fails and the result comes from the fastest one") {
        int res = 0;

        pro::promise<int> p1(returnInt, 1);
        pro::promise<char> p2([]()->char {
            std::this_thread::sleep_for(std::chrono::milliseconds(666));
            throw 'x';
        });
        pro::promise<long> p3([]()->long { throw 115L; });

        //res should be never less than 0.
        //Second callback should be never called, because
        //no promise throws a tuple<int, char, long>.

        pro::PromiseAll(p1, p2, p3).then(
            [&res](auto tuple) { res = -1; },
            [&res](auto tuple) { res = -2; },
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
        });

        CHECK(res > 0);
        REQUIRE(res == 115);
    }

    SECTION("Invalid promise will return default value, not throw") {
        int res = 0;

        pro::promise<int> p(returnInt, 115);
        CHECK(p.valid() == true);

        p.then([](int) {});
        REQUIRE(p.valid() == false);

        pro::PromiseAll(p,
            pro::promise<int>(returnInt, 420)
        ).then(
            [&res](auto tuple) {
                res = std::get<0>(tuple) + std::get<1>(tuple);
            }
        ).fail(
            [&res](std::exception_ptr eptr) { res = -1; }
        );

        CHECK(res > 0);
        REQUIRE(res == 420);
    }
}

TEST_CASE("PromiseAll on a collection", "[util]")
{
    /*test case covers base engine feats as well for promiseRace and promiseAny*/
    SECTION("Result validation on an empty collection") {
        REQUIRE(pro::PromiseAll(std::vector<pro::promise<int>>()).valid() == true);
    }
    
    SECTION("All promises are resolved") {
        int res = 0;

        std::vector<pro::promise<int>> v;
        v.emplace_back(pro::make_promise<int>(returnInt, 115));
        v.emplace_back(pro::make_promise<int>(returnInt, 666));
       
        pro::PromiseAll(v).then(
            [&res](auto vec) { 
                res = std::reduce(vec.begin(), vec.end());
            }
        );

        CHECK(res > 0);
        REQUIRE(res == 115 + 666);
    }

    SECTION("All promises are resolved in the same order were as passed") {
        std::string res = "";

        std::vector<pro::promise<std::string>> v;
        v.emplace_back(pro::make_promise<std::string>([]()->std::string { return "AB"; })); 
        v.emplace_back(pro::make_promise<std::string>([]()->std::string { 
            std::this_thread::sleep_for(std::chrono::milliseconds(115));
            return "CD"; 
        }));
        v.emplace_back(pro::make_promise<std::string>([]()->std::string { return "EF"; }));

        pro::PromiseAll(v).then(
            [&res](auto vec) {
                   res = std::reduce(vec.begin(), vec.end());
            }
        );

        CHECK(res != "");
        REQUIRE(res == "ABCDEF");
    }

    SECTION("All void promises are resolved") {
        int res = 0;

        std::vector<pro::promise<void>> v;
        v.emplace_back(pro::make_promise<void>(dummy));
        v.emplace_back(pro::make_promise<void>(dummy));

        pro::PromiseAll(v).then(
            [&res]() { res = 115;},
            [&res]() { res = 666;}
        );

        CHECK(res > 0);
        REQUIRE(res == 115);
    }
    
    SECTION("Some promises are rejected") {
        int res = 0;

        pro::promise<int> p(returnInt, 115);
        pro::promise<int> p2([]()->int { throw 666; });
        std::vector<pro::promise<int>> v;

        v.push_back(std::move(p));
        v.push_back(std::move(p2));

        pro::PromiseAll(v).then(
            [&res](auto vec) {
                res = std::reduce(vec.begin(), vec.end());
            },
            [&res](auto vec) {
                res = std::reduce(vec.begin(), vec.end());
            }
        );
        
        CHECK(res > 0);
        REQUIRE(res == 666);
    }

    SECTION("Some void promises are rejected") {
        int res = 0;

        std::vector<pro::promise<void>> v;
        v.emplace_back(pro::make_promise<void>(dummy));
        v.emplace_back(pro::make_promise<void>([] { throw 115; }));

        pro::PromiseAll(v).then(
            [&res]() { res = 1; },
            [&res]() { res = 2; }
        );

        CHECK(res > 0);
        REQUIRE(res == 2);
    }

    SECTION("PromiseAll itself is asynchronous") {
        auto start = std::chrono::system_clock::now();

        //test should take no logner than 20ms, 
        //despite promise sleeping for 115ms 

        pro::promise<int> p1(returnInt, 1);
        pro::promise<int> p2(sleepAndReturnInt, 115, 115);
        pro::promise<int> p3([]()->int { throw 666; });

        std::vector<pro::promise<int>> v;
        v.push_back(std::move(p1));
        v.push_back(std::move(p2));
        v.push_back(std::move(p2));

        auto p = pro::PromiseAll(v).then(dummyHandler);

        auto end = std::chrono::system_clock::now();
        REQUIRE(20 > std::chrono::duration_cast <std::chrono::milliseconds> (end - start).count());
    }

    SECTION("Sync PromiseAll resolve all its methods asynchronously") {
        auto start = std::chrono::system_clock::now();

        //test should take no logner than 20ms, 
        //despite promise sleeping for 115ms 

        pro::promise<int> p1(sleepAndReturnInt, 666, 115);
        pro::promise<int> p2(sleepAndReturnInt, 666, 420);
        
        std::vector<pro::promise<int>> v;
        v.push_back(std::move(p1));
        v.push_back(std::move(p2));

        pro::PromiseAll(v).then(dummyHandler);

        auto end = std::chrono::system_clock::now();
        auto i = std::chrono::duration_cast <std::chrono::milliseconds> (end - start).count();
        REQUIRE(1000 > std::chrono::duration_cast <std::chrono::milliseconds> (end - start).count());
    }

    SECTION("Two promise fails and the result comes from the fastest one") {
        int res = 0;

        pro::promise<long> p1([] { return 1L; });
        pro::promise<long> p2([]()->long {
            std::this_thread::sleep_for(std::chrono::milliseconds(115));
            throw 420L; });
        pro::promise<long> p3([]()->long {        
            throw std::exception("");
        });

        std::vector<pro::promise<long>> v;
        v.push_back(std::move(p1));
        v.push_back(std::move(p2));
        v.push_back(std::move(p3));

        pro::PromiseAll(v).fail(         
            [&res](std::exception_ptr eptr) {
                try {
                    std::rethrow_exception(eptr);
                }
                catch (std::exception) {
                    res = 1;
                }
                catch (...) {
                    res = -1;
                }
            });

        CHECK(res > 0);
        REQUIRE(res == 1);
    }

    SECTION("Empty PromiseAll are resolved") {
        int res = 0;

        std::vector<pro::promise<int>> v;

        pro::PromiseAll(v).then(
            [&res](auto vec) {
                res = std::reduce(vec.begin(), vec.end());
            }
        ).fail(
            [&res](auto eptr) {
                res = 115;
            });

        REQUIRE(res == 0);
    }
}

TEST_CASE("PromiseRace", "[util]")
{
    SECTION("PromiseRace resolving the fastest argument") {
        int res = 0;

        std::vector<pro::promise<int>> v;
        v.emplace_back(pro::make_promise<int>(sleepAndReturnInt, 115, 115));
        v.emplace_back(pro::make_promise<int>(returnInt, 666));

        pro::PromiseRace(v).then([&res](int i) { res = i; });

        CHECK(res > 0);
        REQUIRE(res == 666);
    }

    SECTION("PromiseRace resolving the fastest rejecting argument") {
        int res = 0;

        std::vector<pro::promise<int>> v;
        v.emplace_back(pro::make_promise<int>(sleepAndReturnInt, 115, 115));
        v.emplace_back(pro::make_promise<int>([]()->int { throw 666; }));

        pro::PromiseRace(v).then(
            [&res](int i) { res = i; },
            [&res](int i) { res = i; }
        );

        CHECK(res > 0);
        REQUIRE(res == 666);
    }
}

TEST_CASE("PromiseAny", "[util]")
{
    SECTION("PromiseAny resolving the fastest argument") {
        int res = 0;

        std::vector<pro::promise<int>> v;
        v.emplace_back(pro::make_promise<int>(sleepAndReturnInt, 115, 115));
        v.emplace_back(pro::make_promise<int>(returnInt, 666));

        pro::PromiseAny(v).then([&res](int i) { res = i; });

        CHECK(res > 0);
        REQUIRE(res == 666);
    }

    SECTION("Some promises are rejected") {
        int res = 0;

        pro::promise<int> p(returnInt, 115);
        pro::promise<int> p2([]()->int { throw 666; });
        std::vector<pro::promise<int>> v;

        v.push_back(std::move(p));
        v.push_back(std::move(p2));

        pro::PromiseAny(v).then(
            [&res](int i) { res = i; },
            [&res](int i) { res = -1; }
        );

        CHECK(res > 0);
        REQUIRE(res == 115);
    }
    
    SECTION("PromiseAny fails when all promises fail") {
        int res = 0;

        std::vector<pro::promise<int>> v;
        v.emplace_back(pro::make_promise<int>([]()->int { throw 115; }));
        v.emplace_back(pro::make_promise<int>([]()->int { throw 666; }));

        pro::PromiseAny(v).then(
            [&res](int i) { res = -1; },
            [&res](int i) { res = -2; },
            [&res](std::exception_ptr eptr) {
                try {
                    std::rethrow_exception(eptr);
                }
                catch (std::vector<int> &vec) {
                    res = std::reduce(vec.begin(), vec.end());
                }
                catch (...) {
                    res = -3;
                }
            }
        );

        CHECK(res > 0);
        REQUIRE(res == 115 + 666);
    }

    SECTION("PromiseAny fails with an aggregate exception, with the right order") {
        int res = 0;

        std::vector<pro::promise<int>> v;
        v.emplace_back(pro::make_promise<int>([]()->int { throw 115; }));
        v.emplace_back(pro::make_promise<int>([]()->int { 
            std::this_thread::sleep_for(std::chrono::milliseconds(115));
            throw std::exception(""); }));
        v.emplace_back(pro::make_promise<int>([]()->int { throw 666; }));

        pro::PromiseAny(v).fail(
            [&res](std::exception_ptr eptr) {
                try {
                    std::rethrow_exception(eptr);
                }
                catch (std::vector<int>&) {
                    res = -2;
                }
                catch (pro::AggregateException & aggr) {
                    CHECK(aggr.inner_exceptions.size() == 3);
                    REQUIRE_THROWS_AS(std::rethrow_exception(aggr.inner_exceptions[0]), int);
                    REQUIRE_THROWS_AS(std::rethrow_exception(aggr.inner_exceptions[1]), std::exception);
                    REQUIRE_THROWS_AS(std::rethrow_exception(aggr.inner_exceptions[2]), int);
                }
                catch (...) {
                    res = -3;
                }
            }
        );

        REQUIRE(res == 0);
    }

    SECTION("PromiseAny fails when collection is empty") {
        int res = -1;

        std::vector<pro::promise<int>> v;
        pro::PromiseAny(v).fail(
            [&res](std::exception_ptr eptr) {
            try {
                std::rethrow_exception(eptr);
            }
            catch (std::vector<int>& vec) {
                res = std::reduce(vec.begin(), vec.end());
            }
            catch (...) {
                res = -3;
            }
        });

        REQUIRE(res == -3);
    }
}

TEST_CASE("Promise copy/move behaviour", "[basic]")
{
    SECTION("Promise moves its values") {
        pro::promise<CopyCounter> p(returnCounter);

        p.then([&](const CopyCounter &cres) {
            CHECK(cres.constructed == 1);
            CHECK(cres.moved > 0);
            REQUIRE(cres.copied == 0);
        });
    }

    SECTION("PromiseAll/tuple copies its values only once, from the state") {
        int copied_res = 0;

        pro::PromiseAll(
            pro::make_promise<CopyCounter>(returnCounter),
            pro::make_promise<CopyCounter>(returnCounter)
        ).then([&copied_res](auto tuple) {
            copied_res = std::get<0>(tuple).copied + (int)std::get<1>(tuple).copied;
        });
        
        REQUIRE(copied_res == 2);
    }

    SECTION("PromiseAll/collection move its values") {
        int copied_res = 0;
        std::vector<pro::promise<CopyCounter>> v;
        v.emplace_back(pro::make_promise<CopyCounter>(returnCounter));
        v.emplace_back(pro::make_promise<CopyCounter>(returnCounter));

        pro::PromiseAll(v).then([&copied_res](auto vec) {
            for (const auto& c : vec) {
                copied_res += c.copied;
            }          
        });

        REQUIRE(copied_res == 0);
    }
}