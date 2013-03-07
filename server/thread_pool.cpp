#include <cstdlib>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include <string>
#include <map>
#include <deque>
#include <iostream>


using namespace std;

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

typedef void* (*ThreadFunc)(void* arg);

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

static Mutex cache_m;
static map<string, string> cache;

class Task{
	public: 
		virtual ~Task(){}
		virtual void Process() = 0;
};

class GetTask : public Task{
	public:
		GetTask(const string& key):key_(key){}
		virtual ~GetTask(){}
		virtual void Process(){
			MutexLock l(&cache_m);
			cerr<<"Get ("<<key_<<", "<<cache[key_]<<")"<<endl;
		}
	private:
	string key_;
};

class SetTask : public Task{
	public:
		SetTask(const string& key, const string& val):key_(key),val_(val){}
		virtual ~SetTask(){}
		virtual void Process(){
			MutexLock l(&cache_m);
			cache[key_] = val_;
			cerr<<"Set ("<<key_<<", "<<cache[key_]<<")"<<endl;
		}
	private:
	string key_;
	string val_;
};

class ThreadPool{
	public:
		ThreadPool(int nthreads):nthreads_(nthreads), has_tasks_(&m_), stop_(false){
			tids_ = new pthread_t[nthreads_];
		}

		~ThreadPool(){
			delete[] tids_;
		}

		void AddTask(Task* tp){
			MutexLock l(&m_);
			task_queue_.push_back(tp);
			if(task_queue_.size() == 1)	//maybe someone is waiting for tasks
				has_tasks_.SignalAll();
		}

		void Run()
		{
			for(int i = 0; i<nthreads_; i++)
				tids_[i] = StartThread(ThreadPool::thread_func, this);
		}

		void Stop()
		{
			{
				MutexLock l(&m_);
				stop_ = true;
			}
			has_tasks_.SignalAll();
			for(int i = 0; i<nthreads_; i++)
				ThreadJoin(tids_[i]);
		}

	private:
		static void* thread_func(void* arg)
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

		int nthreads_;
		pthread_t* tids_;
		CondVar has_tasks_;
		Mutex m_;
		deque<Task*> task_queue_;
		bool stop_;
};

int main()
{
	int n;
	string cmd, key, val;

	ThreadPool thread_pool(5);
	thread_pool.Run();

	while(1){
		fprintf(stderr, "please input command:\n");
		cin>>cmd;
		if(cmd == "get"){
			cin>>key;
			GetTask* p = new GetTask(key);
			thread_pool.AddTask(p);
		}else if(cmd == "set"){ 
			cin>>key>>val;
			SetTask* p = new SetTask(key, val);
			thread_pool.AddTask(p);
		}else{  //exit
			thread_pool.Stop();
			return 0;
		}
	}

	return 0;
}
