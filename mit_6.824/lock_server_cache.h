#ifndef lock_server_cache_h
#define lock_server_cache_h

#include <string>

#include <map>
#include <set>
#include "lock_protocol.h"
#include "rpc.h"
#include "lock_server.h"


class lock_server_cache {
 private:
  struct client_info{
	  client_info() : cid(""), available(true), waiting(false)	//add this constructor just for compilation, it shouldn't be called
	  { abort(); }
	  client_info(std::string id, bool avail) : cid(id), available(avail), waiting(false)
	  { VERIFY(pthread_cond_init(&cond, 0) == 0); }
	  ~client_info()
	  { VERIFY(pthread_cond_destroy(&cond) == 0); }
	  std::string cid;	// host:port
	  bool available;
	  bool waiting;
	  std::set<std::string> waited_clients;
	  pthread_cond_t cond;
  };
  int nacquire;
  std::map <lock_protocol::lockid_t, client_info> lock_clients;
  pthread_mutex_t lock_clients_mutex;

  rlock_protocol::status call_client_rpc(rlock_protocol::rpc_numbers rpc_num, lock_protocol::lockid_t lid, std::string& id);

 public:
  lock_server_cache();
  ~lock_server_cache();
  lock_protocol::status stat(lock_protocol::lockid_t, int &);
  int acquire(lock_protocol::lockid_t, std::string id, int &);
  int release(lock_protocol::lockid_t, std::string id, int &);
};

#endif
