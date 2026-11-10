#pragma once
#include <iostream>
#include <string>
#include <functional>
#include <utility>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

namespace ThreadModule
{
    using func_t = std::function<void(std::string name)>;
    static int num = 1;
    enum class TSTATUS{
        NEW,
        START,
        STOP,
        JOIN
    };
    class Thread
    {
    private:
        static void *Routine(void *args)
        {
            Thread *td = static_cast<Thread*>(args);
            td->_func(td->_name);
            return nullptr;
        }
    public:
        explicit Thread(func_t func)
            :_pid(getpid())
            ,_status(TSTATUS::NEW)
            ,_func(std::move(func))
            ,_joinable(true)
        {
            _name = std::string("Thread-") + std::to_string(num++);
        }
        bool Start()
        {
            if(_status == TSTATUS::NEW)
            {
                if(pthread_create(&_tid,nullptr,Routine,this) != 0)
                {
                    return false;
                }
                _status = TSTATUS::START;
                return true;
            }
            return false;
        }
        bool Stop()
        {
            if(_status == TSTATUS::START)
            {
                _status = TSTATUS::STOP;
                if(pthread_cancel(_tid) != 0)
                {
                    return false;
                }
                return true;
            }
            return false;
        }
        bool Join()
        {
            if(_joinable && _status == TSTATUS::START)
            {
                if(pthread_join(_tid,nullptr) != 0)
                {
                    return false;
                }
                _status = TSTATUS::JOIN;
                return true;
            }
            return false;
        }
        bool Detach()
        {
            EnableDetach();
            if(pthread_detach(_tid) != 0)
                return false;
            return true;
        }
        void EnableDetach()
        {
            _joinable = false;
        }
        std::string GetName()
        {
            return _name;
        }
        ~Thread()
        {

        }
    private:
        pthread_t _tid;
        pid_t _pid;
        std::string _name;
        TSTATUS _status;
        func_t _func;
        bool _joinable;
    };
}
