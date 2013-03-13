#include <sys/types.h>
#include <unistd.h>
#include <map>
#include <vector>
#include <cstdio>

using namespace std;

struct Node;

class HuffmanEncoder{
	public:
		bool Encode(const char* infile, const char* outfile);
	private:
		bool heap_cmp(int a, int b);
		void BuildTree();
		void OutputTree(int outfd);
		int LettersEncodeAndOutput(FILE* infp, int outfd);
		off_t ReserveRoomInt(int fd);

		map<char, int> letter_count_;
		vector<Node*> node_vec_;
		vector<int> min_heap_;	//value is indices of node_vec
		map<char, int> node_idx_; // letter => index of node_vec
};

class HuffmanDecoder{
	public:
		bool Decode(const char* infile, const char* outfile);
	private:
		void ReadTree(int fd);
		void DecodeAndOutput(int infd, FILE* outfp);
		vector<Node*> node_vec_;
};
