#include <string.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct{
    char* p;
    int refCount;
}string_rep;

typedef struct{
    string_rep* sp;
}string;

void string_init(string* s, const char* str)
{
    s->sp = (string_rep*) malloc(sizeof(string_rep));
    s->sp->p = str? strdup(str) : NULL;
    s->sp->refCount = 1;
}

void string_init2(string* s) { s->sp = NULL; }

void string_free(string* s)
{
    printf("in string_free\n");
    if(s->sp){
        printf("refCount:%d\n", s->sp->refCount);
        if(--s->sp->refCount == 0){
            printf("free\n");
            free(s->sp->p);
            free(s->sp);
            s->sp = NULL;
        }
    }
}

void string_copy(string* dst, string* src)
{
    if(src == dst)
        return;

    string_free(dst);
    dst->sp = src->sp;
    dst->sp->refCount++;
}

void string_print(string* s)
{
    printf("%s\n", s->sp->p);
}

int main()
{
    string s1;
    string_init(&s1, "hello");
    string s2;
    string_init2(&s2);
    string_copy(&s2, &s1);
    string_print(&s1);
    string_print(&s2);
    string_free(&s1);
    string_free(&s2);
    return 0;
}
