#pragma once

#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

class ToyThreadPool
{
    static const size_t max_threads = 128;

private:
    std::queue<std::function<void(void)>> _tasks;
    std::mutex _mtx;
    std::atomic_bool _active = true;
    std::condition_variable _flag;
    size_t _max_thread_num = 8;
    size_t _thread_num = 0;
    size_t _task_num = 0;

private:
    void run()
    {
        while (true)
        {
            std::function<void(void)> task;
            std::unique_lock<std::mutex> lock(_mtx);
            if (_active && _tasks.empty())
            {
                _flag.wait(lock);
            }

            if (_active)
            {
                task = _tasks.front();
                _tasks.pop();
                lock.unlock();
            }
            else
            {
                --_thread_num;
                return;
            }

            try
            {
                if (task)
                {
                    task();
                }
            }
            catch (const std::exception e)
            {
                std::cerr << "Failed to execute " << typeid(decltype(task)).name() << ":" << e.what() << std::endl;
            }

            lock.lock();
            --_task_num;
            lock.unlock();
        }
    }

    void add_thread()
    {
        ++_thread_num;
        std::thread(&ToyThreadPool::run, this).detach();
    }

public:
    ToyThreadPool()
    {
    }

    explicit ToyThreadPool(const size_t threads)
        : _max_thread_num(std::min(max_threads, threads))
    {
    }

    ToyThreadPool(ToyThreadPool &) = delete;
    ToyThreadPool &operator=(ToyThreadPool &) = delete;

    ToyThreadPool(ToyThreadPool &&) = delete;
    ToyThreadPool &operator=(ToyThreadPool &&) = delete;

    ~ToyThreadPool()
    {
        std::unique_lock<std::mutex> lock(_mtx);
        _active = false;
        lock.unlock();
        _flag.notify_all();
        while (true)
        {
            lock.lock();
            if (_thread_num == 0)
            {
                break;
            }
            _flag.notify_all();
            lock.unlock();
        }
    }

    template <typename Func, typename... Args>
    void add_task(Func &&fn, Args &&...args)
    {
        std::unique_lock<std::mutex> lock(_mtx);
        ++_task_num;
        std::function<void(void)> task = std::bind(std::forward<Func>(fn), std::forward<Args>(args)...);
        _tasks.push(task);
        if (_thread_num < _task_num && _thread_num < _max_thread_num)
        {
            add_thread();
        }
        _flag.notify_one();
    }

    void join()
    {
        while (true)
        {
            std::unique_lock<std::mutex> lock(_mtx);
            if (_task_num == 0)
            {
                break;
            }
        }
    }

    void stop()
    {
        std::unique_lock<std::mutex> lock(_mtx);
        _active = false;
        _flag.notify_all();
        while (!_tasks.empty())
        {
            _tasks.pop();
        }
        lock.unlock();
        while (true)
        {
            lock.lock();
            if (_thread_num == 0)
            {
                break;
            }
            _flag.notify_all();
            lock.unlock();
        }
        _active = true;
    }
};