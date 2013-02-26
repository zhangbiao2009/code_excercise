#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <iostream>
#include <map>

using namespace std;

/*
   一个简单的key value DB
 */

//static const int HASH_TABLE_SIZE = 4993;	//一个素数
static const int HASH_TABLE_SIZE = 2;	//一个素数
static const int MAX_KEY_SIZE = 1024;
static const int MAX_VAL_SIZE = 1024*1024;
const int MAX_CACHE_SIZE = 5;

class LRUCache{
	public:
		struct Node{
			string key;
			string val;
			struct Node* prev;
			struct Node* next;

			static Node* NewNode(const string& key, const string& val)
			{
				Node* np = new Node;
				np->key = key;
				np->val = val;
				np->prev = np->next = np; //point to itself
				return np;
			}
		};
		
		LRUCache()
		{
			head_.prev = head_.next = &head_;
		}
		~LRUCache(){}

		//add or replace
		void Set(const string& key, const string& val)
		{
			Node* node = Find(key);
			if(!node){
				node = Node::NewNode(key, val);
				if(cache_.size() >= MAX_CACHE_SIZE){
					//evict an entry according to LRU policy (from list tail)
					Node* tmp = head_.prev;
					RemoveFromList(tmp);
					cache_.erase(tmp->key);
					delete tmp;
				} 
				//store the address in the cache
				cache_[key] = node;
			}else
				node->val = val; //set the value

			//put it in list front
			Promote(node);
		}

		void Del(const string& key)
		{
			Node* np = Find(key);
			if(np){
				RemoveFromList(np);
				cache_.erase(np->key);
				delete np;
			}
		}

		bool Get(const string& key, string* val)
		{
			Node* np = Find(key);
			if(np){
				//put it in list front
				Promote(np);
				*val = np->val;
				return true;
			}
			return false;
		}

		void Print()
		{
			cout<<"current cache status:"<<endl;
			for(Node* np=head_.next; np!=&head_; np=np->next){
				cout<<"("<<np->key<<", "<<np->val<<") ";
			}
			cout<<endl;
		}

	private:

		Node* Find(const string& key)
		{
			map<string, Node*>::iterator iter= cache_.find(key);
			if(iter == cache_.end())
				return NULL;
			return iter->second;
		}

		void RemoveFromList(Node* node)
		{
			node->prev->next = node->next;
			node->next->prev = node->prev;
		}

		//put it to the front
		void Promote(Node* node)
		{
			if(node->prev != node && node->next != node){
				//not a new node, already in the list, remove it from the list
				RemoveFromList(node);
			}

			//insert node in the front
			node->next = head_.next;
			node->prev = &head_;
			node->prev->next = node;
			node->next->prev = node;
		}

		//circular doubly linked list
		Node head_;
		map<string, Node*> cache_;
};

int Open(const string& pathname, int flags)
{
	int ret;
	if((ret = open(pathname.c_str(), flags)) < 0){
		perror("open");
		exit(1);
	}
	return ret;
}

int Open(const string& pathname, int flags, mode_t mode)
{
	int ret;
	if((ret = open(pathname.c_str(), flags, mode)) < 0){
		perror("open");
		exit(1);
	}
	return ret;
}

int Close(int fd)
{
	int ret;
	if((ret = close(fd)) < 0){
		perror("close");
		exit(1);
	}
	return ret;
}

off_t Lseek(int fd, off_t offset, int whence)
{
	off_t off;
	if((off = lseek(fd, offset, whence)) < 0){
		perror("lseek");
		exit(1);
	}
	return off;
}

ssize_t Read(int fd, void *buf, size_t count)
{
	ssize_t size;
	if((size = read(fd, buf, count)) < 0){
		perror("read");
		exit(1);
	}
	return size;
}

ssize_t Write(int fd, const void *buf, size_t count)
{
	ssize_t size;
	if((size = write(fd, buf, count)) < 0){
		perror("write");
		exit(1);
	}
	return size;
}
int Unlink(const string& pathname)
{
	if(unlink(pathname.c_str())<0){
		perror("unlink");
		exit(1);
	}
}


class Link{
	public:
		Link(size_t ts, size_t s, off_t os)
			:total_size_(ts),size_(s),offset_(os){}
		/*
		Link(size_t ns, off_t os)
			:total_size_(ns),size_(ns),offset_(os){}*/
		Link()
			:total_size_(0),size_(0),offset_(0){}

		static void Encode(char* buf, const Link& l)
		{
			char* ptr = buf;
			memcpy(ptr, &l.total_size_, sizeof(l.total_size_));
			ptr += sizeof(l.total_size_);
			memcpy(ptr, &l.size_, sizeof(l.size_));
			ptr += sizeof(l.size_);
			memcpy(ptr, &l.offset_, sizeof(l.offset_));
		}

		static void Decode(Link* lp, char* buf)
		{
			char* ptr = buf;
			memcpy(&lp->total_size_, ptr, sizeof(lp->total_size_));
			ptr += sizeof(lp->total_size_);
			memcpy(&lp->size_, ptr, sizeof(lp->size_));
			ptr += sizeof(lp->size_);
			memcpy(&lp->offset_, ptr, sizeof(lp->offset_));
		}
		//写入文件
		static void Write(const Link& l, int fd, off_t offset)
		{
			Lseek(fd, offset, SEEK_SET);
			Write(l, fd);
		}

		//write in current offset
		static void Write(const Link& l, int fd)
		{
			char* link_rep = new char[Link::Size()];
			Link::Encode(link_rep, l);
			::Write(fd, link_rep, Link::Size());
			delete[] link_rep;
		}

		//从文件中读取
		static void Read(Link* lp, int fd, off_t offset)
		{
			Lseek(fd, offset, SEEK_SET);
			Read(lp, fd);
		}
		//read in current offset
		static void Read(Link* lp, int fd)
		{
			size_t sz = Link::Size();
			char* buf = new char[sz];
			::Read(fd, buf, sz);
			Link::Decode(lp, buf);
			delete[] buf;
		}

		bool IsNull()const {return offset_ == 0;}

		static size_t Size(){
			return sizeof(size_t) + sizeof(size_t) + sizeof(off_t);
		}
		size_t GetSize()const {return size_;}
		void SetSize(size_t s) {size_ = s;}
		size_t GetTotalSize()const {return total_size_;}
		void SetTotalSize(size_t s) {total_size_ = s;}
		off_t GetOffset()const {return offset_;}
		void SetOffset(off_t o) {offset_ = o;}

	private:
		size_t total_size_;	//first allocated size
		size_t size_;	//current size
		off_t offset_;	 //offset_为0为结尾
};

class Node{
	public:
		Node(bool valid, const string& key, const Link& val_link, const Link& next)
			:valid_(valid), key_(key), val_link_(val_link), next_(next){}
		~Node(){}

		static char* Encode(Node* np)
		{
			const char* key = np->key_.c_str();
			int key_len = strlen(key);
			char* buf = new char[np->Size()];

			char* ptr = buf;
			memcpy(ptr, &np->valid_, sizeof(np->valid_));	//写入结点的有效信息
			ptr += sizeof(np->valid_);
			memcpy(ptr, &key_len, sizeof(key_len));	//写入key长度信息
			ptr += sizeof(key_len);
			memcpy(ptr, key, key_len);	//写入key
			ptr += key_len;
			Link::Encode(ptr, np->val_link_);
			ptr += Link::Size();
			Link::Encode(ptr, np->next_);
			ptr += Link::Size();

			return buf;
		}

		static Node* Decode(char* buf)
		{
			Node* np = new Node;
			char* ptr = buf;
			int key_len;

			memcpy(&np->valid_, ptr, sizeof(np->valid_));	//读取结点的有效信息
			ptr += sizeof(np->valid_);
			memcpy(&key_len, ptr, sizeof(key_len));	//读取key长度信息
			ptr += sizeof(key_len);
			np->key_ = string(ptr, key_len); // 读取key
			ptr += key_len;
			Link::Decode(&np->val_link_, ptr);
			ptr += Link::Size();
			Link::Decode(&np->next_, ptr);
			ptr += Link::Size();

			return np;
		}

		size_t Size()
		{
			size_t sz = sizeof(valid_);
			sz += sizeof(int);	//key长度信息
			sz += strlen(key_.c_str()); //key本身的大小
			sz += Link::Size(); //val_link_
			sz += Link::Size(); //next_
			return sz;
		}

		Link GetNext() { return next_; }
		void SetNext(const Link& l) { next_ = l;};
		bool IsValid(){return valid_;} 
		void SetValid(bool v) {valid_ = v;}
		string GetKey() {return key_;}
		Link GetValLink() { return val_link_; }
		void SetValLink(const Link& l) { val_link_ = l; }

	private:
		Node(){}

		bool valid_;
		string key_;
		Link val_link_;
		Link next_;
};

class DList{
	public:
		DList(){}
		~DList(){}

		void Init(int idx_fd, int data_fd, off_t offset, map<size_t, Link>* free_node, map<size_t, Link>* free_data)
		{
			idx_fd_ = idx_fd;
			data_fd_ = data_fd;
			head_offset_ = offset;
			free_node_ = free_node;
			free_data_ = free_data;
		}

		bool Empty()
		{
			return head_.IsNull();
		}

		void Print()
		{
			Link l=head_;
			while(!l.IsNull()){
				Node* np = NodeRead(l);
				if(np->IsValid())
					cout<<"("<<np->GetKey()<<", "<<GetVal(np->GetValLink())<<") ";
				l=np->GetNext();
				delete np;
			}
			cout<<endl;
		}

		bool Find(const string& key, Node** npp = NULL, Link* lp = NULL, 
				Node** prev_npp = NULL, Link* prev_lp = NULL)
		{
			if(prev_lp) *prev_lp = Link();
			if(prev_npp) *prev_npp = NULL;

			Link l=head_;
			while(!l.IsNull()){
				Node* np = NodeRead(l);
				if(np->IsValid() && np->GetKey() == key){	//matched
					if(npp) *npp = np;
					if(lp) *lp = l;
					return true;
				}
				
				if(prev_npp && *prev_npp)
					delete *prev_npp;

				if(prev_npp) *prev_npp = np;
				if(prev_lp) *prev_lp = l;

				l=np->GetNext();

				if(!prev_npp)
					delete np;
			}

			//Not Found
			if(prev_npp && *prev_npp)
				delete *prev_npp;

			return false;
		}

		bool Get(const string& key, string* val){
			Node* np = NULL;
			bool found = Find(key, &np);
			if(!found) 
				return false;
			*val = GetVal(np->GetValLink());
			delete np;
			return true;
		}

		bool Add(const string& key, const string& val)
		{
			if(Find(key))	//alread exist
				return false;

			off_t offset;
			size_t total_size;

			Link val_link = FindRooMAndWrite(val.c_str(), val.length(), data_fd_, free_data_);

			Node n(true, key, val_link, head_);
			char* node_rep = Node::Encode(&n);

			//新的链表头结点信息
			head_ = FindRooMAndWrite(node_rep, n.Size(), idx_fd_, free_node_);
			HeadWrite();

			delete[] node_rep;
			return true;
		}

		bool Set(const string& key, const string& val)
		{
			//update对应key的val，如果不存在则返回false
			Node* np = NULL;
			Link l;
			bool found = Find(key, &np, &l);
			if(!found) 
				return false;

			Link val_link = np->GetValLink();
			if(val_link.GetTotalSize() >= val.length()){ 	//the old val has enough space, write new val at original offset
				Lseek(data_fd_, val_link.GetOffset(), SEEK_SET);
				val_link.SetSize(val.length());
			}else{ 	//space is not enough, write the new val at the end of file
				(*free_data_)[val_link.GetTotalSize()] = val_link;		//add original data space to free data map
				val_link = FindRooMAndWrite(val.c_str(), val.length(), data_fd_, free_data_);
			}

			Write(data_fd_, val.c_str(), val.length());
			np->SetValLink(val_link);
			NodeWriteInplace(np, l.GetOffset());

			delete np;
			return true;
		}

		bool Del(const string& key)
		{
			Node* np = NULL;
			Node* prev_np = NULL;
			Link prev_l, l;
			bool found = Find(key, &np, &l, &prev_np, &prev_l);
			if(!found) 
				return false;

			if(!prev_np){	
				//the found node is the first node of the list, so update the head
				head_ = np->GetNext();
				HeadWrite();
			}else{	
				//update the link info in prev node
				prev_np->SetNext(np->GetNext());
				NodeWriteInplace(prev_np, prev_l.GetOffset());
			}
			//add removed node info to free node map
			(*free_node_)[l.GetTotalSize()] = l;
			//add removed corresponding data space to free data map
			Link val_link = np->GetValLink();
			(*free_data_)[val_link.GetTotalSize()] = val_link;

			/*
			np->SetValid(false);
			NodeWriteInplace(np, l.offset);
			*/
			delete np;
			if(prev_np)
				delete prev_np;

			return true;
		}

		//把head写入文件
		void HeadWrite() { Link::Write(head_, idx_fd_, head_offset_); }

		//从文件中读取头部信息
		void HeadRead() { Link::Read(&head_, idx_fd_, head_offset_); }

		string GetVal(const Link& l)
		{
			char* buf = new char[l.GetSize()];
			Lseek(data_fd_, l.GetOffset(), SEEK_SET);
			Read(data_fd_, buf, l.GetSize()); //至此，结点中的内容在buf里
			string val(buf, l.GetSize()); // 读取val
			delete[] buf;

			return val;
		}

	private:

		Node* NodeRead(const Link& l)
		{
			char* buf = new char[l.GetSize()];
			Lseek(idx_fd_, l.GetOffset(), SEEK_SET);
			Read(idx_fd_, buf, l.GetSize()); //至此，结点中的内容在buf里
			Node* np = Node::Decode(buf);
			delete[] buf;

			return np;
		}

		void NodeWriteInplace(Node* np, off_t offset)
		{
			char* node_rep = Node::Encode(np);
			Lseek(idx_fd_, offset, SEEK_SET);
			Write(idx_fd_, node_rep, np->Size());
			delete[] node_rep;
		}

		//return link info that describes the write which just happened
		Link FindRooMAndWrite(const char* data, size_t len,  int fd, map<size_t, Link>* free_space)
		{	
			Link l;
			//first find data space which is >= data length in free space map
			map<size_t, Link>::iterator iter = free_space->lower_bound(len);
			if(iter != free_space->end()){  // found proper space in free space map
				l = iter->second;
				Lseek(fd, l.GetOffset(), SEEK_SET);
				free_space->erase(iter);		//remove from free space map because it'll be reused
			}else{
				// no proper space in free space map, write data to the end of the file
				l.SetOffset(Lseek(fd, 0, SEEK_END));
				l.SetTotalSize(len);
				l.SetSize(len);
			}

			Write(fd, data, len);
			return l;
		}

		Link head_;
		off_t head_offset_;
		int idx_fd_;
		int data_fd_;
		map<size_t, Link>* free_node_;
		map<size_t, Link>* free_data_;
};


class DB{
	public:
		DB(){
			list_arr_ = new DList[HASH_TABLE_SIZE];
		}
		~DB() {
			Close();
			delete[] list_arr_;
		}

		bool Open(const string& dbname)
		{
			dbname_ = dbname;
			string idx_file_name = dbname_+".idx";
			string data_file_name = dbname_+".data";

			InitFreeMaps();

			if(access(data_file_name.c_str(), F_OK) < 0){	//file not exist
				idx_fd_ = ::Open(idx_file_name, O_CREAT|O_RDWR, 0644);
				data_fd_ = ::Open(data_file_name, O_CREAT|O_RDWR, 0644);

				for(int i=0; i<HASH_TABLE_SIZE; i++){
					list_arr_[i].Init(idx_fd_, data_fd_, i*Link::Size(), &free_node_, &free_data_);
					list_arr_[i].HeadWrite();
				}
			}else{
				idx_fd_ = ::Open(idx_file_name, O_RDWR);
				data_fd_ = ::Open(data_file_name, O_RDWR);

				//read list heads from file;
				char* buf = new char[Link::Size()];
				for(int i=0; i<HASH_TABLE_SIZE; i++){
					list_arr_[i].Init(idx_fd_, data_fd_, i*Link::Size(), &free_node_, &free_data_);
					list_arr_[i].HeadRead();
				}
				delete[] buf;
			}

			return true;
		}

		void Close(){
			::Close(idx_fd_);
			::Close(data_fd_);
			idx_fd_ = data_fd_ = -1;
			WriteFreeMaps();
		}

		bool Add(const string& key, const string& val)
		{
			//先查找，如果记录已经存在，则返回添加失败，暂不支持修改操作
			//直接加入到data文件最后
			//然后根据hash值，找到对应的位置，更新idx文件
			int h = hash(key.c_str());
			return list_arr_[h].Add(key, val);
		}

		bool Set(const string& key, const string& val)
		{
			cache_.Del(key);
			//update对应key的val，如果不存在则返回false
			int h = hash(key.c_str());
			return list_arr_[h].Set(key, val);
		}

		bool Del(const string& key)
		{
			cache_.Del(key);
			int h = hash(key.c_str());
			return list_arr_[h].Del(key);
		}

		bool Get(const string& key, string* val)
		{
			//返回对应key的val，如果不存在则返回false
			if(cache_.Get(key, val))	//in cache
				return true;
			int h = hash(key.c_str());
			int ret = list_arr_[h].Get(key, val);
			if(ret)
				cache_.Set(key, *val);
			return ret;
		}

		void Print()
		{
			for(int i=0; i<HASH_TABLE_SIZE; i++){
				if(!list_arr_[i].Empty()){
					cout<<"list "<<i<<": ";
					list_arr_[i].Print();
				}
			}
		}
		
		void PrintCache()
		{
			cache_.Print();
		}

		void PrintFreeMap()
		{
			cout<<"free node map: ";
			for (map<size_t, Link>::iterator it=free_node_.begin(); it!=free_node_.end(); ++it)
				cout << "(" << it->first << ", " << it->second.GetTotalSize()<< ", "<<it->second.GetOffset()<< ") ";
			cout<<endl;
			cout<<"free data map: ";
			for (map<size_t, Link>::iterator it=free_data_.begin(); it!=free_data_.end(); ++it)
				cout << "(" << it->first << ", " << it->second.GetTotalSize()<< ", "<<it->second.GetOffset()<< ") ";
			cout<<endl;
		}

	private:
		int hash(const char* str)
		{
			int h = 0;
			for(const char* p = str; *p!= '\0'; p++){
				h = (31*h+*p)%HASH_TABLE_SIZE;
			}
			return h;

		}

		void InitFreeMaps(){
			string free_file_name = dbname_+".free";

			if(access(free_file_name.c_str(), F_OK) < 0)	//file not exist, nothing to do
				return;

			int free_fd = ::Open(free_file_name, O_RDWR);
			::Unlink(free_file_name);		//unlink it
			int n_free_node, n_free_data; 
			::Read(free_fd, &n_free_node, sizeof(n_free_node));
			::Read(free_fd, &n_free_data, sizeof(n_free_data));

			Link l;
			for(int i=0; i<n_free_node; i++){
				Link::Read(&l, free_fd);
				free_node_[l.GetTotalSize()] = l;
			}
			for(int i=0; i<n_free_data; i++){
				Link::Read(&l, free_fd);
				free_data_[l.GetTotalSize()] = l;
			}

			::Close(free_fd);
		}

		void WriteFreeMaps(){
			string free_file_name = dbname_+".free";
			int free_fd = ::Open(free_file_name, O_CREAT|O_RDWR, 0644);
			int n_free_node  = free_node_.size();
			int n_free_data = free_data_.size();

			Write(free_fd, &n_free_node, sizeof(n_free_node));
			Write(free_fd, &n_free_data, sizeof(n_free_data));

			map<size_t, Link>::iterator iter;
			for(iter = free_node_.begin(); iter != free_node_.end(); iter++)
				Link::Write(iter->second, free_fd);
			for(iter = free_data_.begin(); iter != free_data_.end(); iter++)
				Link::Write(iter->second, free_fd);
			::Close(free_fd);
		}

		string dbname_;
		int idx_fd_;
		int data_fd_;
		DList* list_arr_;
		map<size_t, Link> free_node_;	//entry: available space => link
		map<size_t, Link> free_data_;

		LRUCache cache_;
};


int main()
{
	string cmd, key, val;

	DB db;
	if(!db.Open("mydb")){
		cerr<<"db open failed:"<<endl;
		return 0;
	}
	while(1){
		cout<<"please input command:"<<endl;
		cin>>cmd;
		if(cmd == "add"){
			cin>>key>>val;
			if(!db.Add(key, val))
				cout << "already exist" <<endl;
		}else if(cmd == "del"){
			cin>>key;
			db.Del(key);
		}else if(cmd == "show"){
			db.Print();
		}else if(cmd == "get"){
			cin>>key;
			if(db.Get(key, &val))
				cout<<"("<<key<<", "<<val<<")"<<endl;
			else
				cout<<"no such key: "<<key<<endl;
		}else if(cmd == "set"){
			cin>>key>>val;
			if(db.Set(key, val))
				cout<<"("<<key<<", "<<val<<")"<<endl;
			else
				cout<<"no such key: "<<key<<endl;
		}
		else{
			return 0;
		}
		db.PrintCache();
	}

	db.Close();

	return 0;
}
