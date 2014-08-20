#include "page_list.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct{
    int key_len;
    int val_len;
    offset_t next;
    char mem[];
}list_node;

#define get_key(np)  ((np)->mem)
#define get_val(np)  ((np)->mem+(np)->key_len)

void list_init(char* base)
{
    //arena_init(base, header, start);
}

// 有序链表(从小到大)，为简化起见，假设key都是C字符串
int list_add(char* base, char* key, int key_len, char* val, int val_len)
{
    btree_node* bp = (btree_node*) base;
    // alloc
    int mem = arena_alloc(base, sizeof(list_node)+key_len+val_len);
    if(mem == 0)
        return 0;
    list_node* np = (list_node*)(base+mem);
    np->key_len = key_len;
    np->val_len = val_len;
    memcpy(get_key(np), key, key_len);
    memcpy(get_val(np), val, val_len);

    int* i = &bp->cell_list;
    while(*i != 0) {
        list_node* tmp = (list_node*)(base+*i);
        if(memcmp(get_key(tmp), key, key_len) > 0)
            break;
        i = &tmp->next;
    }

    np->next = *i;
    *i = mem;

    return 1;
}

void list_print(char* base)
{
    btree_node* bp = (btree_node*) base;
    int i = bp->cell_list;
    while(i != 0) {
        list_node* np = (list_node*)(base+i);
        printf("(%s, %s) ", get_key(np), get_val(np));    // 暂时假设key和val都是C字符串
        i = np->next;
    }
    printf("\n");
}


void list_del(char* base, char* key, int key_len)
{
    btree_node* bp = (btree_node*) base;
    int* i = &bp->cell_list;
    while(*i != 0) {
        list_node* np = (list_node*)(base+*i);
        if(np->key_len == key_len && 
                memcmp(get_key(np), key, key_len) == 0) // match
        {
            arena_free(base, *i);
            *i = np->next;
        }
        else
            i = &np->next;
    }
}


/*
char cmd[100];
char key[100000];
char val[100000];

int main()
{
    char* base = (char*) malloc(PAGE_SIZE);
    btree_node_init(base);
    while(1){
        printf("input cmd:\n");
        scanf("%s", cmd);
        if(strcmp(cmd, "add") == 0){
            scanf("%s%s", key, val);
            if(!list_add(base, key, strlen(key)+1, val, strlen(val)+1))  //为方便打印，把\0也存下来
                printf("add failed\n");
        }
        else{
            scanf("%s", key);
            list_del(base, key, strlen(key)+1);
        }
        list_print(base);
    }
    return 0;
}
*/
