#pragma once
#include <iostream>
#include <vector>
#include <queue>
#include <functional>
#include <memory>
#include <utility>

#include "Thread.hpp"
#include "Mutex.hpp"
#include "Cond.hpp"
#include "Log.hpp"

namespace ThreadPoolModule
{
    using namespace LockModule;
    using namespace ThreadModule;
    using namespace CondModule;

    using thread_t = std::shared_ptr<Thread>;

    const int defaultnum = 4;

    template <class T>
    class ThreadPool
    {
    private:
        bool IsEmpty() { return _task_queue.empty(); }
        void HandlerTask(std::string name)
        {
            LOG(LogLevel::INFO) << "线程-" << name << "进入HandlerTask";
            while (true)
            {
                T task;
                {
                    LockGuard lockguard(_mutex);
                    while (IsEmpty() && _is_running)
                    {
                        _wait_num++;
                        _cond.Wait(_mutex);
                        _wait_num--;
                    }
                    if (IsEmpty() && !_is_running)
                    {
                        LOG(LogLevel::INFO) << "线程-" << name << "准备退出";
                        break;
                    }
                    task = _task_queue.front();
                    _task_queue.pop();
                }
                task(name); // 任务处理
            }
            LOG(LogLevel::INFO) << "线程-" << name << "退出HandlerTask";
        }

    public:
        ThreadPool(int thread_num = defaultnum)
            : _is_running(false), _thread_num(thread_num), _wait_num(0)
        {
            for (int i = 0; i < _thread_num; i++)
            {
                _threads.push_back(std::make_shared<Thread>(std::bind(&ThreadPool::HandlerTask, this, std::placeholders::_1)));
            }
        }
        void Equeue(T &&in)
        {
            LockGuard lockguard(_mutex);
            if(!_is_running)
                return;
            _task_queue.push(std::move(in));
            _cond.Notify();
        }
        void Start()
        {
            if (_is_running)
                return;
            _is_running = true;
            for (const thread_t &th : _threads)
            {
                th->Start();
                LOG(LogLevel::INFO) << "线程池中的线程" << th->GetName() << "已经启动";
            }
        }
        void Stop()
        {
            LockGuard lockguard(_mutex);
            _is_running = false;
            _cond.NotifyAll();
            LOG(LogLevel::INFO) << "线程池停止接收新任务，准备退出";
        }
        void Wait()
        {
            for(const thread_t &th : _threads)
            {
                th->Join();
                LOG(LogLevel::INFO) << "线程池中的线程" << th->GetName() << "回收成功";
            }
        }
        ~ThreadPool()
        {
        }

    private:
        std::vector<thread_t> _threads;
        std::queue<T> _task_queue;
        Mutex _mutex;
        Cond _cond;
        bool _is_running;
        int _thread_num;
        int _wait_num;
    };
}
