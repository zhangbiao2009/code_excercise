// RPC stubs for clients to talk to extent_server

#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

// The calls assume that the caller holds a lock on the extent

extent_client::extent_client(std::string dst)
{
  sockaddr_in dstsock;
  make_sockaddr(dst.c_str(), &dstsock);
  cl = new rpcc(dstsock);
  if (cl->bind() != 0) {
    printf("extent_client: bind failed\n");
  }
}

extent_protocol::status
extent_client::get(extent_protocol::extentid_t eid, std::string &buf)
{
  extent_protocol::status ret = extent_protocol::OK;
  if(cached_extents.find(eid) != cached_extents.end()){
	  buf = cached_extents[eid];
	  cached_attrs[eid].atime = time(NULL);
  }
  else{
	  ret = cl->call(extent_protocol::get, eid, buf);
	  if(ret == extent_protocol::OK)
		  cached_extents[eid] = buf;
  }
  return ret;
}

extent_protocol::status
extent_client::getattr(extent_protocol::extentid_t eid, 
		       extent_protocol::attr &attr)
{
  extent_protocol::status ret = extent_protocol::OK;
  if(cached_extents.find(eid) != cached_extents.end())
	  attr = cached_attrs[eid];
  else{
	  ret = cl->call(extent_protocol::getattr, eid, attr);
	  if(ret == extent_protocol::OK)
		  cached_attrs[eid] = attr;
  }
  return ret;
}

extent_protocol::status
extent_client::setattr(extent_protocol::extentid_t eid, 
		       extent_protocol::attr attr)
{
  extent_protocol::status ret = extent_protocol::OK;
  int r;
  cached_attrs[eid] = attr;
  dirty_attrs.insert(eid);
  return ret;
  ret = cl->call(extent_protocol::setattr, eid, attr, r);
  return ret;
}

extent_protocol::status
extent_client::put(extent_protocol::extentid_t eid, std::string buf)
{
  extent_protocol::status ret = extent_protocol::OK;
  int r;
  cached_extents[eid] = buf;
  cached_attrs[eid].size = buf.length();
  cached_attrs[eid].ctime = cached_attrs[eid].mtime = time(NULL);

  dirty_extents.insert(eid);
  return ret;
  ret = cl->call(extent_protocol::put, eid, buf, r);
  return ret;
}

extent_protocol::status
extent_client::remove(extent_protocol::extentid_t eid)
{
  extent_protocol::status ret = extent_protocol::OK;
  int r;
  std::map<extent_protocol::extentid_t, std::string>::iterator it;
  if((it=cached_extents.find(eid)) == cached_extents.end())
	  return extent_protocol::NOENT;
  cached_extents.erase(it);
  dirty_extents.erase(eid);
  dirty_attrs.erase(eid);
  return ret;

  ret = cl->call(extent_protocol::remove, eid, r);
  return ret;
}


