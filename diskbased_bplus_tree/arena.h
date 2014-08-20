// arena: manage memory in a page

#define PAGE_SIZE 4096
typedef int offset_t;
typedef struct btree_node{
    int isleaf;
    int nkeys;      // invariant: t-1 <= nkeys <= 2t-1
    offset_t free_mem_header;
    offset_t cell_list; // note: 对于index node来说，cell_list的最后一个cell只有一个child，没有key
    char mem[];
}btree_node;

void arena_init(char* base);
int arena_alloc(char* base, int nbytes);
void arena_free(char* base, int mp);

void btree_node_init(char* base);
