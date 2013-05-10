// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
#include "lock_client_cache.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <boost/tokenizer.hpp>

#define FILE_BASE 0x80000000

yfs_client::yfs_client(std::string extent_dst, std::string lock_dst)
{
  ec = new extent_client(extent_dst);
  lc = new lock_client_cache(lock_dst);
}

yfs_client::inum
yfs_client::n2i(std::string n)
{
  std::istringstream ist(n);
  unsigned long long finum;
  ist >> finum;
  return finum;
}

std::string 
yfs_client::i2n(yfs_client::inum inum)
{
	std::ostringstream stream;
	stream << inum;
	return stream.str();
}

std::string
yfs_client::filename(inum inum)
{
  std::ostringstream ost;
  ost << inum;
  return ost.str();
}

bool
yfs_client::isfile(inum inum)
{
  if(inum & FILE_BASE)
    return true;
  return false;
}

bool
yfs_client::isdir(inum inum)
{
  return ! isfile(inum);
}

int
yfs_client::getfile(inum inum, fileinfo &fin)
{
  int r = OK;
  // You modify this function for Lab 3
  // - hold and release the file lock

  printf("getfile %016llx\n", inum);
  extent_protocol::attr a;
  if (ec->getattr(inum, a) != extent_protocol::OK) {
    r = IOERR;
    goto release;
  }

  fin.atime = a.atime;
  fin.mtime = a.mtime;
  fin.ctime = a.ctime;
  fin.size = a.size;
  printf("getfile %016llx -> sz %llu\n", inum, fin.size);

 release:
  return r;
}

int
yfs_client::setattr(inum inum, struct stat st)
{
  int r = OK;
  extent_protocol::attr a;
  a.size = st.st_size;
  a.atime = st.st_atime;
  a.mtime = st.st_mtime;
  a.ctime = st.st_ctime;
  
  if (ec->setattr(inum, a) != extent_protocol::OK){
    r = IOERR;
  }
  return r;
}


int
yfs_client::getdir(inum inum, dirinfo &din)
{
  int r = OK;
  // You modify this function for Lab 3
  // - hold and release the directory lock

  printf("getdir %016llx\n", inum);
  extent_protocol::attr a;
  if (ec->getattr(inum, a) != extent_protocol::OK) {
    r = IOERR;
    goto release;
  }
  din.atime = a.atime;
  din.mtime = a.mtime;
  din.ctime = a.ctime;

 release:
  return r;
}

//new unique number must less than FILE_BASE, greater than 1
//note: current implementation is defective, only correct when using one yfs_client
yfs_client::inum 
yfs_client::new_uniq_number()
{
	yfs_client::inum remain;
	srand(getpid());
	do {
		remain = rand()%FILE_BASE;
	}while(remain<=1 || used_nums_.find(remain) != used_nums_.end()); //only use remain greater than 1
	used_nums_.insert(remain);
	return remain;
}

yfs_client::inum 
yfs_client::new_file_inum()
{ return FILE_BASE + new_uniq_number(); }

yfs_client::inum 
yfs_client::new_dir_inum()
{ return new_uniq_number(); }

int
yfs_client::create(inum parent, const char *name, inum& ino)
{
	int r = OK;
	lc->acquire(parent);
	ino = new_file_inum();
	lc->acquire(ino);
	std::vector<dirent> dirents;
	dirent d;
	//check existence
	r = readdir_internal(parent, dirents);
	if(r != OK)
		goto release;

	for(std::vector<dirent>::size_type i=0; i<dirents.size(); i++)
		if(std::string(name) == dirents[i].name){
			r = EXIST;
			goto release;
		}

	if(ec->put(ino, "") != extent_protocol::OK){
		r = IOERR;
		goto release;
	}
	//add <name, ino> into @parent
	d.name = name;
	d.inum = ino;
	dirents.push_back(d);
	r = writedir(parent, dirents);
release:
	lc->release(parent);
	lc->release(ino);
	return r;
}

int
yfs_client::unlink(inum parent, const char *name)
{
	int r = OK;
	lc->acquire(parent);
	//check existence
	std::vector<yfs_client::dirent> dirents;
	if(readdir_internal(parent, dirents) != OK){
		r = NOENT;
		lc->release(parent);
		return r;
	}
	std::vector<dirent>::size_type i;
	for(i=0; i<dirents.size(); i++)
		if(std::string(name) == dirents[i].name)
			break;

	lc->acquire(dirents[i].inum);
	if(i == dirents.size() || isdir(dirents[i].inum)){ //not found or is a directory
		r = NOENT;
		goto release;
	}
	if(ec->remove(dirents[i].inum) != extent_protocol::OK){
		r = IOERR;
		goto release;
	}

	//remove <name, ino> in @parent
	dirents.erase(dirents.begin()+i);

	if(writedir(parent, dirents)!= OK)
		r = IOERR;
release:
	lc->release(parent);
	lc->release(dirents[i].inum);
	return r;
}

int
yfs_client::mkdir(inum parent, const char *name, inum& ino)
{
	int r = OK;
	lc->acquire(parent);
	//check existence
	std::vector<yfs_client::dirent> dirents;
	dirent d;
	if(readdir_internal(parent, dirents) != OK){
		r = NOENT;
		lc->release(parent);
		return r;
	}

	for(std::vector<dirent>::size_type i=0; i<dirents.size(); i++)
		if(std::string(name) == dirents[i].name){
			r = EXIST;
			lc->release(parent);
			return r;
		}

	//call yfs_client to create a directory 
	ino = new_dir_inum();
	lc->acquire(ino);

	if(ec->put(ino, "") != extent_protocol::OK){
		r = IOERR;
		goto release;
	}

	//add <name, ino> into @parent
	d.name = name;
	d.inum = ino;
	dirents.push_back(d);
	if(writedir(parent, dirents)!= OK)
		r = IOERR;

release:
	lc->release(parent);
	lc->release(ino);
	return r;
}

int 
yfs_client::readdir(inum inum, std::vector<dirent>& dirents)
{
	int r = OK;
	lc->acquire(inum);
	r = readdir_internal(inum, dirents);
	lc->release(inum);
	return r;
}

int 
yfs_client::readdir_internal(inum inum, std::vector<dirent>& dirents)
{
	std::string buf;
	if(ec->get(inum, buf)!= extent_protocol::OK){
		return IOERR;
	}
	//parse buf
	typedef boost::tokenizer<boost::char_separator<char> > tokenizer;

	boost::char_separator<char> sep(";");
    tokenizer tokens(buf, sep);
    for (tokenizer::iterator tok_iter = tokens.begin();tok_iter != tokens.end(); ++tok_iter) {
        unsigned pos = tok_iter->find(":");
        assert (pos!=std::string::npos);
		dirent d;
		d.name = tok_iter->substr(0, pos);
		d.inum = n2i(tok_iter->substr(pos+1));
		dirents.push_back(d);
    }

	return OK;
}

int 
yfs_client::writedir(inum inum, const std::vector<dirent>& dirents)
{
	int r = OK;
	std::string buf;

	for(std::vector<dirent>::size_type i=0; i<dirents.size(); i++){
		std::string estr = dirents[i].name + ":" + i2n(dirents[i].inum);
		if(i+1<dirents.size()) //not the last entry
			estr.append(";");
		buf.append(estr);
	}

	if(ec->put(inum, buf)!= extent_protocol::OK){
		r = IOERR;
	}

	return r;
}

int
yfs_client::read(inum inum, size_t size, off_t off, std::string& buf)
{
	lc->acquire(inum);
	std::string tmpbuf;
	if(ec->get(inum, tmpbuf)!= extent_protocol::OK){
		lc->release(inum);
		return IOERR;
	}
	buf = tmpbuf.substr(off, size);
	lc->release(inum);
	return OK;
}

int
yfs_client::write(inum inum, const char* buf, size_t size, off_t off)
{
	int r = OK;
	lc->acquire(inum);
	std::string tmpbuf; //orignal content

	printf("called write on: %016llx\n", inum);
	if(ec->get(inum, tmpbuf)!= extent_protocol::OK){
		r = IOERR;
		goto release;
	}
	if(off <= tmpbuf.length())
		tmpbuf.replace(off, size, buf, size);
	else{
		tmpbuf.append(off-tmpbuf.length(), '\0');	//append null bytes
		tmpbuf.append(buf, size);
	}

	if(ec->put(inum, tmpbuf)!= extent_protocol::OK){
		r = IOERR;
		goto release;
	}

	extent_protocol::attr a;
	a.size = tmpbuf.length();
	if (ec->setattr(inum, a) != extent_protocol::OK){
		r = IOERR;
	}
release:
	lc->release(inum);
	return r;
}

