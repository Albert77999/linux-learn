#pragma once

#include <iostream>
#include <string>
#include <pthread.h>

namespace LockModule
{
    class Mutex
    {
    public:
        Mutex(const Mutex &mutex) = delete;
        Mutex &operator=(const Mutex &mutex) = delete; // const会自动delete
        Mutex(Mutex &&) = delete;
        Mutex &operator=(Mutex &&) = delete;
        Mutex()
        {
            int n = pthread_mutex_init(&_mutex, nullptr);
            (void)n;
        }
        void Lock()
        {
            int n = pthread_mutex_lock(&_mutex);
            (void)n;
        }
        void Unlock()
        {
            int n = pthread_mutex_unlock(&_mutex);
            (void)n;
        }
        pthread_mutex_t *GetMutex()
        {
            return &_mutex;
        }
        ~Mutex()
        {
            int n = pthread_mutex_destroy(&_mutex);
            (void)n;
        }

    private:
        pthread_mutex_t _mutex;
    };
    // RAII
    class LockGuard
    {
    public:
        LockGuard(const LockGuard &) = delete;
        LockGuard &operator=(const LockGuard &) = delete;
        LockGuard(LockGuard &&) = delete;
        LockGuard &operator=(LockGuard &&) = delete;
        LockGuard(Mutex &mutex) : _mutex(mutex)
        {
            _mutex.Lock();
        }
        ~LockGuard()
        {
            _mutex.Unlock();
        }

    private:
        Mutex &_mutex;
    };
}
