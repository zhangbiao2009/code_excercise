#include <iostream>
#include <algorithm>
#include <cassert>

using namespace std;

void print(int* a, int l, int h);

const int n=100000;
int a[n];
int cnt=0;

int partition(int* a, int l, int h)
{
	assert(h-l>1);

	int pivot = l+rand()%(h-l);		// l<=pivot<h
	int pivot_val = a[pivot];
	swap(a[pivot], a[h-1]);
	int k = l;
	for(int i=l; i<h-1; i++){
		if(a[i] <= pivot_val){
			swap(a[i], a[k]);
			k++;
		}
	}
	swap(a[h-1], a[k]);			// put the pivot record at right position
	return k;
}
void qsort(int* a, int l, int h)	// sort [l, h)
{
	cnt++;
	if(h-l <= 1)
		return;
	int pivot = partition(a, l, h);
	qsort(a, l, pivot);		//sort [l, pivot)
	qsort(a, pivot+1, h);	//sort [pivot+1, h)
}

void print(int* a, int l, int h)
{
	for(int i=l; i<h; i++)
		cout<<a[i]<<" ";
	cout<<endl;
}

void gen_arr()
{
	for(int i=0; i<n; i++)
		a[i] = rand()%1000;
}

bool verify()
{
	for(int i=1; i<n; i++)
		if(a[i]<a[i-1])
			return false;
	return true;
}

int main()
{
	gen_arr();
	/*
	cout<<"before sorting: "<<endl;
	print(a, 0, n);
	*/
	qsort(a, 0, n);
	assert(verify());
	cout<<"cnt="<<cnt<<endl;
	/*
	cout<<"after sorting: "<<endl;
	print(a, 0, n);
	*/
	return 0;
}
