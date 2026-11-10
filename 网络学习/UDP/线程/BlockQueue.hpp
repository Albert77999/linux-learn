	#pragma once
    #include "Mutex.hpp"
    #include "Cond.hpp"

    #include <unistd.h>
    #include <stdlib.h>
    #include <sys/types.h>
    #include <iostream>
    #include <queue>
    
    using namespace LockModule;
    using namespace CondModule;

    // version 2
    static const int gcap = 10;

    template <typename T>
    class BlockQueue
    {
    private:
        bool IsFull() { return _q.size() == _cap; }
        bool IsEmpty() { return _q.empty(); }

    public:
        BlockQueue(int cap = gcap) : _cap(cap), _cwait_num(0), _pwait_num(0)
        {
        }
        void Equeue(const T &in) // 生产者
        {
            LockGuard lockguard(_mutex);
            // 你想放数据，就能放数据吗？？生产数据是有条件的！
            // 结论1: 在临界区中等待是必然的(目前)
            while (IsFull()) // 5. 对条件进行判断，为了防止伪唤醒，我们通常使用while进行判断！
            {//第二个理由则是一个线程被唤醒，另一个线程刚好调用这个函数，形成竞争锁
                std::cout << "生产者进入等待..." << std::endl;

                // 2. 等是，释放_mutex
                _pwait_num++;
                _productor_cond.Wait(_mutex); // wait的时候，必定是持有锁的！！是有问题的！
                _pwait_num--;
                // 3. 返回，线程被唤醒&&重新申请并持有锁(它会在临界区内醒来！)

                std::cout << "生产者被唤醒..." << std::endl;
            }
            // 4. if(IsFull())不满足 || 线程被唤醒
            _q.push(in); // 生产

            // 肯定有数据！
            if (_cwait_num)
            {
                std::cout << "叫醒消费者" << std::endl;
                _consumer_cond.Notify();
            }
        }
        void Pop(T *out) // 消费者
        {
            LockGuard lockguard(_mutex);
            while (IsEmpty())
            {
                std::cout << "消费者进入等待..." << std::endl;
                _cwait_num++;
                _consumer_cond.Wait(_mutex); // 伪唤醒
                //这个函数有两个过程，一个是等待，另一个是抢锁
                _cwait_num--;
                std::cout << "消费者被唤醒..." << std::endl;
            }
            // 4. if(IsEmpty())不满足 || 线程被唤醒
            *out = _q.front();
            _q.pop();

            // 肯定有空间
            if (_pwait_num)
            {
                std::cout << "叫醒生产者" << std::endl;
                _productor_cond.Notify();
            }
        }
        ~BlockQueue()
        {
        }

    private:
        std::queue<T> _q;               // 保存数据的容器，临界资源
        int _cap;                       // bq最大容量
        Mutex _mutex;         // 互斥
        Cond _productor_cond; // 生产者条件变量
        Cond _consumer_cond;  // 消费者条件变量

        int _cwait_num;
        int _pwait_num;
    };
