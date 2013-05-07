// lock protocol

#ifndef lock_protocol_h
#define lock_protocol_h

#include "rpc.h"

/*
   when client a get a RETRY response, it knows that the lock is holding by client b,
   and another client c is acquiring it, so the lock is currently unavailable.

   when client a get a retry response, it means that the lock has been returned to server,
   and server is going to give the lock to client b, and there's no other client acquring the lock.
   so if client a wants the lock, it should acquire the lock immediately.
 */
class lock_protocol {
 public:
  enum xxstatus { OK, RETRY, RPCERR, NOENT, IOERR };
  typedef int status;
  typedef unsigned long long lockid_t;
  typedef unsigned long long xid_t;
  enum rpc_numbers {
    acquire = 0x7001,
    release,
    stat
  };
};

class rlock_protocol {
 public:
  enum xxstatus { OK, RPCERR };
  typedef int status;
  enum rpc_numbers {
    revoke = 0x8001,
    retry = 0x8002
  };
};
#endif 
