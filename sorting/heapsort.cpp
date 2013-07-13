#include <iostream>
#include <algorithm>
#include <cassert>
#include <ctime>
#include <cstdio>
#include <cstring>

using namespace std;

void print(int* a, int l, int h);

const int n=100;
int a[n];
int b[n];
int cnt=0;

void sink(int* a, int i, int n)		// [0, n)
{
	while(i < n){
		int lc = 2*(i+1)-1;
		int rc = 2*(i+1);
		if(lc >= n)
			break;
		if(rc >= n){
			if(a[i] < a[lc])
			   swap(a[i], a[lc]);
			break;
		}
		int mi = a[lc]>a[rc] ? lc:rc;
		if(a[i] < a[mi])
		   swap(a[i], a[mi]);
		i = mi;
	}
}

void heap_make(int* a, int n) // [0, n)
{
	for(int i=n/2-1; i>=0; i--)
		sink(a, i, n);
}

void heap_pop(int* a, int n) 
{
	swap(a[0], a[n-1]);
	sink(a, 0, n-1);
}

void heapsort(int* a, int n)	// sort [0, n)
{
	heap_make(a, n);
	for(int i=0; i<n; i++)
		heap_pop(a, n-i);
	
}

void print(int* a, int l, int h)
{
	for(int i=l; i<h; i++)
		cout<<a[i]<<" ";
	cout<<endl;
}

void gen_arr()
{
	srand(time(NULL));
	for(int i=0; i<n; i++)
		a[i] = rand()%1000;
}

bool verify()
{
	for(int i=0; i<n; i++)
		if(a[i] != b[i])
			return false;
	for(int i=1; i<n; i++)
		if(a[i]<a[i-1])
			return false;
	return true;
}

int main()
{
	gen_arr();
	memcpy(b, a, sizeof(int)*n);
	sort(b, b+n);
	heapsort(a, n);
	assert(verify());
	print(a, 0, n);
	//cout<<"cnt="<<cnt<<endl;
	return 0;
}
