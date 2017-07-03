#include <string>
#include <map>
#include <algorithm>
#include <iostream>

using namespace std;

bool isAllUnique(string s)
{
	sort(s.begin(), s.end());
	for(int i=1; i<s.length(); i++){
		if(s[i] == s[i-1])
			return false;
	}
	return true;
}

bool isAllUnique2(string s)
{
	map<char, int> m;
	for(int i=0; i<s.length(); i++){
		m[s[i]]++;
	}
	for(auto it=m.begin(); it!=m.end(); it++){
		if(it->second != 1)
			return false;
	}
	return true;
}

int main()
{
	string s;
	s = "wergbx";
	cout<<"s: "<<s<<", is all unique: "<< isAllUnique(s)<<endl;
	cout<<"s: "<<s<<", is all unique2: "<< isAllUnique2(s)<<endl;
	cout<<endl;
	s = "a";
	cout<<"s: "<<s<<", is all unique: "<< isAllUnique(s)<<endl;
	cout<<"s: "<<s<<", is all unique2: "<< isAllUnique2(s)<<endl;
	cout<<endl;
	s = "aa";
	cout<<"s: "<<s<<", is all unique: "<< isAllUnique(s)<<endl;
	cout<<"s: "<<s<<", is all unique2: "<< isAllUnique2(s)<<endl;
	cout<<endl;
	s = "wergebx";
	cout<<"s: "<<s<<", is all unique: "<< isAllUnique(s)<<endl;
	cout<<"s: "<<s<<", is all unique2: "<< isAllUnique2(s)<<endl;
	cout<<endl;
	s = "wxergbx";
	cout<<"s: "<<s<<", is all unique: "<< isAllUnique(s)<<endl;
	cout<<"s: "<<s<<", is all unique2: "<< isAllUnique2(s)<<endl;
	cout<<endl;
	return 0;
}
