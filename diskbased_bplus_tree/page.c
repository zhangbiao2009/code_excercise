#include "page.h"
#include "page_list.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static int max_page_id = 1; // page_id 0 reserved for header

page* page_alloc(int fd, int page_id)
{
    // fetch from page free list, or alloc a new page
    page* p = (page*) malloc(sizeof(page));
    p->fd = fd;
    p->id = page_id;
    p->base = (char*) malloc(PAGE_SIZE);
    return p;
}

void page_free(page* p)
{
    free(p->base);
    free(p);
}

page* page_read(int fd, int page_id)
{
    page* p = page_alloc(fd, page_id);
    lseek(fd, p->id*PAGE_SIZE, SEEK_SET);
    read(fd, p->base, PAGE_SIZE);
    return p;
}

void page_write(page* p)
{
    lseek(p->fd, p->id*PAGE_SIZE, SEEK_SET);
    write(p->fd, p->base, PAGE_SIZE);
}

char cmd[100];
char key[100000];
char val[100000];
int main()
{
    int fd = open("a.db", O_CREAT|O_RDWR, 0644);
    page* p;
    /*
    p = page_alloc(fd, 0);
    btree_node_init(p->base);

    while(1){
        printf("input cmd:\n");
        scanf("%s", cmd);
        if(strcmp(cmd, "add") == 0){
            scanf("%s%s", key, val);
            if(!list_add(p->base, key, strlen(key)+1, val, strlen(val)+1))  //为方便打印，把\0也存下来
                printf("add failed\n");
        }
        else if(strcmp(cmd, "del") == 0){
            scanf("%s", key);
            list_del(p->base, key, strlen(key)+1);
        }
        else
            break;
        list_print(p->base);
    }

    page_write(p);
    */
    p = page_read(fd, 0);
    list_print(p->base);

    page_free(p);

    close(fd);
    return 0;
}

