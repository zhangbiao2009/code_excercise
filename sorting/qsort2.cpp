#include <iostream>
#include <algorithm>
#include <cassert>

using namespace std;

void print(int* a, int l, int h);

const int n=1000000;
int a[n];
int cnt=0;

int partition(int* a, int l, int h)
{
	assert(h-l>=2);

	int pivot = l+rand()%(h-l);		// l<=pivot<h
	int pivot_val = a[pivot];
	swap(a[pivot], a[l]);
	int low  = l;
	int high = h - 1;
	// low < high && for all i<=low a[i]<=pivot_val && for all j>high a[j] >= pivot_val
	while(low < high){
		//  low < high
		while(low < high && a[high] >= pivot_val)
			high--;
		// low == high && a[low] <= pivot_val && a[high] < pivot_val || low < high && a[low] <= pivot_val && a[high] < pivot_val
		if(low == high) break;
		// low < high  && a[low] <= pivot_val && a[high] < pivot_val
		while(low < high && a[low] <= pivot_val)
			low++;
		// low == high && a[low] = a[high] < pivot_val || low < high && a[low] > pivot_val && a[high] < pivot_val
		swap(a[low], a[high]);
		// low == high && a[low] = a[high] < pivot_val || low < high && a[low] < pivot_val && a[high] > pivot_val
	}

	// low == high && a[low]<=privot && a[high]<privot && for all i<=low a[i] <= pivot_val && for all j>high a[j] >= pivot_val
	swap(a[l], a[low]);			// put the pivot record at right position
	return low;
}

void qsort(int* a, int l, int h)	// sort [l, h)
{
	cnt++;
	if(h-l <= 1)
		return;
	// h - l >= 2
	int pivot = partition(a, l, h);
	/*
	cout<< "pivot="<<a[pivot]<<endl;
	cout<<"after partition:"<<endl;
	print(a, 0, n);
	*/
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
	/*
	cout<<"after sorting: "<<endl;
	print(a, 0, n);
	return 0;
	*/
	assert(verify());
	cout<<"cnt="<<cnt<<endl;
	/*
	cout<<"after sorting: "<<endl;
	print(a, 0, n);
	*/
	return 0;
}
