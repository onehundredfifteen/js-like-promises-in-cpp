// main.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#include "async.h"

void foo(int waitfor) {
    std::this_thread::sleep_for(std::chrono::milliseconds(waitfor));
    std::cout << "method foo waited in thread = " << std::this_thread::get_id() << " for " << waitfor << " ms." << std::endl;
}

void bar() {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::cout << "method bar executed in thread = " << std::this_thread::get_id() << std::endl;
}

void resolveInfo() {
    std::cout << "promise p3 was resolved" << std::endl;
}
void rejectInfo() {
    std::cout << "promise p3 was rejected" << std::endl;
}

int main()
{
    std::cout << "main thread ID: " << std::this_thread::get_id() << std::endl;
    std::chrono::system_clock::time_point start = std::chrono::system_clock::now();

    //First promise (resolving) with lambda function
    crows::Promise<int>p1([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(2500));
        std::cout << "method in p1 ready" << std::endl;
        return 115; 
    });

    //Second promise (rejecting) with lambda function
    crows::Promise<int>p2([]() -> int {
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        std::cout << "method in p2 ready" << std::endl;
        throw 666; //reject promise by throwing a value
    });

    //Third promise resolving method with an argument
    crows::Promise<void, int> p3(foo, 777);

    //Fourth promise (resolving) bar method, there is no waiting handler
    crows::Promise<void>p4(bar);

    //wait for promise p1, handle resolve
    auto t = p1.then(
        [](int i) { std::cout << "promise 1 resolved, then result = " << i << std::endl; }
    );

    //wait for promise p2, handle both resolve and reject
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

