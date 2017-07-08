#include <iostream>
#include <cstdlib>
#include <cctype>
#include <set>

using namespace std;

int num_of_not_found_characters(string haystack, string needles)
{
	multiset<char> set;
	for(int i=0; i<haystack.size(); i++)
		set.insert(haystack[i]);
	int num_of_not_found = 0;
	for(int i=0; i<needles.size(); i++){
		auto it = set.find(needles[i]);
		if(it == set.end())
			num_of_not_found++;
		else 
			set.erase(it);
	}
	return num_of_not_found;
}

bool one_away(string src, string dest)
{
	int len_diff = src.length()-dest.length();
	if(abs(len_diff) > 1)
		return false;
	if(src.length() == dest.length() + 1)
		return num_of_not_found_characters(src, dest) == 0;
	else if(src.length() + 1 == dest.length())
		return num_of_not_found_characters(dest, src) == 0;
	else{
		return num_of_not_found_characters(dest, src) <= 1;
	}
}

int main()
{
	cout<<one_away("pale", "ple")<<endl;
	cout<<one_away("pales", "pale")<<endl;
	cout<<one_away("pale", "pales")<<endl;
	cout<<one_away("pale", "spales")<<endl;
	cout<<one_away("pale", "bale")<<endl;
	cout<<one_away("pale", "bake")<<endl;
	return 0;
}
