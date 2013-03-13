/*
   oubputfile format:
   nnodes, node array, nbits, bit array
 */

#include "huffman.h"
#include "utils.h"
#include <algorithm>
#include <cstring>
#include <cassert>
#include <iostream>

#define COMPOUND -1
#define BUF_SZ 4096

struct Node{
	Node(int c, int cnt, int l=-1, int r=-1)
		:letter(c), count(cnt), left(l), right(r) {}
	Node(){}

	static void Serialize(char* buf, const Node* np)
	{
		char* ptr = buf;
		memcpy(ptr, &np->letter, sizeof(np->letter));
		ptr += sizeof(np->letter);
		memcpy(ptr, &np->count, sizeof(np->count));
		ptr += sizeof(np->count);
		memcpy(ptr, &np->parent, sizeof(np->parent));
		ptr += sizeof(np->parent);
		memcpy(ptr, &np->left, sizeof(np->left));
		ptr += sizeof(np->left);
		memcpy(ptr, &np->right, sizeof(np->right));
		ptr += sizeof(np->right);
	}

	static void Unserialize(Node* np, char* buf)
	{
		char* ptr = buf;
		memcpy(&np->letter, ptr, sizeof(np->letter));
		ptr += sizeof(np->letter);
		memcpy(&np->count, ptr, sizeof(np->count));
		ptr += sizeof(np->count);
		memcpy(&np->parent, ptr, sizeof(np->parent));
		ptr += sizeof(np->parent);
		memcpy(&np->left, ptr, sizeof(np->left));
		ptr += sizeof(np->left);
		memcpy(&np->right, ptr, sizeof(np->right));
		ptr += sizeof(np->right);
	}

	static int Size(){
		return 5*sizeof(int);
	}

	int letter;		//letter>0 stands for a letter, letter<0 stands for a compound node
	int count;	//use count for frequency
	int parent;
	int left;	//left child;
	int right;
};


class BitVec{
	public:
		BitVec():bit_idx_(0) {
			memset(vec_, 0, BUF_SZ);
		}
		bool Full(){
			return bit_idx_/8 >= BUF_SZ;
		}
		void Add(char bit){
			int i = bit_idx_/8;
			int bits = bit_idx_%8;
			if(bit == 1)
				vec_[i] |= 1<<bits;
			bit_idx_++;
		}
		int Output(int fd){
			int i = bit_idx_/8;
			int bits = bit_idx_%8;
			if(bits > 0) i++;
			if(i>0)
				Write(fd, vec_, i*sizeof(char));
			int ret = bit_idx_;
			bit_idx_ = 0;	//reset for next use
			memset(vec_, 0, BUF_SZ);
			return ret;
		}
	private:
		int bit_idx_;
		char vec_[BUF_SZ];
};

class BitStream{
	public:
		BitStream(int infd, int nbits)
			:infd_(infd), remain_bits_(nbits), curr_idx_(0), end_idx_(0){}

		char Get(){
			if(curr_idx_ == end_idx_){	// buf is empty, read from the file
				int bytesleft = remain_bits_/8;
				if((remain_bits_%8) > 0)
					bytesleft++;
				int nbytes = bytesleft<BUF_SZ? bytesleft:BUF_SZ;
				Read(infd_, buf_, nbytes);
				curr_idx_ = 0;	//reset
				end_idx_ = nbytes*8;	//not accurate, but doesn't matter if used Get() properly
			}

			int i = curr_idx_/8;
			int bits = curr_idx_%8;
			curr_idx_++;
			remain_bits_--;
			return (buf_[i] & 1<<bits) ? 1 : 0;
		}

		bool Empty(){ return remain_bits_ <= 0; }
	private:
		int infd_;
		int remain_bits_;
		char buf_[BUF_SZ];
		int curr_idx_;	//next bit in buf for get
		int end_idx_;	//end idx in buf
};

class HeapCmp{
	public:
		HeapCmp(const vector<Node*>& v):vec(v){}
		bool operator()(int a, int b){
			return vec[a]->count > vec[b]->count;
		}
	private:
		const vector<Node*>& vec;
};

void HuffmanEncoder::BuildTree()
{
	map<char, int>::iterator it;
	for(it = letter_count_.begin(); it!= letter_count_.end(); it++){
		Node* np = new Node(it->first, it->second);
		node_vec_.push_back(np);
		int new_idx = node_vec_.size()-1;
		np->parent = new_idx;	//root points to itself
	}
	for(int i=0; i<node_vec_.size(); i++){
		node_idx_[node_vec_[i]->letter] = i;
		min_heap_.push_back(i);
	}

	HeapCmp heap_cmp(node_vec_);
	make_heap(min_heap_.begin(), min_heap_.end(), heap_cmp);


	while(min_heap_.size()>1){
		pop_heap(min_heap_.begin(), min_heap_.end(), heap_cmp);
		int min1 = min_heap_.back();
		min_heap_.pop_back();
		pop_heap(min_heap_.begin(), min_heap_.end(), heap_cmp);
		int min2 = min_heap_.back();
		min_heap_.pop_back();

		int new_cnt = node_vec_[min1]->count+node_vec_[min2]->count;
		Node* np = new Node(COMPOUND, new_cnt, min1, min2);
		node_vec_.push_back(np);
		int new_idx = node_vec_.size()-1;
		np->parent = new_idx;	//root points to itself
		node_vec_[min1]->parent = node_vec_[min2]->parent = new_idx;
		min_heap_.push_back(new_idx);
		push_heap(min_heap_.begin(), min_heap_.end(), heap_cmp);
	}
}

void HuffmanEncoder::OutputTree(int outfd)
{
	int nnodes = node_vec_.size();
	Write(outfd, &nnodes, sizeof(nnodes));	//nnodes
	char* buf = new char[nnodes*Node::Size()];
	char* ptr = buf;
	for(int i=0; i<node_vec_.size(); i++){
		Node::Serialize(ptr, node_vec_[i]);
		ptr += Node::Size();
	}
	Write(outfd, buf, nnodes*Node::Size());	//all the nodes are written
}

int HuffmanEncoder::LettersEncodeAndOutput(FILE* infp, int outfd)
{
	BitVec bitvec;
	int nbits = 0;
	int c;

	rewind(infp);	//read from beginning
	while((c=fgetc(infp)) != EOF){
		vector<char> code_vec;
		int i = node_idx_[c];
		while(node_vec_[i]->parent != i){
			int parent = node_vec_[i]->parent;
			if(i == node_vec_[parent]->left)	//left child, output 0
				code_vec.push_back(0);
			else		//output 1
				code_vec.push_back(1);
			i = parent;
		}
		//
		while(!code_vec.empty()){
			char b = code_vec.back();
			code_vec.pop_back();
			bitvec.Add(b);
			if(bitvec.Full())
				nbits += bitvec.Output(outfd);
		}
	}

	nbits += bitvec.Output(outfd);	//output the remaining bits if any
	return nbits;
}

off_t HuffmanEncoder::ReserveRoomInt(int fd)
{
	off_t offset = Lseek(fd, 0, SEEK_END);
	int i = 0;
	Write(fd, &i, sizeof(i));
	return offset;
}

bool HuffmanEncoder::Encode(const char* infile, const char* outfile)
{
	FILE* fp = fopen(infile, "r");
	if(!fp)
		return false;
	//count letter frequency
	int c;
	while((c=fgetc(fp)) != EOF)
		letter_count_[c]++;

	BuildTree();
	//min_heap_.size() must be 1 here, and min_heap_[0] is the root of the tree
	assert(min_heap_.size() == 1);

	//then, encode and output
	int fd = Open(outfile, O_CREAT|O_WRONLY|O_TRUNC, 0644);
	//store the tree in output file
	OutputTree(fd);
	//read the file again to encode
	int offset = ReserveRoomInt(fd); //pad zeros for a int
	int nbits = LettersEncodeAndOutput(fp, fd);

	Lseek(fd, offset, SEEK_SET);	//write nbits in the reserved room
	Write(fd, &nbits, sizeof(nbits));

	fclose(fp);
	Close(fd);
}

void HuffmanDecoder::ReadTree(int fd)
{
	int nnodes;
	Read(fd, &nnodes, sizeof(nnodes));
	char* buf = new char[nnodes*Node::Size()];
	Read(fd, buf, nnodes*Node::Size());
	char* ptr = buf;
	for(int i=0; i<nnodes; i++){
		Node* np = new Node;
		Node::Unserialize(np, ptr);
		node_vec_.push_back(np);
		ptr += Node::Size();
	}
}

void HuffmanDecoder::DecodeAndOutput(int infd, FILE* outfp)
{
	int nbits;
	Read(infd, &nbits, sizeof(nbits));
	BitStream bit_stream(infd, nbits);

	int root = node_vec_.size()-1;		//tree root
	while(!bit_stream.Empty()){
		int curr = root;
		while(node_vec_[curr]->letter == COMPOUND){
			char bit = bit_stream.Get();
			curr = (bit == 0) ? node_vec_[curr]->left : node_vec_[curr]->right;
		}
		//arrive at a leaf node, output the charcter
		fprintf(outfp, "%c", node_vec_[curr]->letter);
	}
}

bool HuffmanDecoder::Decode(const char* infile, const char* outfile)
{
	int infd = Open(infile, O_RDONLY);
	FILE* outfp = fopen(outfile, "w");
	ReadTree(infd);

	DecodeAndOutput(infd, outfp);

	Close(infd);
	fclose(outfp);
}
