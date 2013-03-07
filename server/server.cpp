#include "anet.h"
#include <netinet/in.h>
#include <sys/epoll.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <string>
#include <map>
#include <deque>

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
int epfd = -1; 
#define MAX_CLIENTS 100
#define MAX_BUFLEN 10000
#define MAX_CMD_PARTS 3

enum State{CMD_CHECK, CMD_PARSE, GET_CMD, SET_CMD};
struct client{
	client():fd(-1),rpos(0),cmd_end(NULL),state(CMD_CHECK),wpos(0),wend(0){}
	void reset_for_next_read(){
		rpos = 0;
		cmd_end = NULL;
		state = CMD_CHECK;
	}
	void reset_for_next_write(){
		wpos = 0;
		wend = 0;
	}
	int fd;
	char readbuf[MAX_BUFLEN];
	int rpos;		//next pos which input need to store
	char* cmd_end;		//for command parsing, init as NULL
	char* cmd_part[MAX_CMD_PARTS];  //parsed command
	State state;
	char writebuf[MAX_BUFLEN];
	int wpos;		//next pos which need to output 
	int wend;		//the end of the data in write buf, equals to the array index of the last character + 1
};

struct epoll_event events[MAX_CLIENTS];
struct client clients[MAX_CLIENTS];
int nclients;

class Task{
	public: 
		virtual ~Task(){}
		virtual void Process() = 0;
};

void register_event_helper(client* c, uint32_t event)
{
	struct epoll_event ev; 
	c->reset_for_next_read();
	ev.data.fd=c->fd;  
	ev.events=event;  
	epoll_ctl(epfd, EPOLL_CTL_MOD, c->fd, &ev);  
}

class GetTask : public Task{
	public:
		GetTask(client* c):client_(c){}
		virtual ~GetTask(){}
		virtual void Process(){
			MutexLock l(&cache_m);
			sprintf(client_->writebuf, "get key: %s, value: %s\r\n", 
					client_->cmd_part[1], cache[client_->cmd_part[1]].c_str());
			client_->wend = strlen(client_->writebuf) + 1;		//ignore '\0' in the end of C style string
			client_->wpos = 0;
			register_event_helper(client_, EPOLLOUT);
		}
	private:
	client* client_;
};

class SetTask : public Task{
	public:
		SetTask(client* c):client_(c){}
		virtual ~SetTask(){}
		virtual void Process(){
			client_->readbuf[client_->rpos-2] = '\0'; //replace '\r' with '\0', make the data as a C style string
			char* key = client_->cmd_part[1];
			char* val = client_->cmd_end+1;
			{
				MutexLock l(&cache_m);
				cache[key] = val;
			}

			sprintf(client_->writebuf, "set key: %s, value: %s\r\n", key, val);
			client_->wend = strlen(client_->writebuf) + 1;
			client_->wpos = 0;
			register_event_helper(client_, EPOLLOUT);
		}
	private:
	client* client_;
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

ThreadPool thread_pool(5);

int handle_read_event(client* c)
{
	while(1){
		switch(c->state){
			case CMD_CHECK:
				c->cmd_end = (char*)memchr(c->readbuf, '\n', c->rpos);
				if(!c->cmd_end)	//not a complete cmd
					return 0;

				//received a complete cmd
				if(*(c->cmd_end-1) != '\r')	//bad format
					return -1;
				c->state = CMD_PARSE;
				break;
			case CMD_PARSE:
				{
					//parse command
					int npart = 0;
					*(c->cmd_end-1) = '\0';	//set for strtok
					for (char* p=c->readbuf; ; p = NULL) {
						char* token = strtok(p, " ");
						if (token == NULL)
							break;
						c->cmd_part[npart++] = token;
					}
					if(strcmp(c->cmd_part[0], "get") == 0) //get command
						c->state = GET_CMD;
					else if(strcmp(c->cmd_part[0], "set") == 0)
						c->state = SET_CMD;
					else return -1;		//bad command
					break;
				}
			case GET_CMD:
				{
					GetTask* p = new GetTask(c);
					thread_pool.AddTask(p);
					return 1;
					break;
				}
			case SET_CMD:
				{
					int bytes = atoi(c->cmd_part[2]);	//bytes of data block, to do: what if byte is 0 ?
					if(c->readbuf+c->rpos-(c->cmd_end+1) < bytes + 2) //data not received completelly, +2 for '\r\n'
						return 0;

					SetTask* p = new SetTask(c);
					thread_pool.AddTask(p);
					return 1;
					break;
				}
		}
	}
}


int main()
{
	char neterr[ANET_ERR_LEN];
	struct epoll_event ev; 

	int listenfd;

	signal(SIGPIPE, SIG_IGN);	//ignore sigpipe to prevent server shut down

	thread_pool.Run();

	if((listenfd = anetTcpServer(neterr, 9999, NULL)) == ANET_ERR){
		fprintf(stderr, "%s\n", neterr);
		exit(1);
	}

	anetNonBlock(neterr, listenfd); 

	if((epfd = epoll_create(MAX_CLIENTS)) < 0){
		perror("epoll_create");
		exit(1);
	}

	ev.events = EPOLLIN; 
	ev.data.fd =listenfd; 
	if(epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev)<0){
		perror("epoll_ctl");
		exit(1);
	} 

	for(;;){ 

		int nfds = epoll_wait(epfd, events, MAX_CLIENTS, -1);  
		for(int i = 0; i < nfds; ++i) { 

			int fd = events[i].data.fd;

			if(fd  == listenfd) { 
				struct sockaddr_in	cliaddr;
				socklen_t addrlen = sizeof(cliaddr);
				int connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &addrlen); 
				if(connfd < 0){ 
					if(errno == ECONNABORTED)	//connection aborted, ignore it
						continue; 
					else if(errno == EAGAIN)	//no connections are ready to be accepted
						continue; 
					else{	//error happened in accept, report it
						perror("accept"); 
						continue;
					}
				} 
				
				fprintf(stderr, "a new connfd: %d\n", connfd);  
				anetNonBlock(neterr, connfd); 

				ev.events = EPOLLIN; 
				ev.data.fd = connfd; 
				if (epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev) < 0) { 
					perror("epoll_ctl");
					return -1; 
				} 
				clients[connfd].fd = connfd;
				nclients++;
			} 
			else if(events[i].events & EPOLLIN)  
            {  
				int n;
				char* rbuf = clients[fd].readbuf;
                while ((n = read(fd, rbuf+clients[fd].rpos, MAX_BUFLEN-clients[fd].rpos)) > 0)
                    clients[fd].rpos += n;

				if (n == 0) {  //client has been closed
                    close(fd);  
					nclients--;
				}
				if (n<0 && errno != EAGAIN) {
					perror("read");
					close(fd);  
					nclients--;
				}
				int res = handle_read_event(&clients[fd]);
				if(res<0){ //error happened
					fprintf(stderr, "an error happened\n");  
					close(fd);  
					nclients--;
				}else if(res>0){	//the command has been hand over to a background thread, nothing to do here
				}else{ //res == 0, needs more data, continue reading
				}
            }
            else if(events[i].events & EPOLLOUT)  
            {     
				char* wbuf = clients[fd].writebuf;
				int wend = clients[fd].wend;
				int n;
                while (wend>clients[fd].wpos && 
						(n = write(fd, wbuf+clients[fd].wpos, wend-clients[fd].wpos)) > 0)
					clients[fd].wpos += n;

				if (n<0 && errno != EAGAIN) {
					perror("write");
					close(fd);  
					nclients--;
				}

				if(wend == clients[fd].wpos){ //write finished
					//fprintf(stderr, "write to fd %d, content: %s", fd, wbuf);  
					//no need to rest for next write cause read event handler will do it
					//register as read event
					ev.data.fd=fd;  
					ev.events=EPOLLIN;  
					epoll_ctl(epfd,EPOLL_CTL_MOD,fd,&ev);  
				}
            }  
			else{ 
				//impossible!
		  	}
		}
	}

	return 0;
}
