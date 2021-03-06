#ifndef lock_server_cache_rsm_h
#define lock_server_cache_rsm_h

#include <string>
#include <set>
#include <map>

#include "lock_protocol.h"
#include "rpc.h"
#include "fifo.h"
#include "rsm_state_transfer.h"
#include "rsm.h"
#include "marshall.h"

struct client_info{
	client_info() { VERIFY(pthread_cond_init(&cond, 0) == 0); }
	client_info(std::string id, lock_protocol::xid_t xid_, bool avail) : cid(id), xid(xid_), available(avail)
	{ VERIFY(pthread_cond_init(&cond, 0) == 0); }
	~client_info()
	{ VERIFY(pthread_cond_destroy(&cond) == 0); }
	std::string cid;	// host:port
	lock_protocol::xid_t xid;
	bool available;
	std::set<std::string> waited_clients;
	pthread_cond_t cond;
};

inline unmarshall& operator>>(unmarshall &u, client_info* ci)
{
	u >> ci->cid;
	u >> ci->xid;
	u >> ci->available;
	int wclt_size;
	u >> wclt_size;
	for(int i=0; i< wclt_size; i++){
		std::string clt;
		u >> clt;
		ci->waited_clients.insert(clt);
	}
	return u;
}

inline marshall& operator<<(marshall &m, client_info* ci)
{
	m << ci->cid;
	m << ci->xid;
	m << ci->available;

	m << (unsigned int)ci->waited_clients.size();
	std::set<std::string>::iterator it;
	for(it=ci->waited_clients.begin(); it!= ci->waited_clients.end(); it++)
		m << *it;
	return m;
}

class lock_server_cache_rsm : public rsm_state_transfer {
 private:
  
  int nacquire;
  class rsm *rsm;
  fifo<lock_protocol::lockid_t> revoker_queue;
  fifo<lock_protocol::lockid_t> retryer_queue;
  std::map <lock_protocol::lockid_t, client_info*> lock_clients;
  pthread_mutex_t lock_clients_mutex;

  rlock_protocol::status call_client_rpc(rlock_protocol::rpc_numbers rpc_num, 
		  lock_protocol::lockid_t lid, std::string& id, lock_protocol::xid_t xid);

  
 public:
  lock_server_cache_rsm(class rsm *rsm = 0);
  lock_protocol::status stat(lock_protocol::lockid_t, int &);
  void revoker();
  void retryer();
  std::string marshal_state();
  void unmarshal_state(std::string state);
  int acquire(lock_protocol::lockid_t, std::string id, 
	      lock_protocol::xid_t, int &);
  int release(lock_protocol::lockid_t, std::string id, lock_protocol::xid_t,
	      int &);
};

#endif
