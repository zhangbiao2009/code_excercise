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
	attrs_[id].size = buf.length();
	attrs_[id].ctime = attrs_[id].mtime = time(NULL);
	return extent_protocol::OK;
}

int extent_server::get(extent_protocol::extentid_t id, std::string &buf)
{
	ScopedLock l(&m_);
	std::map<extent_protocol::extentid_t, std::string>::iterator it;
	if((it=extents_.find(id)) == extents_.end())
		return extent_protocol::NOENT;
	buf = it->second.substr(0, attrs_[id].size);
	attrs_[id].atime = time(NULL);
	return extent_protocol::OK;
}

int extent_server::getattr(extent_protocol::extentid_t id, extent_protocol::attr &a)
{
	ScopedLock l(&m_);
	a = attrs_[id];
	return extent_protocol::OK;
}

int extent_server::setattr(extent_protocol::extentid_t id, extent_protocol::attr a, int &)
{
	ScopedLock l(&m_);
	if(a.size > attrs_[id].size){	//bigger than orignial size, need to pad null bytes
		extents_[id].append(a.size - attrs_[id].size, '\0');
	}
	attrs_[id].size = a.size;	//only handle size attr
	attrs_[id].ctime = attrs_[id].mtime = time(NULL);
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

