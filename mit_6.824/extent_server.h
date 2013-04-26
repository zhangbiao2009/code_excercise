// this is the extent server

#ifndef extent_server_h
#define extent_server_h

#include <string>
#include <map>
#include "extent_protocol.h"
#include "rpc/slock.h"

class extent_server {

 public:
  extent_server();
  ~extent_server();

  int put(extent_protocol::extentid_t id, std::string, int &);
  int get(extent_protocol::extentid_t id, std::string &);
  int getattr(extent_protocol::extentid_t id, extent_protocol::attr &);
  int remove(extent_protocol::extentid_t id, int &);
 private:
  pthread_mutex_t m_;
  std::map<extent_protocol::extentid_t, std::string> extents_;
};

#endif 







