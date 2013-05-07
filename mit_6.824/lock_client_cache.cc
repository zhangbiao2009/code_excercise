// RPC stubs for clients to talk to lock_server, and cache the locks
// see lock_client.cache.h for protocol details.

#include "lock_client_cache.h"
#include "rpc.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include "tprintf.h"


lock_client_cache::lock_client_cache(std::string xdst, 
				     class lock_release_user *_lu)
  : lock_client(xdst), lu(_lu)
{
  rpcs *rlsrpc = new rpcs(0);
  rlsrpc->reg(rlock_protocol::revoke, this, &lock_client_cache::revoke_handler);
  rlsrpc->reg(rlock_protocol::retry, this, &lock_client_cache::retry_handler);

  const char *hname;
  hname = "127.0.0.1";
  std::ostringstream host;
  host << hname << ":" << rlsrpc->port();
  id = host.str();

  VERIFY(pthread_mutex_init(&lm, 0) == 0);
}

lock_client_cache::~lock_client_cache() 
{
	VERIFY(pthread_mutex_destroy(&lm) == 0);
}

lock_protocol::status
lock_client_cache::acquire(lock_protocol::lockid_t lid)
{
  lock_protocol::status ret = lock_protocol::OK;
  ScopedLock sl(&lm);
begin:
  switch(locks[lid].stat){		// if locks[lid] doesn't exist, it'll be created automatically.
	  case NONE:
		  //rpc acquire
		  int r;
		  locks[lid].stat = ACQUIRING;
/*
rpc acquire() may return immediately if no one hold the lock you want
if the lock is held by another client and no other clients are waiting for it, rpcc acquire() may block a while and return ok finally.
if the lock is held by another client and some clients are waiting for it, rpcc acquire() return RETRY immediately.
*/
		  pthread_mutex_unlock(&lm); //unlock because rpc acquire may block a long time
		  tprintf("thread %lu: call rpc acuqire for lock %llu from client %s\n", pthread_self(), lid, id.c_str());
		  while((ret = cl->call(lock_protocol::acquire, lid, id, r)) == lock_protocol::RETRY){	// waiting for retry msg
			  pthread_mutex_lock(&lm);
			  while(!locks[lid].retry)		//retry msg not come yet
				  pthread_cond_wait(&locks[lid].retry_cond, &lm);
			  pthread_mutex_unlock(&lm); //unlock because rpc acquire may block a long time
		  }
		  pthread_mutex_lock(&lm);
		  tprintf("thread %lu: rpc acuqire returned for lock %llu from client %s\n", pthread_self(), lid, id.c_str());
		  VERIFY (ret == lock_protocol::OK);
		  locks[lid].stat = LOCKED;

		  break;
	  case ACQUIRING:
	  case LOCKED:
	  case RELEASING:
		  // wait until lock available (FREE, NONE)
		  tprintf("thread %lu: wait lock %llu to be available\n", pthread_self(), lid);
		  while(locks[lid].stat != FREE && locks[lid].stat != NONE){
			  pthread_cond_wait(&locks[lid].lcond, &lm);
		  }
		  tprintf("thread %lu: lock %llu is available\n", pthread_self(), lid);
		  goto begin;
		  break;
	  case FREE:
		  locks[lid].stat = LOCKED;
		  break;
  }

  return ret;
}

lock_protocol::status
lock_client_cache::release(lock_protocol::lockid_t lid)
// assumption: only the thread who called acquire before can call release
{
	lock_protocol::status ret = lock_protocol::OK;

	ScopedLock sl(&lm);
	if(locks[lid].stat == NONE)	//nothing to do
		return ret;

	assert(locks[lid].stat == LOCKED || locks[lid].stat == FREE);

	if(!locks[lid].revoked){	//lock cache can be used, just set lock stat, not release back to server
		locks[lid].stat = FREE;
	}
	else{	//must release back to server
		locks[lid].stat = RELEASING;
		int r;
		pthread_mutex_unlock(&lm);
		tprintf("thread %lu: release lock %llu from client %s\n", pthread_self(), lid, id.c_str());
		ret = cl->call(lock_protocol::release, lid, id, r);
		tprintf("thread %lu: lock %llu released from client %s\n", pthread_self(), lid, id.c_str());
		VERIFY (ret == lock_protocol::OK);
		pthread_mutex_lock(&lm);
		locks[lid].stat = NONE;
		locks[lid].revoked = false;
	}
	pthread_cond_signal(&locks[lid].lcond);

	return ret;

}

rlock_protocol::status
lock_client_cache::revoke_handler(lock_protocol::lockid_t lid, 
                                  int &)
{
  int ret = rlock_protocol::OK;

  ScopedLock sl(&lm);
  locks[lid].revoked = true;
  if(locks[lid].stat == FREE){   //return to server immediately
	  int r;
	  pthread_mutex_unlock(&lm);
	  tprintf("thread %lu: release lock %llu in revoke_handler in client %s\n", pthread_self(), lid, id.c_str());
	  ret = cl->call(lock_protocol::release, lid, id, r);
	  tprintf("thread %lu: lock %llu has been released in revoke_handler in client %s\n", pthread_self(), lid, id.c_str());
	  VERIFY (ret == lock_protocol::OK);
	  pthread_mutex_lock(&lm);
	  locks[lid].stat = NONE;
	  locks[lid].revoked = false;
  }
  return ret; 	//just return, the lock holder will call release() later, which will return the lock to server
}

rlock_protocol::status
lock_client_cache::retry_handler(lock_protocol::lockid_t lid, 
                                 int &)
{
  int ret = rlock_protocol::OK;
  ScopedLock sl(&lm);
  locks[lid].retry = true;
  tprintf("thread %lu: retry_handler for lock %llu in client %s\n", pthread_self(), lid, id.c_str());
  pthread_cond_signal(&locks[lid].retry_cond);
  return ret;
}

