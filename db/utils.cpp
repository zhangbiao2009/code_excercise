#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int Open(const string& pathname, int flags)
{
	int ret;
	if((ret = open(pathname.c_str(), flags)) < 0){
		perror("open");
		exit(1);
	}
	return ret;
}

int Open(const string& pathname, int flags, mode_t mode)
{
	int ret;
	if((ret = open(pathname.c_str(), flags, mode)) < 0){
		perror("open");
		exit(1);
	}
	return ret;
}

int Close(int fd)
{
	int ret;
	if((ret = close(fd)) < 0){
		perror("close");
		exit(1);
	}
	return ret;
}

off_t Lseek(int fd, off_t offset, int whence)
{
	off_t off;
	if((off = lseek(fd, offset, whence)) < 0){
		perror("lseek");
		exit(1);
	}
	return off;
}

ssize_t Read(int fd, void *buf, size_t count)
{
	ssize_t size;
	if((size = read(fd, buf, count)) < 0){
		perror("read");
		exit(1);
	}
	return size;
}

ssize_t Write(int fd, const void *buf, size_t count)
{
	ssize_t size;
	if((size = write(fd, buf, count)) < 0){
		perror("write");
		exit(1);
	}
	return size;
}

int Unlink(const string& pathname)
{
	int ret;
	if((ret=unlink(pathname.c_str()))<0){
		perror("unlink");
		exit(1);
	}
	return ret;
}

static void PthreadCall(const char* label, int result) {
	if (result != 0) {
		fprintf(stderr, "pthread %s: %s\n", label, strerror(result));
		abort();
	}
}

Mutex::Mutex() { PthreadCall("init mutex", pthread_mutex_init(&mu_, NULL)); }

Mutex::~Mutex() { PthreadCall("destroy mutex", pthread_mutex_destroy(&mu_)); }

void Mutex::Lock() { PthreadCall("lock", pthread_mutex_lock(&mu_)); }

void Mutex::Unlock() { PthreadCall("unlock", pthread_mutex_unlock(&mu_)); }

CondVar::CondVar(Mutex* mu)
	: mu_(mu) {
		PthreadCall("init cv", pthread_cond_init(&cv_, NULL));
	}

CondVar::~CondVar() { PthreadCall("destroy cv", pthread_cond_destroy(&cv_)); }

void CondVar::Wait() {
	PthreadCall("wait", pthread_cond_wait(&cv_, &mu_->mu_));
}

void CondVar::Signal() {
	PthreadCall("signal", pthread_cond_signal(&cv_));
}

void CondVar::SignalAll() {
	PthreadCall("broadcast", pthread_cond_broadcast(&cv_));
}

ThreadPool::ThreadPool(int nthreads):nthreads_(nthreads), has_tasks_(&m_), stop_(false){
	tids_ = new pthread_t[nthreads_];
}

ThreadPool::~ThreadPool(){
	delete[] tids_;
}

void ThreadPool::AddTask(Task* tp){
	MutexLock l(&m_);
	task_queue_.push_back(tp);
	if(task_queue_.size() == 1)	//maybe someone is waiting for tasks
		has_tasks_.SignalAll();
}

void ThreadPool::Run()
{
	for(int i = 0; i<nthreads_; i++)
		tids_[i] = StartThread(ThreadPool::thread_func, this);
}

void ThreadPool::Stop()
{
	{
		MutexLock l(&m_);
		stop_ = true;
	}
	has_tasks_.SignalAll();
	for(int i = 0; i<nthreads_; i++)
		ThreadJoin(tids_[i]);
}

void* ThreadPool::thread_func(void* arg)
{
	ThreadPool* tp = reinterpret_cast<ThreadPool*> (arg);
	MutexLock l(&tp->m_);
	while(!tp->stop_){
		while(tp->task_queue_.empty()){
			tp->has_tasks_.Wait();
			if(tp->stop_){
				goto end;
			}
		}
		Task* taskp = tp->task_queue_.front();
		tp->task_queue_.pop_front();
		tp->m_.Unlock();
		taskp->Process();
		delete taskp;
		tp->m_.Lock();
	}
end:
	return (void*) 0;
}

pthread_t StartThread(ThreadFunc func, void* arg) {
	pthread_t t;
	PthreadCall("start thread",
			pthread_create(&t, NULL, func, arg));
	return t;
}
void ThreadJoin(pthread_t t)
{
	PthreadCall("join thread",
			pthread_join(t, NULL));
}
