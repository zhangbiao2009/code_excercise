// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
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
yfs_client::getdir(inum inum, dirinfo &din)
{
  int r = OK;

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
yfs_client::create(inum inum)
{
	int r = OK;
	if(ec->put(inum, "") != extent_protocol::OK){
		r = IOERR;
		goto release;
	}

release:
	return r;
}

int 
yfs_client::readdir(inum inum, std::vector<dirent>& dirents)
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

	for(int i=0; i<dirents.size(); i++){
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
