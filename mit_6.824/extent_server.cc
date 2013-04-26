// the extent server implementation

#include "extent_server.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

extent_server::extent_server() 
{
	VERIFY(pthread_mutex_init(&m_, 0) == 0);
	extents_[1] = "";		//according to fuse, the inum for the root directory is 0x00000001
}

extent_server::~extent_server()
{
	VERIFY(pthread_mutex_destroy(&m_) == 0);
}

int extent_server::put(extent_protocol::extentid_t id, std::string buf, int &)
{
	ScopedLock l(&m_);
	extents_[id] = buf;
	return extent_protocol::OK;
}

int extent_server::get(extent_protocol::extentid_t id, std::string &buf)
{
	ScopedLock l(&m_);
	std::map<extent_protocol::extentid_t, std::string>::iterator it;
	if((it=extents_.find(id)) == extents_.end())
		return extent_protocol::NOENT;
	buf = it->second;
	return extent_protocol::OK;
}

int extent_server::getattr(extent_protocol::extentid_t id, extent_protocol::attr &a)
{
	// You fill this in for Lab 2.
	// You replace this with a real implementation. We send a phony response
	// for now because it's difficult to get FUSE to do anything (including
	// unmount) if getattr fails.
	a.size = 0;
	a.atime = 0;
	a.mtime = 0;
	a.ctime = 0;
	return extent_protocol::OK;
}

int extent_server::remove(extent_protocol::extentid_t id, int &)
{
	ScopedLock l(&m_);
	std::map<extent_protocol::extentid_t, std::string>::iterator it;
	if((it=extents_.find(id)) == extents_.end())
		return extent_protocol::NOENT;
	extents_.erase(it);
	return extent_protocol::OK;
}

