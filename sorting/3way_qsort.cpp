#include <iostream>
#include <algorithm>
#include <cassert>
#include <ctime>
#include <cstdio>
#include <cstring>

using namespace std;

void print(int* a, int l, int h);

const int n=1000;
int a[n];
int b[n];
int cnt=0;

void qsort(int* a, int l, int h)	// sort [l, h]
{
	cnt++;
	//print(a, 0, n);
	if(h-l < 1)
		return;
	int pivot = l+rand()%(h-l)+1;		// l<=pivot<=h
	int pivot_val = a[pivot];
	int lt = l;
	int i=l;
	int gt = h;
	while(i<=gt){
		if(a[i] < pivot_val){
			swap(a[i], a[lt]);
			lt++;
			i++;
		}else if (a[i] == pivot_val)
			i++;
		else{
			swap(a[i], a[gt]);
			gt--;
		}
	}
	//[l, lt-1] < pivot
	//[lt, i) = pivot
	//[i,h] > pivot
	qsort(a, l, lt-1);		//sort [l, pivot-1]
	qsort(a, i, h);	//sort [pivot+1, h]
}

void print(int* a, int l, int h)
{
	for(int i=l; i<h; i++)
		cout<<a[i]<<" ";
	cout<<endl;
}

void gen_arr()
{
	//srand(time(NULL));
	for(int i=0; i<n; i++)
		a[i] = rand()%10;
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
	qsort(a, 0, n-1);
	assert(verify());
	cout<<"cnt="<<cnt<<endl;
	return 0;
}
