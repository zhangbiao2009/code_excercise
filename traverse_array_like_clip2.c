#include <stdio.h>

#define M 3
#define N 3
int a[M][N] = {
    0x1, 0x2, 0x3,
    0x8, 0x9, 0x4,
    0x7, 0x6, 0x5,
};

/*
#define M 4
#define N 3
int a[M][N] = {
    0x1, 0x2, 0x3,
    0xa, 0xb, 0x4,
    0x9, 0xc, 0x5,
    0x8, 0x7, 0x6,
};
*/

// 归纳总结：走一圈需要的nsteps = 2*(m+n-2)
void print()
{
    int nloop = 0;
    int m = M;
    int n = N;
    for(int i=0, j=0; i<M*N; i++, j++){

        if(m>1 && n>1 && j == 2*(m+n-2)){     // 该下一圈了
            nloop++;
            m-=2;
            n-=2;
            j = 0;
        }
        
        // 计算当前坐标
        int x, y;
        // 分段方式采用前闭后开的区间, 即[ )
        if(j >= n-1+m-1+n-1){      // 一圈中的最后一段
            x = nloop+m-1 - (j - (n-1+m-1+n-1));
            y = nloop;
        }
        else if( j >= n-1+m-1){     // 一圈中的第三段
            x = nloop+m-1;
            y = nloop+n-1 - (j - (n-1+m-1));
        }else if (j >= n-1){     // 一圈中的第二段
            x = nloop + (j - (n-1));
            y = nloop+n-1;
        }else{     // 一圈中的第一段
            x = nloop;
            y = nloop+j;
        }
        printf("%x\n", a[x][y]);
    }
}

int main()
{
    print();
    return 0;
}
