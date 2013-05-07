// lock client interface.

#ifndef lock_client_cache_h

#define lock_client_cache_h

#include <string>
#include "lock_protocol.h"
#include "rpc.h"
#include "lock_client.h"
#include "lang/verify.h"

// Classes that inherit lock_release_user can override dorelease so that 
// that they will be called when lock_client releases a lock.
// You will not need to do anything with this class until Lab 5.
class lock_release_user {
 public:
  virtual void dorelease(lock_protocol::lockid_t) = 0;
  virtual ~lock_release_user() {};
};

class lock_client_cache : public lock_client {
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
  class lock_release_user *lu;
  int rlock_port;
  std::string hostname;
  std::string id;

  pthread_mutex_t lm;
  std::map<lock_protocol::lockid_t, lock_info> locks;

 public:
  lock_client_cache(std::string xdst, class lock_release_user *l = 0);
  virtual ~lock_client_cache();
  lock_protocol::status acquire(lock_protocol::lockid_t);
  lock_protocol::status release(lock_protocol::lockid_t);
  rlock_protocol::status revoke_handler(lock_protocol::lockid_t, 
                                        int &);
  rlock_protocol::status retry_handler(lock_protocol::lockid_t, 
                                       int &);
};


#endif
