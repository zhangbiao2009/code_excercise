#include "page.h"

static int max_page_id = 1; // page_id 0 reserved for header

page* page_alloc()
{
    // fetch from page free list, or alloc a new page
    page* p = (page*) malloc(sizeof(page));
    p->id = max_page_id++;
    return p;
}

void page_free(page* p)
{
    //
}

page* read(page_id)
{
}

void page_write(page* p)
{
}

