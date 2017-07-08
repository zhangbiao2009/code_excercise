#include <iostream>
#include <cctype>
#include <map>

using namespace std;

bool is_palindrome_permutation(string str)
{
	map<char, int> char_count;
	for(int i=0; i<str.length(); i++){
		if(isspace(str[i])) continue;
		char c = tolower(str[i]);
		char_count[c]++;
	}
	int odd_numer_count = 0;
	for(auto it=char_count.begin(); it!=char_count.end(); it++){
		if(it->second%2 != 0)
			odd_numer_count++;
	}
	return odd_numer_count <= 1;
}

int main()
{
	cout<<is_palindrome_permutation("Tact Coa")<<endl;
	cout<<is_palindrome_permutation("Tact Coa")<<endl;
	return 0;
}
