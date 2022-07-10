#pragma once
#include <array>
#include <deque>
#include <functional>
#include <future>
#include <mutex>
#include <thread>
#include <utility>
#include <chrono>
#include <iostream>

using namespace std::chrono_literals;

// In computer programming, a thread pool is a software design pattern for
// achieving concurrency of execution in a computer program.Often also called a
// replicated workers or worker - crew model, [1] a thread pool maintains
// multiple threads waiting for tasks to be allocated for concurrent execution
// by the supervising program.By maintaining a pool of threads, the model
// increases performanceand avoids latency in execution due to frequent
// creationand destruction of threads for short - lived tasks.[2] The number of
// available threads is tuned to the computing resources available to the
// program, such as a parallel task queue after completion of execution

using namespace std::chrono_literals;

namespace kr
{
template <size_t s> class Threadpool
{
  public:
    Threadpool()
    {
        // note how we have to pass the object as an argument
        if (s > 0)
        {
            _monitor = std::thread(&Threadpool::process, this);
        }
    }

    Threadpool(Threadpool& other) = delete;

    Threadpool& operator=(Threadpool& other) = delete;

    //can only work for similarly sized arrays
    Threadpool(Threadpool&& other)
    {
        if (other == this)
        {
            return;
        }
        other._lock.lock();
        if(_monitor.joinable())
            _monitor.join();
        std::thread _monitor = other._monitor;

        _threads = std::move(other._threads);
        _tasks = std::move(other._tasks);
        _kill = other._kill;
        _check = other._check;
        other.unlock();
    }

    Threadpool& operator=(Threadpool&& other)
    {
        if (other == this)
        {
            return *this;
        }
        other._lock.lock();
        if (_monitor.joinable())
            _monitor.join();
        std::thread _monitor = other._monitor;

        _threads = std::move(other._threads);
        _tasks = std::move(other._tasks);
        _kill = other._kill;
        _check = other._check;
        other.unlock();
        return this;
    }

    void process()
    {
        std::unique_lock<std::mutex> ul(_lock);
        auto empty = _tasks.empty();
        auto kill = _kill;
        ul.unlock();

        while (!kill || !empty)
        {
            ul.lock();
            _signal.wait(ul, [&]() { return _check == true; }); // waitfor if u wanna use a timeout

            empty = _tasks.empty();
            kill = _kill;
            ul.unlock();
            if (!empty)
            {
                bool assigned = false;

                ul.lock();
                for (auto &t : _threads)
                {
                    if (!t.first.joinable())
                    {
                        auto f = _tasks.front(); // todo can I do a ref copy here?
                        auto &p = t.second;
                        _tasks.pop_front();
                        const auto encapsulate = [&p, f]() {
                            f();
                            p = true;
                        };

                        t.second = false;
                        t.first = std::thread(encapsulate);

                        std::cout << "Assigned: " << t.first.get_id() << std::endl;
                        assigned = true;
                        if (empty)
                        {
                            _check = false;
                        }
                        break;
                    }
                }
                ul.unlock();
                if (!assigned)
                {
                    ul.lock();
                    for (auto &t : _threads)
                    {
                        if (t.first.joinable() && t.second)
                        {
                            std::cout << "Joining due to normal execution completion: " << t.first.get_id()
                                      << std::endl;
                            t.first.join();
                        }
                    }
                    ul.unlock();
                }
            }
        }
    }

    void addTask(const std::function<void(void)> &t)
    {
        std::lock_guard<std::mutex> lock{_lock};
        _tasks.push_back(t);
        _check = true;
        _signal.notify_all();
    }

    ~Threadpool()
    {
        if (s == 0)
            return;
        _lock.lock();
        _kill = true;
        _lock.unlock();

        while (true)
        {
            _lock.lock();
            if (_tasks.empty())
            {
                _lock.unlock();
                break;
            }
            _lock.unlock();
            std::this_thread::sleep_for(0ms);
        }

        _lock.lock();
        for (auto &t : _threads)
        {
            if (t.first.joinable())
            {
                std::cout << "Joining due to destructor: " << t.first.get_id() << std::endl;
                t.first.join();
            }
        }
        _lock.unlock();

        if (_monitor.joinable())
        {
            std::cout << "Joining Monitor." << std::endl;
            _monitor.join();
        }
    }

  private:
    std::thread _monitor;
    std::array<std::pair<std::thread, std::atomic<bool>>, s> _threads;
    std::deque<std::function<void(void)>> _tasks;
    bool _kill = false;
    bool _check = false;

    std::mutex _lock;
    std::condition_variable _signal;
};
} // namespace kr
