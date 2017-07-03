#include <string>
#include <iostream>
using namespace std;

bool is_permutation(string str1, string str2)
{
	if(str1.length() != str2.length())
		return false;
	sort(str1.begin(), str1.end());
	sort(str2.begin(), str2.end());
	return str1 == str2;
}

int main()
{
	string str1;
	string str2;
	cout<<"str1: "<<str1<<", str2: "<< str2<<", is_permutation: "<<is_permutation(str1, str2)<<endl<<endl;
	str1 = "adsf";
	str2 = "asf";
	cout<<"str1: "<<str1<<", str2: "<< str2<<", is_permutation: "<<is_permutation(str1, str2)<<endl<<endl;
	str1 = "adsf";
	str2 = "asfd";
	cout<<"str1: "<<str1<<", str2: "<< str2<<", is_permutation: "<<is_permutation(str1, str2)<<endl<<endl;
	str1 = "adsf";
	str2 = "akfd";
	cout<<"str1: "<<str1<<", str2: "<< str2<<", is_permutation: "<<is_permutation(str1, str2)<<endl<<endl;
	str1 = "adsf";
	str2 = "adsf";
	cout<<"str1: "<<str1<<", str2: "<< str2<<", is_permutation: "<<is_permutation(str1, str2)<<endl<<endl;
	str1 = "adsf";
	str2 = "adsfd";
	cout<<"str1: "<<str1<<", str2: "<< str2<<", is_permutation: "<<is_permutation(str1, str2)<<endl<<endl;
	return 0;
}
