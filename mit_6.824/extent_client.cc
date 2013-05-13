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
	  if(cached_attrs.find(eid) == cached_attrs.end()){
		  extent_protocol::attr attr;
		  ret = cl->call(extent_protocol::getattr, eid, attr);
		  assert(ret == extent_protocol::OK);
		  cached_attrs[eid] = attr;
		  cached_attrs[eid].atime = time(NULL);
	  }
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
  if(cached_attrs.find(eid) != cached_attrs.end())
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
  cached_attrs[eid] = attr;
  dirty_attrs.insert(eid);
  return ret;
}

extent_protocol::status
extent_client::put(extent_protocol::extentid_t eid, std::string buf)
{
  extent_protocol::status ret = extent_protocol::OK;
  cached_extents[eid] = buf;
  if(cached_attrs.find(eid) == cached_attrs.end()){
		  extent_protocol::attr attr;
		  ret = cl->call(extent_protocol::getattr, eid, attr);
		  if(ret == extent_protocol::OK)
			  cached_attrs[eid] = attr;
		  else
			  cached_attrs[eid].atime = time(NULL);
  }
  cached_attrs[eid].size = buf.length();
  cached_attrs[eid].ctime = cached_attrs[eid].mtime = time(NULL);

  dirty_extents.insert(eid);
  dirty_attrs.insert(eid);
  if(removed.find(eid) != removed.end())
	  removed.erase(eid);
  return ret;
}

extent_protocol::status
extent_client::remove(extent_protocol::extentid_t eid)
{
  std::string buf;
  extent_protocol::status ret = extent_protocol::OK;
  ret = cl->call(extent_protocol::get, eid, buf);

  if(ret != extent_protocol::OK && 
		  cached_extents.find(eid) == cached_extents.end())	// not in cache and extents server
	  return extent_protocol::NOENT;

  if(ret == extent_protocol::OK)		//exists in extents server
	  removed.insert(eid);

  cached_extents.erase(eid);
  dirty_extents.erase(eid);
  dirty_attrs.erase(eid);

  return extent_protocol::OK;
}

extent_protocol::status
extent_client::flush(extent_protocol::extentid_t eid)
{
	//putting the corresponding extent (if dirty) to the extent server, or removing the extent (if locally removed) from the extent server. 
	int r;
	extent_protocol::status ret = extent_protocol::OK;
	std::set<extent_protocol::extentid_t>::iterator it;
	if((it=dirty_extents.find(eid)) != dirty_extents.end()){
		ret = cl->call(extent_protocol::put, eid, cached_extents[*it], r);
		assert(ret == extent_protocol::OK);
		dirty_extents.erase(it);
		dirty_attrs.erase(eid);
	}
	else if((it=removed.find(eid)) != removed.end()){
		ret = cl->call(extent_protocol::remove, *it, r);
		assert(ret == extent_protocol::OK);
		removed.erase(it);
	}
	cached_extents.erase(eid);
	cached_attrs.erase(eid);
	return ret;
}
