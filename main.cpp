// main.cpp : This file contains examples how to use promise.h
//

#include <iostream>

#include "include/promise.h"

void foo(int waitfor) {
    std::this_thread::sleep_for(std::chrono::milliseconds(waitfor));
    std::cout << "method foo waited in thread = " << std::this_thread::get_id() << " for " << waitfor << " ms." << std::endl;
}

void bar() {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::cout << "method bar executed in thread = " << std::this_thread::get_id() << std::endl;
}

void resolveInfo() {
    std::cout << "promise was resolved" << std::endl;
}
void rejectInfo() {
    std::cout << "promise was rejected" << std::endl;
}

int main()
{
    std::cout << "main thread ID: " << std::this_thread::get_id() << std::endl;
    std::chrono::system_clock::time_point start = std::chrono::system_clock::now();

    //First promise (resolving) with lambda function
    pro::Promise<int>p1([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(2500));
        std::cout << "method in p1 ready" << std::endl;
        return 115; //resolve this promise by returning a value
    });

    //Second promise (rejecting) with lambda function
    pro::Promise<int>p2([]() -> int {
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        std::cout << "method in p2 ready" << std::endl;
        throw 666; //reject this promise by throwing a value
    });

    //Third promise resolving  'foo' method with an argument
    pro::Promise<void, int> p3(foo, 777);

    //Fourth promise (resolving) 'bar method', there is no waiting handler
    pro::Promise<void>p4(bar);

    //wait for promise p1, handle resolve. Result (i) should be equal to 115
    auto t = p1.then(
        [](int i) { std::cout << "promise 1 resolved, then result = " << i << std::endl; }
    );

    //wait for promise p2, handle both resolve and reject
    //Promise p2 should be rejected with result equal to 666
    auto tt = p2.then(
        [](int i) { std::cout << "promise 2 was resolved, then result = " << i << std::endl; },
        [](int i) { std::cout << "promise 2 was rejected, then result = " << i << std::endl; }
    );

    //wait for promise p3, handle both resolve and reject and perform chaining
    auto tt3 = p3.then(
        resolveInfo,
        rejectInfo
    ).then(
        resolveInfo,
        rejectInfo
    );

    auto end = std::chrono::system_clock::now();
    auto diff = std::chrono::duration_cast <std::chrono::milliseconds> (end - start).count();
    std::cout << "Total Time Taken = " << diff << " ms" << std::endl;
}

