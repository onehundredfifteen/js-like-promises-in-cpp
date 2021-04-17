// main.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#include "async.h"

void foo(int waitfor) {
    std::this_thread::sleep_for(std::chrono::milliseconds(waitfor));
    std::cout << "method foo waited in thread = " << std::this_thread::get_id() << " for " << waitfor << " ms." << std::endl;
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

    //Third promise (resolving) there is no waiting handler
    crows::Promise<void, int> p3(foo, 777);

    auto t = p1.then([](int i) { std::cout << "promise 1 resolved, then result = " << i << std::endl; });

    auto tt = p2.then(
        [](int i) { std::cout << "promise 2 was resolved, then result = " << i << std::endl; },
        [](int i) { std::cout << "promise 2 was rejected, then result = " << i << std::endl; }
    );

    auto end = std::chrono::system_clock::now();
    auto diff = std::chrono::duration_cast <std::chrono::milliseconds> (end - start).count();
    std::cout << "Total Time Taken = " << diff << " ms" << std::endl;
}

