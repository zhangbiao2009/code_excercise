#include "huffman.h"

int main()
{
	HuffmanEncoder encoder;
	encoder.Encode("a.txt", "a.huff");

	HuffmanDecoder decoder;
	decoder.Decode("a.huff", "b.txt");

	return 0;
}
