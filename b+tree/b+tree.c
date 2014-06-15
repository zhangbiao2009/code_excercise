#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "b+tree.h"

btree* g_bt;

int is_leaf_node(btree_node* bp)
{
    return bp->isleaf;
}

btree_index_node* alloc_index_node(int t, int nkeys)
{   
    btree_index_node* bp = (btree_index_node*) malloc(sizeof(btree_index_node));
    assert(bp != NULL);
    bp->base.isleaf = 0;
    bp->base.nkeys = nkeys;
    bp->base.keys = (int*)calloc(2*t-1, sizeof(int));   // at most 2t-1 keys
    bp->child = (btree_node**)calloc(2*t, sizeof(btree_node*));   // at most 2t child
    return bp;
}   

btree_leaf_node* alloc_leaf_node(int t, int nkeys)
{   
    btree_leaf_node* bp = (btree_leaf_node*) malloc(sizeof(btree_leaf_node));
    assert(bp != NULL);
    bp->base.isleaf = 1;
    bp->base.nkeys = nkeys;
    bp->base.keys = (int*)calloc(2*t-1, sizeof(int));   // at most 2t-1 keys
    bp->values = (int*)calloc(2*t-1, sizeof(int));   // at most 2t-1 values 
    bp->prev = NULL;
    bp->next = NULL;
    return bp;
}

void free_node(btree_node* bp)
{
    if(is_leaf_node(bp))
        free(((btree_leaf_node*)bp)->values);
    else
        free(((btree_index_node*)bp)->child);
    free(bp->keys);
    free(bp);
}

btree* btree_new(int t)
{
    btree* bt = (btree*) malloc(sizeof(btree));
    bt->t = t;
    btree_node* bp = (btree_node*)alloc_leaf_node(t, 0);
    bt->root = bp;
    bt->first = bt->last = bp;
    return bt;
}

void btree_destroy_internal(btree_node* bp)
{
    if(is_leaf_node(bp)){
        free_node(bp);
    }
    else{
        int i;
        for(i=0; i<=bp->nkeys; i++)
            btree_destroy_internal(((btree_index_node*)bp)->child[i]);
        free_node(bp);
    }
}

void btree_destroy()
{
    btree_destroy_internal(g_bt->root);
    free(g_bt);
}

void btree_split(btree_index_node* bp, int ci)  // split bp的第ci个child，前提是ci已经有2t-1个key了
{
    int t = g_bt->t;
    //assert(bp->child[ci]->nkeys == 2*t-1);

    /* 新分配一个节点newbp, newbp 拷贝snode的后半段, 
     * 前半段仍留在snode中，如果是index node，则中心点上升到bp去 
     * 如果是leaf node，中心点key value都拷贝到newbp中去，key上升到bp去
     * */
    int i;
    btree_node* tmp;
    if(is_leaf_node(bp->child[ci])){
        // 拷贝snodep后面t个key和value，设置next指针
        btree_leaf_node* snodep = (btree_leaf_node*)bp->child[ci];
        btree_leaf_node* newbp = alloc_leaf_node(t, t);
        for(i=0; i < newbp->base.nkeys; i++){
            newbp->base.keys[i] = snodep->base.keys[t-1+i];
            newbp->values[i] = snodep->values[t-1+i];
        }
        newbp->next = snodep->next;
        snodep->next = newbp;
        newbp->prev = snodep;
        if(newbp->next != NULL){
            newbp->next->prev = newbp;
        }
        if(g_bt->last == (btree_node*)snodep)
            g_bt->last = (btree_node*)newbp;
        tmp = (btree_node*)newbp;
    }
    else{
        btree_index_node* snodep = (btree_index_node*)bp->child[ci];
        btree_index_node* newbp = alloc_index_node(t, t-1);
        for(i=0; i < newbp->base.nkeys; i++)
            newbp->base.keys[i] = snodep->base.keys[t+i];
        for(i=0; i <= newbp->base.nkeys; i++)
            newbp->child[i] = snodep->child[t+i];
        tmp = (btree_node*)newbp;
    }

    btree_node* snodep = bp->child[ci];
    snodep->nkeys = t-1;

    for(i=bp->base.nkeys-1; i >= ci; i--)
        bp->base.keys[i+1] = bp->base.keys[i];

    bp->base.keys[ci] = snodep->keys[t-1]; // up

    for(i= bp->base.nkeys; i >= ci+1; i--)
        bp->child[i+1] = bp->child[i];

    bp->child[ci+1] = tmp;

    bp->base.nkeys++;
}

void btree_insert_internal(btree_node* bp, int key, int val)
{
    int i = bp->nkeys - 1;
    if(is_leaf_node(bp)){
        for(; i >= 0 && key < bp->keys[i]; i--){
            bp->keys[i+1] = bp->keys[i];
            ((btree_leaf_node*)bp)->values[i+1] = ((btree_leaf_node*)bp)->values[i];
        }
        bp->keys[i+1] = key;
        ((btree_leaf_node*)bp)->values[i+1] = val;
        bp->nkeys++;
    }
    else{
        for(; i >= 0 && key < bp->keys[i]; i--)
            ;
        i++;
        if(((btree_index_node*)bp)->child[i]->nkeys == 2*g_bt->t-1){
            btree_split((btree_index_node*)bp, i);
            if(key >= bp->keys[i])
                i++;
        }
        btree_insert_internal(((btree_index_node*)bp)->child[i], key, val);
    }
}

void btree_insert(int key, int val)
{
    btree_node* bp = g_bt->root;
    int t = g_bt->t;
    if(bp->nkeys == 2*t-1){ // the root needs split
        btree_index_node* newbp = alloc_index_node(t, 0);
        newbp->child[0] = bp;
        btree_split(newbp, 0);
        btree_insert_internal((btree_node*)newbp, key, val);
        g_bt->root = (btree_node*)newbp;
        return ;
    }
    btree_insert_internal(bp, key, val);
}

// 一个cell就是一个key/value组合对(针对leaf node)，或者是key/ptr组合对(针对index node)
void move_cell(btree_node* dest_node, int dest_start_offset, btree_node* src_node, int src_start_offset, int nborrow)
{
    memmove(dest_node->keys+dest_start_offset, 
            src_node->keys+src_start_offset, nborrow*sizeof(int));
    if(is_leaf_node(dest_node))
        memmove(((btree_leaf_node*)dest_node)->values+dest_start_offset, 
                ((btree_leaf_node*)src_node)->values+src_start_offset, nborrow*sizeof(int));
    else
        memmove(((btree_index_node*)dest_node)->child+dest_start_offset, 
                ((btree_index_node*)src_node)->child+src_start_offset, nborrow*sizeof(btree_node*));
}

void redistribute(btree_index_node* bp, int me, int borrow_from)
{
    btree_node* me_node =  bp->child[me];
    btree_node* borrow_node =  bp->child[borrow_from];

    assert(me_node->nkeys < g_bt->t-1 &&
            borrow_node->nkeys > g_bt->t-1);
    int naverage = (me_node->nkeys + 
            borrow_node->nkeys)/2;
    int nborrow = naverage - me_node->nkeys;
    assert(nborrow < borrow_node->nkeys);

    int pkey_idx = me < borrow_from ? me : borrow_from;
    if(is_leaf_node(me_node)){
        if(me < borrow_from){
            move_cell(me_node, me_node->nkeys, borrow_node, 0, nborrow);
            bp->base.keys[pkey_idx] = borrow_node->keys[nborrow]; // up
            move_cell(borrow_node, 0, borrow_node, 
                    nborrow, borrow_node->nkeys-nborrow);
        }
        else{
            move_cell(me_node, nborrow, me_node, 0, me_node->nkeys);
            move_cell(me_node, 0, borrow_node, borrow_node->nkeys-nborrow, nborrow);
            bp->base.keys[pkey_idx] = me_node->keys[0]; // up
        }
    }
    else{
        if(me < borrow_from){
            int start = me_node->nkeys;
            me_node->keys[start] = bp->base.keys[pkey_idx];
            start++;
            move_cell(me_node, start, borrow_node, 0, nborrow-1);
            start += nborrow-1;

            ((btree_index_node*)me_node)->child[start] = ((btree_index_node*)borrow_node)->child[nborrow-1];
            bp->base.keys[pkey_idx] = borrow_node->keys[nborrow-1]; // up
            move_cell(borrow_node, 0, borrow_node, nborrow, borrow_node->nkeys-nborrow);
            // the last pointer
            int last_pos = borrow_node->nkeys-nborrow;
            ((btree_index_node*)borrow_node)->child[last_pos] = 
                ((btree_index_node*)borrow_node)->child[borrow_node->nkeys];
        }
        else{

            int last = me_node->nkeys;
            ((btree_index_node*)me_node)->child[last+nborrow] = ((btree_index_node*)me_node)->child[last];
            move_cell(me_node, nborrow, me_node, 0, me_node->nkeys);

            me_node->keys[nborrow-1] = bp->base.keys[pkey_idx];
            int pos = borrow_node->nkeys;
            ((btree_index_node*)me_node)->child[nborrow-1] = ((btree_index_node*)borrow_node)->child[pos];

            int start = borrow_node->nkeys - nborrow;
            if(nborrow-1 > 0){
                move_cell(me_node, 0, borrow_node, start, nborrow-1);
                bp->base.keys[pkey_idx] = borrow_node->keys[start-1]; // up
            }
            else
                bp->base.keys[pkey_idx] = borrow_node->keys[start]; // up
        }
    }
    me_node->nkeys += nborrow;
    borrow_node->nkeys -= nborrow;
}

void merge(btree_index_node* bp, int me, int to_merge)
{
    int left = me;
    int right = to_merge;
    if(me > to_merge){
        left = to_merge;
        right = me;
    }

    btree_node* left_node =  bp->child[left];
    btree_node* right_node =  bp->child[right];

    int start = left_node->nkeys;
    if(is_leaf_node(left_node)){
        move_cell(left_node, start, right_node, 0, right_node->nkeys);
        left_node->nkeys += right_node->nkeys;
        ((btree_leaf_node*)left_node)->next = ((btree_leaf_node*)right_node)->next;
        if(((btree_leaf_node*)right_node)->next != NULL)
            ((btree_leaf_node*)right_node)->next->prev = (btree_leaf_node*)left_node;

        if(g_bt->last == right_node)
            g_bt->last = left_node;
    }
    else{
        left_node->keys[start] = bp->base.keys[left];  // 把索引父节点的key拿下来
        start++;
        move_cell(left_node, start, right_node, 0, right_node->nkeys);
        start += right_node->nkeys;
        // the last pointer
        ((btree_index_node*)left_node)->child[start] = ((btree_index_node*)right_node)->child[right_node->nkeys];
        left_node->nkeys += start-left_node->nkeys;
    }

    free_node(right_node);
    int j;
    for(j=left+1; j<bp->base.nkeys; j++){
        bp->base.keys[j-1] = bp->base.keys[j];
        bp->child[j] = bp->child[j+1];
    }
    bp->base.nkeys--;
}

//优先考虑左边，不论是redistribute还是merge
static void balance(btree_index_node* bp, int ci)
{
    int to_merge = -1;
    if(ci > 0){
        if(bp->child[ci-1]->nkeys > g_bt->t-1){
            redistribute(bp, ci, ci-1);
            return;
        }
        to_merge = ci-1;
    }
    if (ci < bp->base.nkeys){
        if(bp->child[ci+1]->nkeys > g_bt->t-1){
            redistribute(bp, ci, ci+1);
            return;
        }
        if(to_merge < 0)
            to_merge = ci+1;
    }

    assert (to_merge >= 0);

    merge(bp, ci, to_merge);
}

static void delete_entry(btree_node* bp, int idx)
{
    int i;
    for(i=idx+1; i<bp->nkeys; i++){
        bp->keys[i-1] = bp->keys[i];
        ((btree_leaf_node*)bp)->values[i-1] = ((btree_leaf_node*)bp)->values[i];
    }
    bp->nkeys--;
}

void btree_delete_internal(btree_node* bp, int key, int* rebalance)
{
    int i = bp->nkeys - 1;
    for(; i >= 0 && key < bp->keys[i]; i--)
        ;
    if(is_leaf_node(bp)){
        *rebalance = 0;
        if(i >= 0 && key == bp->keys[i])    // found
        {
            // delete it
            delete_entry(bp, i);
            if(bp->nkeys < g_bt->t-1)
                *rebalance = 1;
        }
    }
    else{
        i++;
        int tmp;
        btree_delete_internal(((btree_index_node*)bp)->child[i], key, &tmp);
        if(tmp)
            balance((btree_index_node*)bp, i);
        *rebalance = bp->nkeys < g_bt->t-1 ? 1:0;
    }
}

void btree_delete(int key)
{
    // root doesn't need rebalancing, ignore return value
    int rebalance;
    btree_delete_internal(g_bt->root, key, &rebalance);
    assert(g_bt->root->nkeys >= 0);
    if(g_bt->root->nkeys == 0 && !is_leaf_node(g_bt->root))  // root节点只剩一根指针了
    {
        btree_node* bp = g_bt->root;
        g_bt->root = ((btree_index_node*)g_bt->root)->child[0]; // 减小了btree的深度
        free_node(bp);
    }
}

btree_node* btree_get(btree_node* bp, int key, int* slot)
{
    int i;
    for(i=0; i<bp->nkeys && key >= bp->keys[i]; i++)
        ;
    if(!is_leaf_node(bp))
        return btree_get(((btree_index_node*)bp)->child[i], key, slot);
    if(i > 0 && bp->keys[i-1] == key){
            *slot = i-1;
            return bp;
    }
    return NULL;
}

static int node_id = 0;

void btree_print_edge(FILE* fp, int src_id, int src_field, int dest_id)
{
    fprintf(fp, "\"node%d\":f%d -> \"node%d\";\n", src_id, src_field, dest_id);
}

int btree_print_leaf_node(FILE* fp, btree_node* bp)
{
    assert(bp->nkeys >= 0);
    int id = node_id++;
    bp->id = id;
    fprintf(fp, "node%d[label = \"", id);

    if(bp->nkeys > 0)
        fprintf(fp, "<f0> %d", bp->keys[0]);
    int i;
    for(i=1; i<bp->nkeys; i++){
        fprintf(fp, "|<f%d> %d", i, bp->keys[i]);
    }

    fprintf(fp, " \"];\n");

    /*
    for(i=0; i<bp->nkeys; i++){
        int dest_id = node_id++;
        fprintf("node%d[label = \"value: %d\"];\n", dest_id, 
                ((btree_leaf_node*)bp)->values[i]);
        btree_print_edge(id, i, dest_id);
    }
    */

    return id;
}

int btree_print_index_node(FILE* fp, btree_node* bp)
{
    assert(bp->nkeys > 0);
    int id = node_id++;
    bp->id = id;
    fprintf(fp, "node%d[label = \"", id);

    int i;
    for(i=0; i< bp->nkeys; i++){
        fprintf(fp, "<f%d> |<f%d> %d|", 2*i, 2*i+1, bp->keys[i]);
    }
    fprintf(fp, "<f%d> ", 2*i);

    fprintf(fp, " \"];\n");

    return id;
}

int btree_print(FILE* fp, btree_node* bp)
{
    if(is_leaf_node(bp)){
        return btree_print_leaf_node(fp, bp);
    }else{
        int src_id = btree_print_index_node(fp, bp);
        int i;
        for(i=0; i<=bp->nkeys; i++){
            int dest_id = btree_print(fp, ((btree_index_node*)bp)->child[i]);
            btree_print_edge(fp, src_id, 2*i, dest_id);
        }
        return src_id;
    }
}

static int pic_id = 1;
void btree_print_in_dot_language()
{
    char fname[1024];
    sprintf(fname, "btree_%d.dot", pic_id++);
    FILE* fp = fopen(fname, "a");

    fprintf(fp, "digraph btree {\n");
    fprintf(fp, "node [shape = record,height=.1];\n");
    btree_print(fp, g_bt->root);


    /*
    btree_leaf_node* bp = (btree_leaf_node*)g_bt->last;
    btree_leaf_node* nextbp;

    for(; bp != NULL; bp = bp->prev){
        nextbp = bp->prev;
        if(nextbp != NULL){
            fprintf(fp, "\"node%d\" -> \"node%d\";\n", bp->base.id, nextbp->base.id);
        }
    }
    */
    fprintf(fp, "}");

    fclose(fp);
}

void btree_traverse(btree_node* bp, int d)
{
    int i;
    printf("( ");
    if(is_leaf_node(bp)){
        for(i=0; i<bp->nkeys; i++)
            printf("%d,%d,%d ", bp->keys[i], ((btree_leaf_node*)bp)->values[i], d);
    }else{
        for(i=0; i<=bp->nkeys; i++){
            btree_traverse(((btree_index_node*)bp)->child[i], d+1);
            if(i < bp->nkeys)
                printf("%d,%d ", bp->keys[i], d);
        }
    }
    printf(") ");
}

void btree_linear_traverse(int backward)
{
    btree_leaf_node* bp = backward? 
        (btree_leaf_node*)g_bt->last : (btree_leaf_node*)g_bt->first;

    for(; bp != NULL; bp = backward? bp->prev : bp->next){
        int i = backward? bp->base.nkeys-1 : 0;
        for(; backward? i >= 0: i<bp->base.nkeys; backward? i-- : i++)
            printf("%d,%d ", bp->base.keys[i], bp->values[i]);
        printf(" | ");
    }
}


int main()
{
    g_bt = btree_new(2);
    char buf[1024];
    char* sep = " ";
    while(fgets(buf,sizeof(buf), stdin) != NULL){
        buf[strlen(buf)-1] = '\0'; //去掉'\n'
        char* token = strtok(buf, sep);
        int flag = 0;

        if(token == NULL)
            continue;

        if(strcmp(token, "p") == 0){
            btree_print_in_dot_language();
            continue;
        }

        if(strcmp(token, "add") == 0)
            flag = 1;
        else if(strcmp(token, "del") == 0)
            flag = 2;
        else
            return 0;

        while((token = strtok(NULL, sep)) != NULL){
            int k = atoi(token);
            switch(flag){
                case 1:
                    btree_insert(k, k);
                    break;
                case 2:
                    btree_delete(k);
                    break;
                default:
                    return 0;

            }
        }
    }

    btree_destroy(g_bt->root);

    /*
    btree_insert(100,100);
    btree_insert(250,250);
    btree_insert(270,270);
    btree_insert(260,260);
    btree_print_in_dot_language();
    */
    //btree_traverse(g_bt->root, 1);
    //btree_linear_traverse(1);
    /*
    btree_iter iter = btree_first(bp);
    for(; iter != btree_end(bp); iter = btree_next(iter))
        printf("%, %s\n", btree_key(iter), btree_value(iter));
        */
    return 0;
}
