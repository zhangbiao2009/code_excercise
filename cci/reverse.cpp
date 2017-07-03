#include <iostream>
using namespace std;

void reverse(char* str)
{
	if(!str || !str[0])
		return;

	int first = 0;
	int last;
	for(last = 0; str[last] != '\0'; last++)
		;
	last--;
	while(first < last){
		swap(str[first], str[last]);
		first++;
		last--;
	}
}
void reverse2(char* str)
{
	if(!str || !str[0])
		return;

	char* first = str;
	char* last;
	for(last = str; *last != '\0'; last++)
		;
	last--;
	while(first < last){
		swap(*first, *last);
		first++;
		last--;
	}
}

int main()
{
	char str[100] = {'\0'};
	reverse2(NULL);
	strcpy(str, "");
	reverse2(str);
	cout<<"str: "<<str<<endl<<endl;
	strcpy(str, "a");
	reverse2(str);
	cout<<"str: "<<str<<endl<<endl;
	strcpy(str, "ab");
	reverse2(str);
	cout<<"str: "<<str<<endl<<endl;
	strcpy(str, "abc");
	reverse2(str);
	cout<<"str: "<<str<<endl<<endl;
	strcpy(str, "abcd");
	reverse2(str);
	cout<<"str: "<<str<<endl<<endl;
	strcpy(str, "abcde");
	reverse2(str);
	cout<<"str: "<<str<<endl<<endl;
	strcpy(str, "abcdef");
	reverse2(str);
	cout<<"str: "<<str<<endl<<endl;
	return 0;
}
