#include <stdio.h>

#define M 4
#define N 3
int a[M][N] = {
    0x1, 0x2, 0x3,
    0xa, 0xb, 0x4,
    0x9, 0xc, 0x5,
    0x8, 0x7, 0x6,
};

int up_border = 0;
int down_border = M-1;
int left_border = 0;
int right_border = N-1;

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
        // go to border
        int finished = gotoBorder();
        if(finished)
            break;
        // change direction
        curr_dir = (curr_dir+1)%nDirection;
    }
}

int gotoBorder()
{
    while(1){
        printf("%x\n", a[curr_pos.y][curr_pos.x]);
        //print
        switch(curr_dir){
            case Right:
                if(curr_pos.x == right_border){
                    up_border++;        // 走完一行
                    curr_pos.y++;       // 挪到下一个开始点
                    goto exit;
                }
                curr_pos.x++;
                break;
            case Down:
                if(curr_pos.y == down_border){
                    right_border--;     // 走完一列
                    curr_pos.x--;
                    goto exit;
                }
                curr_pos.y++;
                break;
            case Left:
                if(curr_pos.x == left_border){
                    down_border--;        // 走完一行
                    curr_pos.y--;
                    goto exit;
                }
                curr_pos.x--;
                break;
            case Up:
                if(curr_pos.y == up_border){
                    left_border++;     // 走完一列
                    curr_pos.x++;
                    goto exit;
                }
                curr_pos.y--;
                break;
        }
    }

exit:
    if(up_border > down_border || left_border > right_border)
        return 1;
    else
        return 0;
}

int main()
{
    print();
    return 0;
}
