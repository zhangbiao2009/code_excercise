#include <iostream>

using namespace std;


int partition(int a[], int low, int high)
{
	/*if(low == high)
		return low; */ // 这两行没有也能正常工作
	int i = low+1;
	int j = high;

	while(i <= j){
		// invariant1: i<=j
		// invariant2: a[low+1, i) <= a[low] && a(j, high] >= a[low]
		while(a[i] <= a[low] && i <= j)
			i++;
		// a[low+1, i) <= a[low] && (a[i] > a[low] || i > j)
		while(a[j] >= a[low] && i <= j)
			j--;
		// a(j, high] >= a[low] && (a[j] < a[low] || i > j)
		//
		if(i > j){
			break;
		}

		// i<=j && a[i] > a[low] && a[j] < a[low]
		swap(a[i], a[j]);
		// i<=j && a[i] < a[low] && a[j] > a[low]
		i++;
		j--;
		// invariant 2 holds
	}

	// invariant 2 holds && i = j+1
	// j < i && a[1, i) <= a[low] => a[j] <= a[low]
	// j < i && a(j, high] >= a[low] => a[i] >= a[low]
	// so j is the place where pivot key should be place
	swap(a[low], a[j]);

	cout<<"after partition: ";
	for(int k=low; k<=high; k++)
		cout<<a[k]<<' ';
	cout<<endl;
	return j;
}


int main(){
	//int a[] = {5, 3, 6, 7, 2, 1, 9, 4, 8};
	//int a[] = {5, 3};
	int a[] = {5};
	int low = 0;
	int high = sizeof(a)/sizeof(a[0]) - 1;

	cout<<"before partition: ";
	for(int k=low; k<=high; k++)
		cout<<a[k]<<' ';
	cout<<endl;
	cout<<"pivot index: "<< partition(a, 0, high)<<endl;
	return 0;
}
