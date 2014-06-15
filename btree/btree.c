#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "btree.h"

btree* alloc_node(int t, int isleaf, int nkeys)
{
    assert(t >= 2);
    btree* bp = (btree*) malloc(sizeof(btree));
    bp->keys = (int*)calloc(2*(t-1), sizeof(int));   // at most 2t-1 keys
    bp->child = (btree**)calloc(2*t, sizeof(btree*));   // at most 2t child
    assert(bp != NULL);
    bp->isleaf = isleaf;
    bp->nkeys = nkeys;
    bp->t = t;
    return bp;
}

btree* btree_new(int t)
{
    return alloc_node(t, 1, 0);
}

void btree_split(btree* bp, int ci)  // split bp的第ci个child，前提是ci已经有2t-1个key了
{
    assert(bp->child[ci]->nkeys == 2*bp->t-1);

    btree* snodep = bp->child[ci];
    /* 新分配一个节点newbp, newbp 拷贝snode的后半段, 
     * 前半段仍留在snode中，中间的元素上升到bp去 */
    btree* newbp = alloc_node(bp->t, snodep->isleaf, bp->t-1);
    int i;
    for(i=0; i < newbp->nkeys; i++)
        newbp->keys[i] = snodep->keys[bp->t+i];
    if(!snodep->isleaf){
        for(i=0; i <= newbp->nkeys; i++)
            newbp->child[i] = snodep->child[bp->t+i];
    }

    snodep->nkeys = bp->t-1;

    for(i=bp->nkeys-1; i >= ci; i--)
        bp->keys[i+1] = bp->keys[i];

    bp->keys[ci] = snodep->keys[bp->t-1]; // up

    for(i= bp->nkeys; i >= ci+1; i--)
        bp->child[i+1] = bp->child[i];

    bp->child[ci+1] = newbp;

    bp->nkeys++;
}

void btree_insert_internal(btree* bp, int key)
{
    int i = bp->nkeys - 1;
    if(bp->isleaf){
        for(; i >= 0 && key < bp->keys[i]; i--)
            bp->keys[i+1] = bp->keys[i];
        bp->keys[i+1] = key;
        bp->nkeys++;
    }
    else{
        for(; i >= 0 && key < bp->keys[i]; i--)
            ;
        i++;
        if(bp->child[i]->nkeys == 2*bp->t-1){
            btree_split(bp, i);
            if(key > bp->keys[i])
                i++;
        }
        btree_insert_internal(bp->child[i], key);
    }
}

btree* btree_insert(btree* bp, int key)
{
    if(bp->nkeys == 2*bp->t-1){ // the root needs split
        btree* newbp = alloc_node(bp->t, 0, 0);
        newbp->child[0] = bp;
        btree_split(newbp, 0);
        btree_insert_internal(newbp, key);
        return newbp;
    }
    btree_insert_internal(bp, key);
    return bp;
}

btree* btree_get(btree* bp, int key, int* slot)
{
    int i;
    for(i=0; i<bp->nkeys && key > bp->keys[i]; i++)
        ;
    if(i<bp->nkeys && bp->nkeys == key){
        *slot = i;
        return bp;
    }
    else if (bp->isleaf)
        return NULL;
    else{
        return btree_get(bp->child[i], key, slot);
    }
}

void btree_traverse(btree* bp, int d)
{
    int i;
    printf("( ");
    if(bp->isleaf){
        for(i=0; i<bp->nkeys; i++)
            printf("%d,%d ", bp->keys[i], d);
    }else{
        for(i=0; i<bp->nkeys; i++){
            btree_traverse(bp->child[i], d+1);
            printf("%d,%d ", bp->keys[i], d);
        }
        btree_traverse(bp->child[bp->nkeys], d+1);
    }
    printf(") ");
}

int main()
{
    btree* bp = btree_new(2);
    bp = btree_insert(bp, 100);
    bp = btree_insert(bp, 80);
    bp = btree_insert(bp, 95);
    bp = btree_insert(bp, 50);
    bp = btree_insert(bp, 74);
    bp = btree_insert(bp, 79);
    bp = btree_insert(bp, 88);
    bp = btree_insert(bp, 85);
    bp = btree_insert(bp, 90);
    bp = btree_insert(bp, 92);
    bp = btree_insert(bp, 93);
    bp = btree_insert(bp, 91);
    bp = btree_insert(bp, 150);
    bp = btree_insert(bp, 180);
    bp = btree_insert(bp, 280);
    bp = btree_insert(bp, 200);
    bp = btree_insert(bp, 250);
    bp = btree_insert(bp, 270);
    bp = btree_insert(bp, 260);
    btree_traverse(bp, 1);
    /*
    btree_iter iter = btree_begin(bp);
    for(; iter != btree_end(bp); iter = btree_next(iter))
        printf("%, %s\n", btree_key(iter), btree_value(iter));
        */
    return 0;
}
