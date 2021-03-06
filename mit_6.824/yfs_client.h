#ifndef yfs_client_h
#define yfs_client_h

#include <string>
//#include "yfs_protocol.h"
#include "extent_client.h"
#include <vector>
#include <set>

#include "lock_protocol.h"
#include "lock_client.h"

class yfs_client {
  extent_client *ec;
  lock_client *lc;
 public:

  typedef unsigned long long inum;
  enum xxstatus { OK, RPCERR, NOENT, IOERR, EXIST };
  typedef int status;

  struct fileinfo {
    unsigned long long size;
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
  };
  struct dirinfo {
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
  };
  struct dirent {
    std::string name;
    yfs_client::inum inum;
  };

 private:
  static std::string filename(inum);
  static inum n2i(std::string);
  static std::string i2n(inum);
  inum new_uniq_number();
  std::set<inum> used_nums_;
  int readdir_internal(inum, std::vector<dirent>&);
 public:

  yfs_client(std::string, std::string);

  bool isfile(inum);
  bool isdir(inum);

  int getfile(inum, fileinfo &);
  int getdir(inum, dirinfo &);

  int setattr(inum inum, struct stat st);
  int create(inum parent, const char *name, inum&);
  int mkdir(inum parent, const char *name, inum&);
  int readdir(inum, std::vector<dirent>&);
  int writedir(inum inum, const std::vector<dirent>& dirents);
  int read(inum, size_t size, off_t off, std::string& buf);
  int write(inum, const char* buf, size_t size, off_t off);
  int unlink(inum, const char*);
  inum new_file_inum();
  inum new_dir_inum();
};

#endif 
