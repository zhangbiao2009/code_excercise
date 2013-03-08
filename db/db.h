#ifndef __DB_H__
#define __DB_H__

#include <map>
#include <string>

using namespace std;

class DList;
class LRUCache;

class Link{
	public:
		Link(size_t ts, size_t s, off_t os);
		Link();
		static void Encode(char* buf, const Link& l);
		static void Decode(Link* lp, char* buf);
		//写入文件
		static void Write(const Link& l, int fd, off_t offset);

		//write in current offset
		static void Write(const Link& l, int fd);

		//从文件中读取
		static void Read(Link* lp, int fd, off_t offset);
		//read in current offset
		static void Read(Link* lp, int fd);

		bool IsNull()const;
		static size_t Size();
		size_t GetSize()const;
		void SetSize(size_t s) ;
		size_t GetTotalSize()const;
		void SetTotalSize(size_t s);
		off_t GetOffset()const;
		void SetOffset(off_t o);

	private:
		size_t total_size_;	//first allocated size
		size_t size_;	//current size
		off_t offset_;	 //offset_为0为结尾
};

class DB{
	public:
		DB();
		~DB();

		bool Open(const string& dbname);
		void Close();
		bool Add(const string& key, const string& val);
		bool Set(const string& key, const string& val);
		bool Del(const string& key);
		bool Get(const string& key, string* val);
		void Print();
		void PrintCache();
		void PrintFreeMap();

	private:
		int hash(const char* str);
		void InitFreeMaps();
		void WriteFreeMaps();

		string dbname_;
		int idx_fd_;
		int data_fd_;
		DList* list_arr_;
		map<size_t, Link> free_node_;	//entry: available space => link
		map<size_t, Link> free_data_;

		LRUCache* cache_;
};

#endif	/*__DB_H__*/
