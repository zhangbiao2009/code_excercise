#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

#include <iostream>

using namespace std;

/*
   一个简单的key value DB
 */

/*
读取：
先读入一个固定大小的结构，其中包含必要的元信息，从元信息中得到长度信息，再读入。
*/

static const int HASH_TABLE_SIZE = 4993;	//一个素数
static const int MAX_KEY_SIZE = 1024;
static const int MAX_VAL_SIZE = 1024*1024;

struct hash_entry{
	int entry_size;
	off_t record_offset;	//record offset in data file
	size_t record_size;	//record size
	bool deleted;			//这条记录是正在使用，还是已经被标记为删除？
	char* key;
};

class DB{
	public:
		DB(){}
		~DB() {
			Close();
		}
		bool Open(char* dbname){
			fd_ = open(dbname, O_CREAT|O_RDWR, 0644);
			return fd_<0? false:true;
			
		}
		void Close(){
			close(fd_);
		}
		void Add(char* key, char* val){
			//先查找，如果记录已经存在，则返回添加失败，暂不支持修改操作
			//直接加入到data文件最后
			//然后根据hash值，找到对应的位置，更新idx文件
		}
		void Del(char* key){
			//先查找，如果记录不存在则直接返回失败，否则mark为deleted，不做记录移动
		}
		char* Get(char* key)const{
			//返回对应key的val，如果不存在则返回NULL
			return NULL;
		}
	private:
		int fd_;
};

int main()
{
	DB db;
	if(!db.Open("my.db")){
		cerr<<"db open failed:"<<endl;
		perror("open");
	}

	return 0;
}
