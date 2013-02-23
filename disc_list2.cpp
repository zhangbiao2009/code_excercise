#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <string>
#include <iostream>

using namespace std;

/*
   一个磁盘上的list
 */

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

void *Malloc(size_t size)
{
	void * ptr = malloc(size);
	if(!ptr){
		fprintf(stderr, "not enough memory\n");
		exit(1);
	}
	return ptr;
}

struct Link{
	size_t size;
	off_t offset;	 //offset为0为结尾

	Link(size_t ns, off_t os)
		:size(ns),offset(os){}
	Link()
		:size(0),offset(0){}

	static void Encode(char* buf, Link l)
	{
		char* ptr = buf;
		memcpy(ptr, &l.size, sizeof(l.size));	//写入下一个结点的大小信息
		ptr += sizeof(l.size);
		memcpy(ptr, &l.offset, sizeof(l.offset));	//写入下一个结点的offset信息
	}

	static void Decode(Link* lp, char* buf)
	{
		char* ptr = buf;
		memcpy(&lp->size, ptr, sizeof(lp->size));	//读取下一个结点的大小信息
		ptr += sizeof(lp->size);
		memcpy(&lp->offset, ptr, sizeof(lp->offset));	//读取下一个结点的offset信息
	}

	bool IsNull() {return offset == 0;}
	static size_t Size(){
		return sizeof(size_t) + sizeof(off_t);
	}
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
		void SetNext(Link l) { next_ = l;};
		bool IsValid(){return valid_;} 
		void SetValid(bool v) {valid_ = v;}
		string GetKey() {return key_;}
		Link GetValLink() { return val_link_; }

	private:
		Node(){}

		bool valid_;
		string key_;
		Link val_link_;
		Link next_;
};

class Header{
	public:
		Header(){}
		~Header(){}

		static size_t Size() { return Link::Size(); }

		static char* Encode(Header* hp)
		{
			char* buf = new char[Size()];
			char* ptr = buf;
			memcpy(ptr, &hp->first_.size, sizeof(hp->first_.size));	//写入下一个结点的大小信息
			ptr += sizeof(hp->first_.size);
			memcpy(ptr, &hp->first_.offset, sizeof(hp->first_.offset));	//写入下一个结点的offset信息
			return buf;
		}

		static Header* Decode(Header* hp, char* buf)
		{
			char* ptr = buf;
			memcpy(&hp->first_.size, ptr, sizeof(hp->first_.size));	//写入下一个结点的大小信息
			ptr += sizeof(hp->first_.size);
			memcpy(&hp->first_.offset, ptr, sizeof(hp->first_.offset));	//写入下一个结点的offset信息
			return hp;
		}

		Link GetFirst()
		{
			return first_;
		}

		void SetFirst(Link l){
			first_.size = l.size;
			first_.offset = l.offset;
		}
	private:
		Link first_;
};

class DiscList{
	public:
		DiscList(const string& name):name_(name){}
		DiscList(){}
		~DiscList(){}

		bool Open()
		{
			/* 先看文件是否存在，如果存在就假定它的格式是正确的,读取头部元数据;
			   如果不存在，则是新建文件，按照disc list的办法格式化之
			 */
			string idx_file_name = name_+".idx";
			string data_file_name = name_+".data";

			if(access(data_file_name.c_str(), F_OK) < 0){
				idx_fd_ = ::Open(idx_file_name, O_CREAT|O_RDWR, 0644);
				data_fd_ = ::Open(data_file_name, O_CREAT|O_RDWR, 0644);
				HeaderWrite();	//是否要把新建的header写入到文件？？写入。
			}else{
				idx_fd_ = ::Open(idx_file_name, O_RDWR);
				data_fd_ = ::Open(data_file_name, O_RDWR);
				//读取头部信息
				HeaderRead();
			}

			return true;
		}

		void Init(int fd, off_t offset)
		{
			idx_fd_ = fd;
			header_offset_ = offset;
		}

		void Close()
		{
			close(idx_fd_);
			close(data_fd_);
			idx_fd_ = data_fd_ = -1;
		}

		void Print()
		{
			Link p=header_.GetFirst();
			cout<<"current list:"<<endl;
			while(!p.IsNull()){
				Node* np = NodeRead(p);
				if(np->IsValid())
					cout<<"("<<np->GetKey()<<", "<<GetVal(np->GetValLink())<<") ";
				p=np->GetNext();
				delete np;
			}
			cout<<endl;
		}

		//copy list to another file, only valid nodes are copied.
		/*
		void Copy(DiscList& dest)
		{
			Link p=header_.GetFirst();
			while(!p.IsNull()){
				Node* np = NodeRead(p);
				if(np->IsValid())
					dest.Add(np->GetKey());
				p=np->GetNext();
				delete np;
			}
		}*/

		bool Find(const string& key, Node** npp = NULL, Link* lp = NULL, 
				Node** prev_npp = NULL, Link* prev_lp = NULL)
		{
			if(prev_lp) *prev_lp = Link(0, 0);
			if(prev_npp) *prev_npp = NULL;

			Link l=header_.GetFirst();
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

		bool Add(const string& key, const string& val)
		{
			if(Find(key))	//alread exist
				return false;

			off_t offset;

			//write data to the end of data file
			offset = Lseek(data_fd_, 0, SEEK_END);
			Write(data_fd_, val.c_str(), val.length());
			Link val_link(val.length(), offset);

			Node n(true, key, val_link, header_.GetFirst());
			char* node_rep = Node::Encode(&n);
			//在文件结尾写入新的结点，并更新头结点信息
			offset = Lseek(idx_fd_, 0, SEEK_END);
			Write(idx_fd_, node_rep, n.Size());
			//新的链表头结点信息
			header_.SetFirst(Link(n.Size(), offset));
			HeaderWrite();

			delete[] node_rep;
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
				//the found node is the first node of the list, so update the link info in header
				header_.SetFirst(np->GetNext());
				HeaderWrite();
			}else{	
				//update the link info in prev node
				prev_np->SetNext(np->GetNext());
				NodeWriteInplace(prev_np, prev_l.offset);
			}

			/*
			np->SetValid(false);
			NodeWriteInplace(np, l.offset);
			*/
			delete np;
			if(prev_np)
				delete prev_np;

			return true;
		}

		string GetVal(Link l)
		{
			char* buf = new char[l.size];
			Lseek(data_fd_, l.offset, SEEK_SET);
			Read(data_fd_, buf, l.size); //至此，结点中的内容在buf里
			string val(buf, l.size); // 读取val
			delete[] buf;

			return val;
		}

	private:

		Node* NodeRead(Link l)
		{
			char* buf = new char[l.size];
			Lseek(idx_fd_, l.offset, SEEK_SET);
			Read(idx_fd_, buf, l.size); //至此，结点中的内容在buf里
			Node* np = Node::Decode(buf);
			delete[] buf;

			return np;
		}

		//把header写入文件
		void HeaderWrite()
		{
			char* header_rep = Header::Encode(&header_);
			Lseek(idx_fd_, 0, SEEK_SET);
			Write(idx_fd_, header_rep, Header::Size());
			delete[] header_rep;
		}

		//从文件中读取头部信息
		void HeaderRead()
		{
			size_t sz = Header::Size();
			char* buf = new char[sz];
			Lseek(idx_fd_, 0, SEEK_SET);
			Read(idx_fd_, buf, sz);
			Header::Decode(&header_, buf);
			
			delete[] buf;
		}

		void NodeWriteInplace(Node* np, off_t offset)
		{
			char* node_rep = Node::Encode(np);
			Lseek(idx_fd_, offset, SEEK_SET);
			Write(idx_fd_, node_rep, np->Size());
			delete[] node_rep;
		}

		Header header_;
		off_t header_offset_;
		string name_;
		int idx_fd_;
		int data_fd_;
};


int main()
{
	string cmd, key, val;
	DiscList l("mylist");

	if(!l.Open()){
		cerr<<"db open failed:"<<endl;
	}

	/*
	dest.Open();
	l.Copy(dest);
	dest.Close();
	return 0;
	*/

	while(1){
		cout<<"please input command:"<<endl;
		cin>>cmd;
		if(cmd == "add"){
			cin>>key>>val;
			if(!l.Add(key, val))
				cout << "already exist" <<endl;
		}else if(cmd == "del"){
			cin>>key;
			l.Del(key);
		}else if(cmd == "show"){
		}
		else{
			return 0;
		}
		l.Print();
	}

	l.Close();

	return 0;
}
