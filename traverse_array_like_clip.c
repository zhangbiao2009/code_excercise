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

int nx = N;
int ny = M;

struct pos{
    int x;
    int y;
};

struct pos curr_pos = {0, 0};

int nDirection = 4;
enum Direction { Right=0, Down, Left, Up };
enum Direction curr_dir = Right;

void print()
{
    while(1){
        switch(curr_dir){
            case Right:
                for(int i=0; i<nx; i++){
                    printf("%x\n", a[curr_pos.y][curr_pos.x]);
                    if(i < nx-1)        // 最后一次坐标不变
                        curr_pos.x++;
                }
                ny--;           // 走完一行
                curr_pos.y++;   // 挪到下一个开始点
                break;
            case Down:
                for(int i=0; i<ny; i++){
                    printf("%x\n", a[curr_pos.y][curr_pos.x]);
                    if(i < ny-1)        // 最后一次坐标不变
                        curr_pos.y++;
                }
                nx--;           // 走完一列
                curr_pos.x--;   // 挪到下一个开始点
                break;
            case Left:
                for(int i=0; i<nx; i++){
                    printf("%x\n", a[curr_pos.y][curr_pos.x]);
                    if(i < nx-1)        // 最后一次坐标不变
                        curr_pos.x--;
                }
                ny--;           // 走完一行
                curr_pos.y--;   // 挪到下一个开始点
                break;
            case Up:
                for(int i=0; i<ny; i++){
                    printf("%x\n", a[curr_pos.y][curr_pos.x]);
                    if(i < ny-1)        // 最后一次坐标不变
                        curr_pos.y--;
                }
                nx--;           // 走完一列
                curr_pos.x++;   // 挪到下一个开始点
                break;
        }
        if(nx<=0 || ny<=0)
            return;
        // change direction
        curr_dir = (curr_dir+1)%nDirection;
    }
}

int main()
{
    print();
    return 0;
}
