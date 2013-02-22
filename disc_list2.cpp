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

int Open(const char *pathname, int flags)
{
	int ret;
	if((ret = open(pathname, flags)) < 0){
		perror("open");
		exit(1);
	}
	return ret;
}

int Open(const char *pathname, int flags, mode_t mode)
{
	int ret;
	if((ret = open(pathname, flags, mode)) < 0){
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
	size_t node_size;
	off_t offset;	 //offset为0为结尾

	Link(size_t ns, off_t os)
		:node_size(ns),offset(os){}
	Link()
		:node_size(0),offset(0){}
	bool IsNull() {return offset == 0;}
	static size_t Size(){
		return sizeof(size_t) + sizeof(off_t);
	}
};

class Node{
	public:
		Node(bool valid, const string& data, const Link& next)
			:valid_(valid), data_(data), next_(next){}
		~Node(){}

		static char* Encode(Node* np)
		{
			const char* data = np->data_.c_str();
			int data_len = strlen(data);
			char* buf = new char[np->Size()];

			char* ptr = buf;
			memcpy(ptr, &np->valid_, sizeof(np->valid_));	//写入结点的有效信息
			ptr += sizeof(np->valid_);
			memcpy(ptr, &data_len, sizeof(data_len));	//写入data长度信息
			ptr += sizeof(data_len);
			memcpy(ptr, data, data_len);	//写入data
			ptr += data_len;
			memcpy(ptr, &np->next_.node_size, sizeof(np->next_.node_size));	//写入下一个结点的大小信息
			ptr += sizeof(np->next_.node_size);
			memcpy(ptr, &np->next_.offset, sizeof(np->next_.offset));	//写入下一个结点的offset信息

			return buf;
		}

		static Node* Decode(char* buf)
		{
			Node* np = new Node;
			char* ptr = buf;
			int data_len;

			memcpy(&np->valid_, ptr, sizeof(np->valid_));	//读取结点的有效信息
			ptr += sizeof(np->valid_);
			memcpy(&data_len, ptr, sizeof(data_len));	//读取data长度信息
			ptr += sizeof(data_len);
			np->data_ = string(ptr, data_len); // 读取data
			ptr += data_len;
			memcpy(&np->next_.node_size, ptr, sizeof(np->next_.node_size));	//读取下一个结点的大小信息
			ptr += sizeof(np->next_.node_size);
			memcpy(&np->next_.offset, ptr, sizeof(np->next_.offset));	//读取下一个结点的offset信息
			return np;
		}

		size_t Size()
		{
			size_t sz = sizeof(valid_);
			sz += sizeof(int);	//data长度信息
			sz += strlen(data_.c_str()); //data本身的大小
			sz += sizeof(next_.node_size); //
			sz += sizeof(next_.offset); //
			return sz;
		}

		Link Next() { return next_; }
		bool IsValid(){return valid_;} 
		void SetValid(bool v) {valid_ = v;}
		string GetData() {return data_;}

	private:
		Node(){}

		bool valid_;
		string data_;
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
			memcpy(ptr, &hp->first_.node_size, sizeof(hp->first_.node_size));	//写入下一个结点的大小信息
			ptr += sizeof(hp->first_.node_size);
			memcpy(ptr, &hp->first_.offset, sizeof(hp->first_.offset));	//写入下一个结点的offset信息
			return buf;
		}

		static Header* Decode(char* buf)
		{
			Header* hp = new Header;
			char* ptr = buf;
			memcpy(&hp->first_.node_size, ptr, sizeof(hp->first_.node_size));	//写入下一个结点的大小信息
			ptr += sizeof(hp->first_.node_size);
			memcpy(&hp->first_.offset, ptr, sizeof(hp->first_.offset));	//写入下一个结点的offset信息
			return hp;
		}

		Link GetFirst()
		{
			return first_;
		}

		void SetFirst(Link l){
			first_.node_size = l.node_size;
			first_.offset = l.offset;
		}
	private:
		Link first_;
};

class DiscList{
	public:
		DiscList(const string& file):file_(file){
			hp_ = new Header;
		}
		~DiscList(){
			delete hp_;
		}

		bool Open()
		{
			/* 先看文件是否存在，如果存在就假定它的格式是正确的,读取头部元数据;
			   如果不存在，则是新建文件，按照disc list的办法格式化之
			 */
			const char* file = file_.c_str();
			if(access(file, F_OK) < 0){
				fd_ = ::Open(file, O_CREAT|O_RDWR, 0644);
				HeaderWrite();	//是否要把新建的header写入到文件？？写入。
			}else{
				fd_ = ::Open(file, O_RDWR);
				//读取头部信息
				HeaderRead();
			}

			return true;
		}

		void Close()
		{
			close(fd_);
			fd_ = -1;
		}

		void Print()
		{
			Link p=hp_->GetFirst();
			cout<<"current list:"<<endl;
			while(!p.IsNull()){
				Node* np = NodeRead(p);
				if(np->IsValid())
					cout<<np->GetData()<<", ";
				p=np->Next();
				delete np;
			}
			cout<<endl;
		}

		//copy list to another file, only valid nodes are copied.
		void Copy(DiscList& dest)
		{
			Link p=hp_->GetFirst();
			while(!p.IsNull()){
				Node* np = NodeRead(p);
				if(np->IsValid())
					dest.Add(np->GetData());
				p=np->Next();
				delete np;
			}
		}

		/*
		   bool list_get(char* data)
		   {
		   Link p=head.first;
		   while(p.offset!=0){
		   Node* np = node_read(p);
		   if(np->valid_ && !strcmp(np->data, data))	//matched
		   return true;
		   p=np->next;
		   node_free(np);
		   }
		   return false;
		   }
		 */

		void Add(const string& data)
		{
			Node n(true, data, hp_->GetFirst());
			char* node_rep = Node::Encode(&n);
			//在文件结尾写入新的结点，并更新头结点信息
			off_t offset = Lseek(fd_, 0, SEEK_END);
			Write(fd_, node_rep, n.Size());
			//新的链表头结点信息
			hp_->SetFirst(Link(n.Size(), offset));
			HeaderWrite();

			delete node_rep;
		}

		bool Del(const string& data)
		{
			Link p=hp_->GetFirst();
			while(!p.IsNull()){
				Node* np = NodeRead(p);
				if(np->IsValid() && np->GetData() == data){	//matched
					np->SetValid(false);
					NodeWriteInplace(np, p.offset);
					return true;
				}
				p=np->Next();
				delete np;
			}
			return false;
		}

	private:

		Node* NodeRead(Link lp)
		{
			char* buf = new char[lp.node_size];

			Lseek(fd_, lp.offset, SEEK_SET);

			Read(fd_, buf, lp.node_size); //至此，结点中的内容在buf里
			Node* np = Node::Decode(buf);
			delete[] buf;

			return np;
		}

		//把header写入文件
		void HeaderWrite()
		{
			char* header_rep = Header::Encode(hp_);
			Lseek(fd_, 0, SEEK_SET);
			Write(fd_, header_rep, Header::Size());
			free(header_rep);
		}

		//从文件中读取头部信息
		void HeaderRead()
		{
			size_t sz = Header::Size();
			char* buf = new char[sz];
			Lseek(fd_, 0, SEEK_SET);
			Read(fd_, buf, sz);
			hp_ = Header::Decode(buf);
			
			delete[] buf;
		}

		void NodeWriteInplace(Node* np, off_t offset)
		{
			char* node_rep = Node::Encode(np);
			Lseek(fd_, offset, SEEK_SET);
			Write(fd_, node_rep, np->Size());
			delete node_rep;
		}

		Header* hp_;
		string file_;
		int fd_;
};


int main()
{
	string cmd, data;
	DiscList l("mylist"), dest("mylist.new");

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
			cin>>data;
			l.Add(data);
		}else if(cmd == "del"){
			cin>>data;
			l.Del(data);
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
