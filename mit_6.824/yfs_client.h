#ifndef yfs_client_h
#define yfs_client_h

#include <string>
//#include "yfs_protocol.h"
#include "extent_client.h"
#include <vector>
#include <set>


class yfs_client {
  extent_client *ec;
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
 public:

  yfs_client(std::string, std::string);

  bool isfile(inum);
  bool isdir(inum);

  int getfile(inum, fileinfo &);
  int getdir(inum, dirinfo &);

  int setattr(inum inum, struct stat st);
  int create(inum);
  int readdir(inum, std::vector<dirent>&);
  int writedir(inum inum, const std::vector<dirent>& dirents);
  int read(inum, size_t size, off_t off, std::string& buf);
  int write(inum, const char* buf, size_t size, off_t off);
  inum new_file_inum();
  inum new_dir_inum();
};

#endif 
