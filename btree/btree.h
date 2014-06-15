typedef struct btree{
    int t;  // minimum degree
    int isleaf;
    int nkeys;      // invariant: t-1 <= nkeys <= 2t-1
    int* keys;
    struct btree** child;      // invariant: t <= nchild <= 2t for non-leaf nodes
}btree;

btree* btree_new(int t);
btree* btree_insert(btree* bp, int key);
btree* btree_get(btree* bp, int key, int* slot);
//void btree_delete(btree* t, char* key);
