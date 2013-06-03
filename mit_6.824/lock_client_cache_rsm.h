// lock client interface.

#ifndef lock_client_cache_rsm_h

#define lock_client_cache_rsm_h

#include <string>
#include <map>
#include "lock_protocol.h"
#include "rpc.h"
#include "lock_client.h"
#include "lang/verify.h"

#include "rsm_client.h"

// Classes that inherit lock_release_user can override dorelease so that 
// that they will be called when lock_client releases a lock.
// You will not need to do anything with this class until Lab 5.
class lock_release_user {
 public:
  virtual void dorelease(lock_protocol::lockid_t) = 0;
  virtual ~lock_release_user() {};
};


class lock_client_cache_rsm;

// Clients that caches locks.  The server can revoke locks using 
// lock_revoke_server.
class lock_client_cache_rsm : public lock_client {
 private:
  enum xxstatus { NONE, ACQUIRING, LOCKED, RELEASING, FREE };
  typedef int status;
  struct lock_info{ 
	  bool revoked; 
	  status stat;
	  bool retry;
	  pthread_cond_t lcond;
	  pthread_cond_t retry_cond;
	  lock_info(): revoked(false), stat(NONE), retry(false)
	  { 
		  VERIFY(pthread_cond_init(&lcond, 0) == 0); 
		  VERIFY(pthread_cond_init(&retry_cond, 0) == 0); 
	  }
	  ~lock_info()
	  { 
		  VERIFY(pthread_cond_destroy(&lcond) == 0); 
		  VERIFY(pthread_cond_destroy(&retry_cond) == 0); 
	  }
  };
  rsm_client *rsmc;
  class lock_release_user *lu;
  int rlock_port;
  std::string hostname;
  std::string id;
  lock_protocol::xid_t xid;

  pthread_mutex_t lm;
  std::map<lock_protocol::lockid_t, lock_info> locks;

 public:
  static int last_port;
  lock_client_cache_rsm(std::string xdst, class lock_release_user *l = 0);
  virtual ~lock_client_cache_rsm() {};
  lock_protocol::status acquire(lock_protocol::lockid_t);
  virtual lock_protocol::status release(lock_protocol::lockid_t);
  void releaser();
  rlock_protocol::status revoke_handler(lock_protocol::lockid_t, 
				        lock_protocol::xid_t, int &);
  rlock_protocol::status retry_handler(lock_protocol::lockid_t, 
				       lock_protocol::xid_t, int &);
};


#endif
