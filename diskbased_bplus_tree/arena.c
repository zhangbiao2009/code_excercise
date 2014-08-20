#include "arena.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define nodep(base, ptr) \
    ((list_node*)(base+ptr))
#define get_size(base, ptr) \
    (((list_node*)(base+ptr))->size)
#define get_next(base, ptr) \
    (((list_node*)(base+ptr))->next)

typedef struct {
    int size;
    int next;
}list_node;

//key type 目前默认只有一种类型，即字符串

void btree_node_init(char* base)
{
    btree_node* bp = (btree_node*) base;
    bp->isleaf = 1;
    bp->nkeys = 0;
    bp->free_mem_header = sizeof(btree_node);
    bp->cell_list = 0;
    arena_init(base);
}

void arena_init(char* base)
{
    btree_node* bp = (btree_node*) base;
    offset_t header = bp->free_mem_header;

    offset_t start = header+sizeof(list_node);
    get_size(base, start) = (PAGE_SIZE-start)/sizeof(list_node);     // 分配时以list_node大小为基本单位
    get_next(base, start) = 0;

    get_size(base, header) = 0;
    get_next(base, header) = start;     // 分配头结点是为了简化alloc和free的代码
}

void free_list_print(char* base);
int arena_alloc(char* base, int nbytes)
{
    printf("alloc %d bytes:", nbytes);
    free_list_print(base);
    btree_node* bp = (btree_node*) base;
    offset_t header = bp->free_mem_header;

    int nunits = (nbytes+sizeof(list_node)-1)/sizeof(list_node) + 1;

    int prev, ptr;
    for(prev=header, ptr=get_next(base, header); ptr!=0; prev=ptr, ptr=get_next(base, ptr)){
        if(get_size(base, ptr) >= nunits){      // found
            if(get_size(base, ptr) == nunits){  // exactly
                get_next(base, prev) = get_next(base, ptr);
            }
            else{
                get_size(base, ptr) -= nunits;
                ptr += get_size(base, ptr)*sizeof(list_node);
                get_size(base, ptr) = nunits;
            }
            return ptr+sizeof(list_node);   // 给caller返回的内存不包括头部，方便回收
        }
    }
    return 0;      // not found
}

void arena_free(char* base, int mp)
{
    printf("in free:");
    btree_node* bp = (btree_node*) base;
    offset_t header = bp->free_mem_header;

    int ptr = mp - sizeof(list_node);
    int prev, p;
    for(prev = header, p = get_next(base, header); p!= 0 && p<ptr; prev = p, p=get_next(base, p))
        ;
    assert (p == 0 || p > ptr);

    if(ptr+get_size(base, ptr)*sizeof(list_node) == p){           //和下一块内存合并
        get_size(base, ptr) += get_size(base, p);
        get_next(base, ptr) = get_next(base, p);
    }
    else
        get_next(base, ptr) = p;
    if(prev+get_size(base, prev)*sizeof(list_node) == ptr){   //和上一块内存合并
        get_size(base, prev) += get_size(base, ptr);
        get_next(base, prev) = get_next(base, ptr);
    }
    else
        get_next(base, prev) = ptr;
    free_list_print(base);
}

void free_list_print(char* base)
{
    btree_node* bp = (btree_node*) base;
    offset_t header = bp->free_mem_header;
    int p;
    printf("\nfree_list: header=%d\t", header);
    for(p = get_next(base, header); p != 0;  p=get_next(base, p))
        printf("p=%d, bytes=%lu ", p, get_size(base,p)*sizeof(list_node));
    printf("\n");
}

void chunk_print(char* base, int ptr)
{
    int p = ptr - sizeof(list_node);
    printf("p=%d, bytes=%lu ", p, get_size(base,p)*sizeof(list_node));
}

void alloced_chunk_print(char* base, int* ptr_arr, int nptr)
{
    printf("alloced chunks:\t");
    int i;
    for(i=0; i<nptr; i++)
        chunk_print(base, ptr_arr[i]);
    printf("\n");
}

void remove_ptr_arr_entry(int* ptr_arr, int* nptr, int i)
{
    assert(i>=0 && i<*nptr);
    int j;
    for(j=i+1; j<*nptr; j++)
        ptr_arr[j-1] = ptr_arr[j];
    (*nptr)--;
}

/*
int main()
{
    char* base = (char*) malloc(PAGE_SIZE);
    int ptr_arr[100];
    int nptr = 0;
    btree_node_init(base);
    free_list_print(base);
    alloced_chunk_print(base, ptr_arr, nptr);
    ptr_arr[nptr++] = arena_alloc(base, 100);
    free_list_print(base);
    alloced_chunk_print(base, ptr_arr, nptr);

    ptr_arr[nptr++] = arena_alloc(base, 50);
    free_list_print(base);
    alloced_chunk_print(base, ptr_arr, nptr);

    ptr_arr[nptr++] = arena_alloc(base, 3881);
    free_list_print(base);
    alloced_chunk_print(base, ptr_arr, nptr);

    int i;
    while(1){
        printf("\nplease input array idx:");
        scanf("%d", &i);
        arena_free(base, ptr_arr[i]);
        remove_ptr_arr_entry(ptr_arr, &nptr, i);
        free_list_print(base);
        alloced_chunk_print(base, ptr_arr, nptr);
    }
    return 0;
}
*/
