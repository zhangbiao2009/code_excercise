// the caching lock server implementation

#include "lock_server_cache_rsm.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "lang/verify.h"
#include "handle.h"
#include "tprintf.h"


static void *
revokethread(void *x)
{
  lock_server_cache_rsm *sc = (lock_server_cache_rsm *) x;
  sc->revoker();
  return 0;
}

static void *
retrythread(void *x)
{
  lock_server_cache_rsm *sc = (lock_server_cache_rsm *) x;
  sc->retryer();
  return 0;
}

lock_server_cache_rsm::lock_server_cache_rsm(class rsm *_rsm) 
  : rsm (_rsm)
{
  pthread_t th;
  int r = pthread_create(&th, NULL, &revokethread, (void *) this);
  VERIFY (r == 0);
  r = pthread_create(&th, NULL, &retrythread, (void *) this);
  VERIFY (r == 0);
  VERIFY(pthread_mutex_init(&lock_clients_mutex, 0) == 0);
}

void
lock_server_cache_rsm::revoker()
{

  // This method should be a continuous loop, that sends revoke
  // messages to lock holders whenever another client wants the
  // same lock

	while(1){
		lock_protocol::lockid_t lid;
		// sends revoke message to lock holder
		revoker_queue.deq(&lid);
		tprintf("send revoke msg to cid=%s for lid=%llu\n", lock_clients[lid].cid.c_str(), lid);
		if(call_client_rpc(rlock_protocol::revoke, lid, lock_clients[lid].cid, lock_clients[lid].xid) != rlock_protocol::OK){
			// what to do it revoke failed?	unrecoverable error!
			abort();
		}

		tprintf("lock_server_cache_rsm::revoker: add lid=%llu to retryer_queue\n", lid);
		// notify retrythread
		retryer_queue.enq(lid);
	}
}


void
lock_server_cache_rsm::retryer()
{

  // This method should be a continuous loop, waiting for locks
  // to be released and then sending retry messages to those who
  // are waiting for it.

	while(1) {
		lock_protocol::lockid_t lid;
		retryer_queue.deq(&lid);
		// wait until the lock has been released
		pthread_mutex_lock(&lock_clients_mutex);
		while(!lock_clients[lid].available)
			pthread_cond_wait(&lock_clients[lid].cond, &lock_clients_mutex);		//wait the lock holder release the lock

		//send a retry to one of waited clients
		assert(!lock_clients[lid].waited_clients.empty());

		std::string cid = *lock_clients[lid].waited_clients.begin();
		/* pass 0 as xid temporarily as xid is not so useful for retry handler */
		pthread_mutex_unlock(&lock_clients_mutex);
		tprintf("send retry msg to cid=%s for lid=%llu\n", cid.c_str(), lid);
		if(call_client_rpc(rlock_protocol::retry, lid, cid, 0) != rlock_protocol::OK){
			//what to do it retry failed?	unrecoverable error!
			abort();
		}
	}
}

rlock_protocol::status
lock_server_cache_rsm::call_client_rpc(rlock_protocol::rpc_numbers rpc_num,
	   	lock_protocol::lockid_t lid, std::string& id, lock_protocol::xid_t xid)
{
	handle h(id);
	rlock_protocol::status ret = rlock_protocol::OK;
	if (h.safebind()) {
		int r;
		ret = h.safebind()->call(rpc_num, lid, xid, r);
	}
	if (!h.safebind() || ret != rlock_protocol::OK) {
		ret = rlock_protocol::RPCERR;
	}
	return ret;
}

int lock_server_cache_rsm::acquire(lock_protocol::lockid_t lid, std::string id, 
             lock_protocol::xid_t xid, int &)
{
  lock_protocol::status ret = lock_protocol::OK;

  ScopedLock ml(&lock_clients_mutex);
  // check if it is a duplicated request
  if(lock_clients.find(lid) != lock_clients.end() && id == lock_clients[lid].cid 
		  && xid == lock_clients[lid].xid){
	  tprintf("lock_server_cache_rsm::acquire: duplicate request for lock %llu from %s, xid=%llu \n", lid, id.c_str(), xid);
	  return ret;
  }

  // it's a new request or a retry request

  //the first requester of this lock
  if(lock_clients.find(lid) == lock_clients.end()){
	  tprintf("lock_server_cache_rsm::acquire: here0, lid=%llu, id=%s, xid=%llu\n", lid, id.c_str(), xid);
	  lock_clients.insert(std::pair<lock_protocol::lockid_t, client_info>(lid, client_info(id, xid, false)));
	  return ret;
  }

  std::set<std::string>* waited_clts = &lock_clients[lid].waited_clients;
  std::set<std::string>::iterator it;
  tprintf("waited_clts content:\t");
  for(it=waited_clts->begin(); it!= waited_clts->end(); it++)
	  printf("%s,", (*it).c_str());
  printf("\n");

  if(lock_clients[lid].available){
	  tprintf("lock lid=%llu is available\n", lid);
  }else{
	  tprintf("current lock holder of lid=%llu: %s\n", lid, lock_clients[lid].cid.c_str());
  }

  // there's other clients expect current client already in the waited_clts, so I'm a late comer
  if(!waited_clts->empty() && waited_clts->find(id) == waited_clts->end()){
	  tprintf("lock_server_cache_rsm::acquire: here1, lid=%llu, id=%s, xid=%llu\n", lid, id.c_str(), xid);
	  tprintf("waited_clt insert id=%s\n", id.c_str());
	  waited_clts->insert(id);
	  return lock_protocol::RETRY;
  }
  if(!lock_clients[lid].available){
	  tprintf("lock_server_cache_rsm::acquire: here2, lid=%llu, id=%s, xid=%llu\n", lid, id.c_str(), xid);
	  tprintf("waited_clt insert id=%s\n", id.c_str());
	  waited_clts->insert(id);
	  //notify revoke thread 
	  if(rsm->amiprimary()){
		  tprintf("lock_server_cache_rsm::acquire: add lid=%llu to revoker_queue\n", lid);
		  revoker_queue.enq(lid);
	  }
	  return lock_protocol::RETRY;
  }

	  tprintf("lock_server_cache_rsm::acquire: here3, lid=%llu, id=%s, xid=%llu\n", lid, id.c_str(), xid);
  //lock is available
  lock_clients[lid].cid = id;
  lock_clients[lid].xid = xid;
  lock_clients[lid].available = false;

  //remove current client from waited clients set (if exists)
  tprintf("waited_clt remove id=%s\n", id.c_str());
  waited_clts->erase(id);

  if(!waited_clts->empty() && rsm->amiprimary()){	// ask the lock holder releasing the lock ASAP and send retry to one of waited clients
	  tprintf("lock_server_cache_rsm::acquire: add lid=%llu to revoker_queue\n", lid);
	  revoker_queue.enq(lid);
  }

  return ret;
}

int 
lock_server_cache_rsm::release(lock_protocol::lockid_t lid, std::string id, 
         lock_protocol::xid_t xid, int &r)
{
  lock_protocol::status ret = lock_protocol::OK;
  ScopedLock ml(&lock_clients_mutex);

  // todo: do we need to check duplicate request? for release(), I think it's not harmful executing duplicate requests
  // todo check: xid must equal to lock_clients[lid].xid, otherwise the client is sending wrong RPC

  std::map<lock_protocol::lockid_t, client_info>::iterator it = lock_clients.find(lid);
  if(it != lock_clients.end() && it->second.cid == id) {
	  it->second.available = true;
      pthread_cond_signal(&lock_clients[lid].cond);		//wake up the retry thread
  }/* else {
      ret = lock_protocol::NOENT;
  }*/
  r = lid;
  tprintf("lock_server_cache_rsm::release: here1, lid=%llu, id=%s, xid=%llu\n", lid, id.c_str(), xid);
  return ret;
}

std::string
lock_server_cache_rsm::marshal_state()
{
  std::ostringstream ost;
  std::string r;
  return r;
}

void
lock_server_cache_rsm::unmarshal_state(std::string state)
{
}

lock_protocol::status
lock_server_cache_rsm::stat(lock_protocol::lockid_t lid, int &r)
{
  printf("stat request\n");
  r = nacquire;
  return lock_protocol::OK;
}

