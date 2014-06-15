#define PAGE_SIZE 4096

typedef struct page{
    int id;
    char* page;
}page;

page* page_alloc();
page_free(page* p);
page* read(page_id);
page_write(page* p);

