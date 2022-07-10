#include <thread>
#include <functional>
#include <iostream>
#include <chrono>
#include <vector>
#include <algorithm>

#include "threadpool.h"

using namespace std::chrono_literals;

int f3(int i)
{
    std::this_thread::sleep_for( 20ms);
    std::cout << "Hi there mister! I am:" << std::this_thread::get_id() << std::endl;
    return i;
}

int main()
{
    kr::Threadpool<32> threadPool;
    std::this_thread::sleep_for(5ms);
    for (int i = 0 ; i < 2000; ++i)
    {
        threadPool.addTask(std::bind(f3, i));
        std::this_thread::yield();
    }

    kr::Threadpool<64> threadPool2{ kr::Threadpool<64>()};
    for (int i = 0; i < 2000; ++i)
    {
        threadPool2.addTask(std::bind(f3, i));
        std::this_thread::yield();
    }

    kr::Threadpool<4> threadPool3 = kr::Threadpool<4>();
    for (int i = 0; i < 2000; ++i)
    {
        threadPool3.addTask(std::bind(f3, i));
        std::this_thread::yield();
    }

    kr::Threadpool<4> threadPool4;
    std::this_thread::sleep_for(5ms);
    for (int i = 0; i < 2000; ++i)
    {
        threadPool4.addTask(std::bind(f3, i));
        std::this_thread::yield();
    }

    kr::Threadpool<0> threadPool5;
    threadPool5.addTask(std::bind(f3,0));


    std::this_thread::sleep_for(500ms);

    return 0;
}