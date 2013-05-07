// the caching lock server implementation

#include "lock_server_cache.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "lang/verify.h"
#include "handle.h"
#include "tprintf.h"


lock_server_cache::lock_server_cache()
{
	VERIFY(pthread_mutex_init(&lock_clients_mutex, 0) == 0);
}

lock_server_cache::~lock_server_cache()
{
	VERIFY(pthread_mutex_destroy(&lock_clients_mutex) == 0);
}

int lock_server_cache::acquire(lock_protocol::lockid_t lid, std::string id, 
                               int &)
{
/*
   if lock is free, return it.
    if the lock is held by another client and some clients are waiting for it, (add this client to waited clients set??), return RETRY msg immediately.
    if lock is held by another client and no other clients are waiting for it, send revoke msg to the lock holder, when the lock holder release the lock, return it. before returning, send retry msg to one of the waited lock clients.
*/
  lock_protocol::status ret = lock_protocol::OK;
  ScopedLock ml(&lock_clients_mutex);
  if(lock_clients.find(lid) != lock_clients.end() && !(lock_clients[lid].available && lock_clients[lid].waited_clients.empty())){
	  //no one is waiting for the lock and I'm in waited client set or the first one waiting for the lock
	  std::set<std::string>* waited_clts = &lock_clients[lid].waited_clients;
	  if(!lock_clients[lid].available && !lock_clients[lid].waiting && 
			  (waited_clts->empty() || waited_clts->find(id) != waited_clts->end())){
		  lock_clients[lid].waiting = true;
		  //add the client to waited clients set
		  if(waited_clts->empty())
			  waited_clts->insert(id);
		  //revoke lid from its owner, revoke should return quickly. 
		  pthread_mutex_unlock(&lock_clients_mutex);
		  if(call_client_rpc(rlock_protocol::revoke, lid, lock_clients[lid].cid) != rlock_protocol::OK){
			  //what to do it revoke failed?	unrecoverable error!
			  abort();
		  }
		  pthread_mutex_lock(&lock_clients_mutex);
		  while(!lock_clients[lid].available)
			  pthread_cond_wait(&lock_clients[lid].cond, &lock_clients_mutex);		//wait the lock holder release the lock
		  //todo: what happens if the lock holder client crashes before releasing the lock?
		  lock_clients[lid].waiting = false;
	  }else{	//some one has already been waiting for the lock or I'm a new comer (not in waited clients set)
		  //add the clt to waited clients set
		  waited_clts->insert(id);
		  return lock_protocol::RETRY;
	  }
  }

  assert(lock_clients.find(lid) == lock_clients.end() || lock_clients[lid].available);
  if(lock_clients.find(lid) != lock_clients.end()){
	  lock_clients[lid].available = false;
	  lock_clients[lid].cid = id;
  }else
	  lock_clients.insert(std::pair<lock_protocol::lockid_t, client_info>(lid, client_info(id, false)));

  //remove current client from waited clients set
  lock_clients[lid].waited_clients.erase(id);
  //send a retry to one of waited clients (if any)
  if(!lock_clients[lid].waited_clients.empty()){
	  std::string cid = *lock_clients[lid].waited_clients.begin();
	  pthread_mutex_unlock(&lock_clients_mutex);
	  if(call_client_rpc(rlock_protocol::retry, lid, cid) != rlock_protocol::OK){
		  //what to do it retry failed?	unrecoverable error!
		  abort();
	  }
	  pthread_mutex_lock(&lock_clients_mutex);
  }
  return ret;
}

rlock_protocol::status
lock_server_cache::call_client_rpc(rlock_protocol::rpc_numbers rpc_num, lock_protocol::lockid_t lid, std::string& id)
{
	handle h(id);
	rlock_protocol::status ret = rlock_protocol::OK;
	if (h.safebind()) {
		int r;
		ret = h.safebind()->call(rpc_num, lid, r);
	}
	if (!h.safebind() || ret != rlock_protocol::OK) {
		ret = rlock_protocol::RPCERR;
	}
	return ret;
}

int 
lock_server_cache::release(lock_protocol::lockid_t lid, std::string id, 
         int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  ScopedLock ml(&lock_clients_mutex);
  std::map<lock_protocol::lockid_t, client_info>::iterator it = lock_clients.find(lid);
  if(it != lock_clients.end() && it->second.cid == id) {
	  it->second.available = true;
      pthread_cond_signal(&lock_clients[lid].cond);		//wake up the thread who is acquiring this lock
  }/* else {
      ret = lock_protocol::NOENT;
  }*/
  r = lid;
  return ret;
}

lock_protocol::status
lock_server_cache::stat(lock_protocol::lockid_t lid, int &r)
{
  tprintf("stat request\n");
  r = nacquire;
  return lock_protocol::OK;
}

