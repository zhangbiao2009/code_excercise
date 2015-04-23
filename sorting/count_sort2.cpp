#include <iostream>
#include <string>

using namespace std;

struct student{
	const char* name;
	int section_no;
};

student stus[] = {
	{"Anderson", 2},
	{"Brown",    3},
	{"Davis",    3},
	{"Garcia",   4},
	{"Harris",   1},
	{"Jackson",  3},
	{"Johnson",  4},
	{"Jones",    3},
	{"Martin",   1},
	{"Martinez", 2},
	{"Miller",   2},
	{"Moore",    1},
	{"Robinson", 2},
	{"Smith",    4},
	{"Taylor",   3},
	{"Thomas",   4},
	{"Thompson", 4},
	{"White",    2},
	{"Williams", 3},
	{"Wilson",   4},
};

void count_sort(student arr[], int n)
{
	int count[4] = {0};
	for(int i=0; i<n; i++){
		count[arr[i].section_no-1]++;
	}
	for(int i=1; i<4; i++)
		count[i] += count[i-1];

	student arr2[n];
	for(int i=0; i<n; i++){
		int key = arr[i].section_no-1;
		int new_index = count[key];
		arr2[new_index-1] = arr[i];
		count[key]--;
	}

	for(int i=0; i<n; i++)
		cout<<arr2[i].name<<", "<<arr2[i].section_no<<endl;
}

int main()
{
	int n = sizeof(stus)/sizeof(student);
	count_sort(stus, n);
	return 0;
}
