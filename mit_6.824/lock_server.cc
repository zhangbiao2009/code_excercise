// the lock server implementation

#include "lock_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

lock_server::lock_server():
  nacquire (0)
{
  VERIFY(pthread_mutex_init(&lock_clients_mutex, 0) == 0);
  VERIFY(pthread_cond_init(&lock_clients_cond, 0) == 0);
}
lock_server::~lock_server()
{
  VERIFY(pthread_mutex_destroy(&lock_clients_mutex) == 0);
  VERIFY(pthread_cond_destroy(&lock_clients_cond) == 0);
}

lock_protocol::status
lock_server::stat(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  printf("stat request from clt %d\n", clt);
  r = nacquire;
  return ret;
}

lock_protocol::status
lock_server::acquire(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  ScopedLock ml(&lock_clients_mutex);
  printf("before acquire request from clt %d\n", clt);
  locktable_dump();
  while(lock_clients.find(lid) != lock_clients.end())
      pthread_cond_wait(&lock_clients_cond, &lock_clients_mutex);

  lock_clients[lid] = clt;
  r = lid;
  printf("after acquire request from clt %d\n", clt);
  locktable_dump();
  return ret;
}

lock_protocol::status
lock_server::release(int clt, lock_protocol::lockid_t lid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  ScopedLock ml(&lock_clients_mutex);
  printf("before release request from clt %d\n", clt);
  locktable_dump();
  std::map <int, int>::iterator it = lock_clients.find(lid);
  if(it != lock_clients.end() && it->second == clt) {
      lock_clients.erase(it);
      pthread_cond_signal(&lock_clients_cond);
  }/* else {
      ret = lock_protocol::NOENT;
  }*/
  printf("after release request from clt %d\n", clt);
  locktable_dump();
  r = lid;
  return ret;
}

void 
lock_server::locktable_dump()
{
  std::map <int, int>::iterator it = lock_clients.begin();
  printf("lock table dump begin: \n");
  for(; it != lock_clients.end(); it++) {
      printf("clt %d holds lock %d\n", it->second, it->first);
  }
  printf("lock table dump end\n");
}
