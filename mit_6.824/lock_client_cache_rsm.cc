// RPC stubs for clients to talk to lock_server, and cache the locks
// see lock_client.cache.h for protocol details.

#include "lock_client_cache_rsm.h"
#include "rpc.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include "tprintf.h"

#include "rsm_client.h"

static void *
releasethread(void *x)
{
  lock_client_cache_rsm *cc = (lock_client_cache_rsm *) x;
  cc->releaser();
  return 0;
}

int lock_client_cache_rsm::last_port = 0;

lock_client_cache_rsm::lock_client_cache_rsm(std::string xdst, 
				     class lock_release_user *_lu)
  : lock_client(xdst), lu(_lu)
{
  srand(time(NULL)^last_port);
  rlock_port = ((rand()%32000) | (0x1 << 10));
  const char *hname;
  // VERIFY(gethostname(hname, 100) == 0);
  hname = "127.0.0.1";
  std::ostringstream host;
  host << hname << ":" << rlock_port;
  id = host.str();
  last_port = rlock_port;
  rpcs *rlsrpc = new rpcs(rlock_port);
  rlsrpc->reg(rlock_protocol::revoke, this, &lock_client_cache_rsm::revoke_handler);
  rlsrpc->reg(rlock_protocol::retry, this, &lock_client_cache_rsm::retry_handler);
  xid = 0;
  // You fill this in Step Two, Lab 7
  // - Create rsmc, and use the object to do RPC 
  //   calls instead of the rpcc object of lock_client
  pthread_t th;
  int r = pthread_create(&th, NULL, &releasethread, (void *) this);
  VERIFY (r == 0);
  VERIFY(pthread_mutex_init(&lm, 0) == 0);
}


void
lock_client_cache_rsm::releaser()
{

  // This method should be a continuous loop, waiting to be notified of
  // freed locks that have been revoked by the server, so that it can
  // send a release RPC.

}


lock_protocol::status
lock_client_cache_rsm::acquire(lock_protocol::lockid_t lid)
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
if the lock is held by another client, rpcc acquire() return RETRY immediately.
*/
		  xid++;
		  pthread_mutex_unlock(&lm); //unlock because rpc acquire may block a long time
		  while((ret = cl->call(lock_protocol::acquire, lid, id, xid, r)) == lock_protocol::RETRY){	// waiting for retry msg
			  pthread_mutex_lock(&lm);
			  while(!locks[lid].retry)		//retry msg not come yet
				  pthread_cond_wait(&locks[lid].retry_cond, &lm);
			  pthread_mutex_unlock(&lm); //unlock because rpc acquire may block a long time
		  }
		  pthread_mutex_lock(&lm);
		  VERIFY (ret == lock_protocol::OK);
		  locks[lid].stat = LOCKED;

		  break;
	  case ACQUIRING:
	  case LOCKED:
	  case RELEASING:
		  // wait until lock available (FREE, NONE)
		  while(locks[lid].stat != FREE && locks[lid].stat != NONE){
			  pthread_cond_wait(&locks[lid].lcond, &lm);
		  }
		  goto begin;
		  break;
	  case FREE:
		  locks[lid].stat = LOCKED;
		  break;
  }

  return ret;
}

lock_protocol::status
lock_client_cache_rsm::release(lock_protocol::lockid_t lid)
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
		if(lu)
			lu->dorelease(lid);
		ret = cl->call(lock_protocol::release, lid, id, xid, r);
		VERIFY (ret == lock_protocol::OK);
		pthread_mutex_lock(&lm);
		locks[lid].stat = NONE;
		locks[lid].revoked = false;
	}
	pthread_cond_signal(&locks[lid].lcond);

	return ret;

}

rlock_protocol::status
lock_client_cache_rsm::revoke_handler(lock_protocol::lockid_t lid, 
		lock_protocol::xid_t xid, int &)
{
  int ret = rlock_protocol::OK;

  ScopedLock sl(&lm);
  locks[lid].revoked = true;
  if(locks[lid].stat == FREE){   //return to server immediately
	  int r;
	  pthread_mutex_unlock(&lm);
	  if(lu)
		  lu->dorelease(lid);
	  ret = cl->call(lock_protocol::release, lid, id, xid, r);
	  VERIFY (ret == lock_protocol::OK);
	  pthread_mutex_lock(&lm);
	  locks[lid].stat = NONE;
	  locks[lid].revoked = false;
  }
  return ret; 	//just return, the lock holder will call release() later, which will return the lock to server
}

rlock_protocol::status
lock_client_cache_rsm::retry_handler(lock_protocol::lockid_t lid, 
		lock_protocol::xid_t xid, int &)
{
  int ret = rlock_protocol::OK;
  ScopedLock sl(&lm);
  locks[lid].retry = true;
  pthread_cond_signal(&locks[lid].retry_cond);
  return ret;
}
