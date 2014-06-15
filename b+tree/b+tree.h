typedef struct btree_node{
    int isleaf;
    int nkeys;      // invariant: t-1 <= nkeys <= 2t-1
    int* keys;
    int id;     // for print in dot language
}btree_node;

typedef struct btree_index_node{
    btree_node base;
    btree_node** child;      // invariant: t <= nchild <= 2t for non-leaf nodes
}btree_index_node;
   
typedef struct btree_leaf_node{
    btree_node base;
    int* values;
    struct btree_leaf_node* prev;
    struct btree_leaf_node* next;
}btree_leaf_node;
   
typedef struct btree{
    btree_node* root;
    btree_node* first;
    btree_node* last;
    int t;  // minimum degree
}btree;

btree* btree_new(int t);
void btree_insert(int key, int val);
btree_node* btree_get(btree_node* bp, int key, int* slot);
void btree_delete(int key);
void btree_destroy();
