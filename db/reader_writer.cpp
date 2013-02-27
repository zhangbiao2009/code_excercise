#include <cstdlib>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include <string>

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

void gen_random(char *s, const int len) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    for (int i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    s[len] = 0;
}

static char data[10] = {'\0'};

//use writers-preference algorithm to handle concurrent readers and writers
Mutex m;
CondVar no_writer(&m);
CondVar no_reader(&m);
int nreaders=0;
int nwriters=0;
int nwaited_readers = 0;
int nwaited_writers = 0;

#define MAX_NTHREADS 100
Mutex cm[MAX_NTHREADS];	//mutex for control
CondVar* cv[MAX_NTHREADS];	//convar for control
volatile int tid=0;		//tid++ when a new waited thread created

void init_cond_vars()
{
	for(int i=0; i<MAX_NTHREADS; i++)
		cv[i] = new CondVar(&cm[i]);
}

void* reader(void* arg)
{
	MutexLock l(&m);
	while(nwriters>0){
		nwaited_readers++;
		fprintf(stderr, "nwaited_readers: %d\n", nwaited_readers);
		no_writer.Wait();	//wait until no active writers
		nwaited_readers--;
		fprintf(stderr, "nwaited_readers: %d\n", nwaited_readers);
	}


	nreaders++;
	fprintf(stderr, "n_readers: %d\n", nreaders);
	m.Unlock();		//unlock to allow concurrent reading
	//read data
	if(arg){	//wait until continue command sending
		int self = *(int*)arg;
		cv[self]->Wait();
	}
	if(arg) delete (int*)arg;
		
	fprintf(stderr, "read: %s\n", data);
	m.Lock();
	nreaders--;
	fprintf(stderr, "n_readers: %d\n", nreaders);
	if(nreaders == 0)
		no_reader.SignalAll();

	return (void*)0;
}

void* writer(void* arg)
{
	MutexLock l(&m);
	nwriters++;		//note: first increment, then wait
	fprintf(stderr, "nwriters: %d\n", nwriters);
	while(nreaders > 0){
		nwaited_writers++;
		fprintf(stderr, "nwaited_writers: %d\n", nwaited_writers);
		no_reader.Wait();
		nwaited_writers--;
		fprintf(stderr, "nwaited_writers: %d\n", nwaited_writers);
	}

	//write
	if(arg){
		int self = *(int*)arg;
		cv[self]->Wait();
	}
	if(arg) delete (int*)arg;
		
	char buf[10];
	gen_random(buf, 6);
	strcpy(data, buf);
	fprintf(stderr, "writer data: %s\n", data);
	nwriters--;
	fprintf(stderr, "nwriters: %d\n", nwriters);
	if(nwriters == 0)
		no_writer.SignalAll();

	return (void*)0;
}

typedef void* (*ThreadFunc)(void* arg);

void StartThread(ThreadFunc func, void* arg) {
  pthread_t t;
  int* p = NULL;
  if(arg){
	  tid++;
	  p = new int(tid);
	  fprintf(stderr, "start wait thread %d\n", tid);
  }
	  
  PthreadCall("start thread",
              pthread_create(&t, NULL, func, (void*)p));
}


int main()
{
	int n;
	char cmd[100], key[100];

	init_cond_vars();

	while(1){
		fprintf(stderr, "please input command:\n");
		scanf("%s", cmd);
		if(string(cmd) == "cont"){	//cont <tid>, thread tid continue running
			scanf("%d", &n);
			cv[n]->Signal();
		}else if(string(cmd) == "start"){ //start <n> <reader|writer>
			scanf("%d", &n);
			scanf("%s", key);
			ThreadFunc func = string(key) == "reader"? reader : writer;
			while(n--)
				StartThread(func, NULL);
		}else if(string(cmd) == "wstart"){ //wstart <n> <reader|writer>, start waited readers or writers
			scanf("%d", &n);
			scanf("%s", key);
			ThreadFunc func = string(key) == "reader"? reader : writer;
			while(n--)
				StartThread(func, (void*)"wait");
		}
		else{
			return 0;
		}
	}

	return 0;
}
