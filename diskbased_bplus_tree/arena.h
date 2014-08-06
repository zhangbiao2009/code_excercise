// arena: manage memory in a page

typedef struct {
    int size;
    int next;
}list_node;

void arena_init(char* base, int header, int start);
int arena_alloc(char* base, int header, int nbytes);
void arena_free(char* base, int header, int mp);
