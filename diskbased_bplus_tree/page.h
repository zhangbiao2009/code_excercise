#define PAGE_SIZE 4096

typedef struct{
    int fd;
    int id;
    char* base;
}page;

page* page_alloc(int fd, int page_id);
void page_free(page* p);
page* page_read(int fd, int page_id);
void page_write(page* p);

