#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include <iostream>

using namespace std;

/*
   一个磁盘上的list
 */

/*
读取：
先读入一个固定大小的结构，其中包含必要的元信息，从元信息中得到长度信息，再读入。
*/

struct Link{
	size_t node_size;
	off_t offset;	 //offset为0为结尾
};

struct Node{
	char* data;
	Link next;
};

struct Head{
	Link first;
};
static Head head;
size_t head_size = sizeof(head.first.node_size) + sizeof(head.first.offset);
static int fd=-1;

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

char* head_encode(Head* headp)
{
	char* buf = (char*) Malloc(head_size);
	char* ptr = buf;
	memcpy(ptr, &headp->first.node_size, sizeof(headp->first.node_size));	//写入下一个结点的大小信息
	ptr += sizeof(headp->first.node_size);
	memcpy(ptr, &headp->first.offset, sizeof(headp->first.offset));	//写入下一个结点的offset信息
	return buf;
}

void head_decode(char* buf)
{
	char* ptr = buf;
	memcpy(&head.first.node_size, ptr, sizeof(head.first.node_size));	//写入下一个结点的大小信息
	ptr += sizeof(head.first.node_size);
	memcpy(&head.first.offset, ptr, sizeof(head.first.offset));	//写入下一个结点的offset信息
}

//把head写入文件
void write_head()
{
	char* head_rep = head_encode(&head);
	Lseek(fd, 0, SEEK_SET);
	Write(fd, head_rep, head_size);
	free(head_rep);
}

//从文件中读取头部信息
void read_head()
{
	char* buf = (char*)Malloc(head_size);
	Lseek(fd, 0, SEEK_SET);
	Read(fd, buf, head_size);
	head_decode(buf);
	free(buf);
}

bool list_init(char* file)
{
	/* 先看文件是否存在，如果存在就假定它的格式是正确的,读取头部元数据;
	   如果不存在，则是新建文件，按照disc list的办法格式化之
	   */
	if(access(file, F_OK) < 0){
		fd = open(file, O_CREAT|O_RDWR, 0644);
		if(fd<0){
			perror("open");
			exit(1);
		}
		write_head(); //是否要把新建的head写入到文件？？写入。
	}else{
		fd = open(file, O_RDWR);
		if(fd<0){
			perror("open");
			exit(1);
		}
		//读取头结点信息
		read_head();
	}

	return fd<0? false:true;
}

void list_release(){
	close(fd);
	fd = -1;
}

char* node_encode(Node* np, size_t* size)
{
	int data_size = strlen(np->data)+1;
	*size = data_size + sizeof(np->next.node_size) + sizeof(np->next.offset);

	char* buf = (char*) Malloc(*size);
	char* ptr = buf;
	strcpy(ptr, np->data);
	ptr += data_size;
	memcpy(ptr, &np->next.node_size, sizeof(np->next.node_size));	//写入下一个结点的大小信息
	ptr += sizeof(np->next.node_size);
	memcpy(ptr, &np->next.offset, sizeof(np->next.offset));	//写入下一个结点的offset信息
	return buf;
}

Node* node_decode(char* buf)
{
	Node* np = (Node*)Malloc(sizeof(Node));
	np->data = strdup(buf); /* 读取结点中的数据，C style string */

	char* ptr = buf+strlen(np->data)+1;
	memcpy(&np->next.node_size, ptr, sizeof(np->next.node_size));	//读取下一个结点的大小信息
	ptr += sizeof(np->next.node_size);
	memcpy(&np->next.offset, ptr, sizeof(np->next.offset));	//读取下一个结点的offset信息
	return np;
}

Node* read_node(Link lp)
{
	char* buf = (char*)Malloc(lp.node_size);

	Lseek(fd, lp.offset, SEEK_SET);

	Read(fd, buf, lp.node_size); //至此，结点中的内容在buf里
	Node* np = node_decode(buf);
	free(buf);

	return np;
}

void node_free(Node* np)
{
	free(np->data);
	free(np);
}

void list_print()
{
	Link p=head.first;
	printf("current list:\n");
	while(p.offset!=0){
		Node* np = read_node(p);
		printf("%s, ", np->data);
		p=np->next;
		node_free(np);
	}
	printf("\n");
}

bool list_get(char* data)
{
	Link p=head.first;
	while(p.offset!=0){
		Node* np = read_node(p);
		if(!strcmp(np->data, data))	//matched
			return true;
		p=np->next;
		node_free(np);
	}
	return false;
}

void list_insert(const char* data)
{
	Node* np = (Node*)Malloc(sizeof(Node));
	np->data = strdup(data);
	np->next = head.first;
	//write np 文件结尾， 返回写的offset
	size_t size;
	char* node_rep = node_encode(np, &size);
	off_t offset = Lseek(fd, 0, SEEK_END);
	Write(fd, node_rep, size);
	//新的链表第一个结点的offset
	head.first.offset = offset;
	//新的链表第一个结点的大小信息
	head.first.node_size = strlen(np->data)+1; //
	head.first.node_size += sizeof(np->next.node_size); //
	head.first.node_size += sizeof(np->next.offset); //

	write_head();

	node_free(np);
	free(node_rep);
}

/*
void list_remove(char* data)
{
	//Node* np = list_get(data);
}
*/

int main()
{
	string cmd, data;
	if(!list_init("mylist")){
		cerr<<"db open failed:"<<endl;
		perror("open");
	}
	while(1){
		cout<<"please input command:"<<endl;
		cin>>cmd;
		if(cmd == "add"){
			cin>>data;
			list_insert(data.c_str());
		}else if(cmd == "del"){
			cin>>data;
			//list_remove(data.c_str());
		}
		else{
			return 0;
		}
		list_print();
	}

	return 0;
}
