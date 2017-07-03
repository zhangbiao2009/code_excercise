#include <iostream>
#include <string>
#include <vector>

using namespace std;

bool write_to_vec(vector<char>& res, char last_char, int cnt, int orig_len)
{
	res.push_back(last_char);
	string str_cnt = to_string(cnt);
	for(int i=0; i<str_cnt.length(); i++){
		res.push_back(str_cnt[i]);
	}
	if(res.size() >= orig_len)
		return false;
	return true;
}

string compress(string str)
{
	if(str.empty())
		return str;

	int n = str.length()+1;
	vector<char> res;
	char last_char;
	int cnt;
	bool begin = true;

	for(int i=0; i<str.length(); i++){
		if(begin){
			last_char = str[i];
			cnt = 1;
			begin = false;
			continue;
		}

		if(str[i] == last_char)
			cnt++;
		else{
			if(!write_to_vec(res, last_char, cnt, str.length()))
				return str;
			last_char = str[i];
			cnt = 1;
		}
	}
	if(!write_to_vec(res, last_char, cnt, str.length()))
		return str;

	return string(res.begin(), res.end());
}

int main()
{
	cout<<compress("")<<endl;
	cout<<compress("a")<<endl;
	cout<<compress("aa")<<endl;
	cout<<compress("aaa")<<endl;
	cout<<compress("aab")<<endl;
	cout<<compress("aaab")<<endl;
	cout<<compress("aaaab")<<endl;
	cout<<compress("aaabb")<<endl;
	cout<<compress("aabcccccaaa")<<endl;
	cout<<compress("aabcccccccccccaaa")<<endl;

	return 0;
}
