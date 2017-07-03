#include <iostream>
using namespace std;

void replace(char* str, int n)
{
	if(!str || n <= 0)
		return;
	int nspace = 0;
	for(int i=0; i<n; i++){
		if(str[i] == ' ')
			nspace++;
	}

	for(int i=n-1; i>=0; i--){
		if(str[i] != ' '){
			int new_idx = i + nspace * 2;
			str[new_idx] = str[i];
		}
		else{
			int new_idx = i + (nspace - 1) * 2;
			str[new_idx] = '%';
			str[new_idx+1] = '2';
			str[new_idx+2] = '0';
			nspace--;
		}
	}
}

void test(const char* s){
	char str[100] = {0};
	strcpy(str, s);
	replace(str, strlen(s));
	cout<<"str: "<<str<<endl<<endl;
}
int main()
{
	test("");
	test(" ");
	test("  ");
	test("a");
	test(" a");
	test("  a");
	test("a ");
	test("a  ");
	test("abc");
	test("abc ");
	test("abc  ");
	test("Mr John Smith");
	test("Mr John Smith  ");
	test("Mr    John Smith");

	return 0;
}
