#include <stdio.h>

int num_of_ones(int v){
	int c;
	for(c = 0; v; c++)
		v &= v -1;
	return c;
}

int bit_idx(int v) {
	//assert(v != 0);
	int c;
	v = (v ^ (v-1)) >> 1;
	for (c=0; v; c++)
		v >>= 1;
	return c;
}

int mul2_internal(int a, int b, int* cntp){
	(*cntp)++;
	printf("a: %d, b: %d\n", a, b);
	if(a==0 || b==0)
		return 0;
	int nz = bit_idx(b);
	return (a <<  nz) + mul2_internal(a, b - (1<<nz), cntp);
}

int mul2(int a, int b, int* cntp){
	int na = num_of_ones(a);
	int nb = num_of_ones(b);
	return na > nb ? mul2_internal(a, b, cntp) : mul2_internal(b, a, cntp);
}

int mul(int a, int b, int* cntp){
	(*cntp)++;
	printf("a: %d, b: %d\n", a, b);
	if(a==0 || b==0)
		return 0;
	if (a < b){
		int tmp = a;
		a = b;
		b = tmp;
	}
	if ( (b&1) != 0 )
		return a + mul(a, b-1, cntp);
	else{
		return mul(a, b>>1, cntp)<< 1;
	}
}

void foo(int a, int b) {
	int cnt = 0;
	printf("res: %d\n", mul(a, b, &cnt));
	printf("cnt: %d\n", cnt);
	int cnt2 = 0;
	printf("res2: %d\n", mul2(a, b, &cnt2));
	printf("cnt2: %d\n", cnt2);

}

int main(){
	foo(59, 150);
		/*
	printf("%d\n", mul(3, 1));
	printf("%d\n", mul(3, 4));
	printf("%d\n", mul(1, 4));
	printf("%d\n", mul(0, 4));
	*/
	return 0;
}
