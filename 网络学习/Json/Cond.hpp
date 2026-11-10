#pragma once

#include <pthread.h>
#include <iostream>
#include "Mutex.hpp"

using namespace LockModule;

namespace CondModule
{
    class Cond
    {
    public:
        Cond()
        {
            pthread_cond_init(&_cond, nullptr);
        }
        void Wait(Mutex &mutex)
        {
            pthread_cond_wait(&_cond, mutex.GetMutex());
        }
        void Notify()
        {
            pthread_cond_signal(&_cond);
        }
        void NotifyAll()
        {
            pthread_cond_broadcast(&_cond);
        }
        ~Cond()
        {
            pthread_cond_destroy(&_cond);
        }

    private:
        pthread_cond_t _cond;
    };
}
