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
        const ThreadPool<T> &operator=(const ThreadPool<T>&) = delete;
        ThreadPool(const ThreadPool<T>&) = delete;
        ThreadPool(int thread_num = defaultnum)
            : _is_running(false), _thread_num(thread_num), _wait_num(0)
        {
            LOG(LogLevel::INFO) << "单例首次被加载执行";
            for (int i = 0; i < _thread_num; i++)
            {
                _threads.push_back(std::make_shared<Thread>(std::bind(&ThreadPool::HandlerTask, this, std::placeholders::_1)));
            }
        }
        //存在用户声明的拷贝操作和析构函数后，编译器不会自动生成移动操作
    public:
        static ThreadPool<T> *getInstance()
        //c++11保证这种写法线程安全
        {
            static ThreadPool<T> instance;
            return &instance;
        }
        void Equeue(T &&in)
        {
            LockGuard lockguard(_mutex);
            if(!_is_running)
                return;
            _task_queue.push(std::move(in));
            _cond.Notify();
        }
        bool Start()
        {
            if (_is_running)
                return true;
            _is_running = true;
            std::size_t started_num = 0;
            for (const thread_t &th : _threads)
            {
                if (th->Start())
                {
                    ++started_num;
                    LOG(LogLevel::INFO) << "线程池中的线程" << th->GetName() << "已经启动";
                }
                else
                {
                    LOG(LogLevel::ERROR) << "线程池中的线程" << th->GetName() << "启动失败";
                    Stop();
                    for (std::size_t i = 0; i < started_num; ++i)
                    {
                        if (_threads[i]->Join())
                        {
                            LOG(LogLevel::INFO) << "线程池中的线程" << _threads[i]->GetName() << "回收成功";
                        }
                        else
                        {
                            LOG(LogLevel::ERROR) << "线程池中的线程" << _threads[i]->GetName() << "回收失败";
                        }
                    }
                    return false;
                }
            }
            return true;
        }
        void Stop()//线程启动不完全处理
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
                if (th->Join())
                {
                    LOG(LogLevel::INFO) << "线程池中的线程" << th->GetName() << "回收成功";
                }
                else
                {
                    LOG(LogLevel::ERROR) << "线程池中的线程" << th->GetName() << "回收失败";
                }
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
