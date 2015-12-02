#ifndef __THREAD_UTILS_H__
#define __THREAD_UTILS_H__

#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <deque>
#include <string>

using namespace std;

int Open(const string& pathname, int flags);
int Open(const string& pathname, int flags, mode_t mode);
int Close(int fd);
off_t Lseek(int fd, off_t offset, int whence);
ssize_t Read(int fd, void *buf, size_t count);
ssize_t Write(int fd, const void *buf, size_t count);
int Unlink(const string& pathname);

class CondVar;

class Mutex {
    public:
        Mutex();
        ~Mutex();

        void Lock();
        void Unlock();
        void AssertHeld() { }

    private:
        friend class CondVar;
        pthread_mutex_t mu_;

        // No copying
        Mutex(const Mutex&);
        void operator=(const Mutex&);
};

class CondVar {
    public:
        explicit CondVar(Mutex* mu);
        ~CondVar();
        void Wait();
        void Signal();
        void SignalAll();
    private:
        pthread_cond_t cv_;
        Mutex* mu_;
};

class MutexLock {		//scoped lock
    public:
        explicit MutexLock(Mutex *mu)
            : mu_(mu)  {
                this->mu_->Lock();
            }
        ~MutexLock() { this->mu_->Unlock(); }

    private:
        Mutex *const mu_;
        // No copying allowed
        MutexLock(const MutexLock&);
        void operator=(const MutexLock&);
};


class Task {
    public: 
        virtual ~Task(){}
        virtual void Process() = 0;
};

class ThreadPool{
    public:
        ThreadPool(int nthreads);
        ~ThreadPool();
        void AddTask(Task* tp);
        void Run();
        void Stop();

    private:
        static void* thread_func(void* arg);
        int nthreads_;
        pthread_t* tids_;
        CondVar has_tasks_;
        Mutex m_;
        std::deque<Task*> task_queue_;
        bool stop_;
};

typedef void* (*ThreadFunc)(void* arg);

pthread_t StartThread(ThreadFunc func, void* arg);
void ThreadJoin(pthread_t t);

#endif	/* __THREAD_UTILS_H__ */
