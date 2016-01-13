#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

int handle_file(const char* path)
{
    FILE* fp = fopen(path, "r");
    if(fp == NULL)
        return 0;
    int no_of_line = 0;
    
    int c;
    while((c = fgetc(fp)) != EOF){
        if(c == '\n')
            no_of_line++;
    }
    return no_of_line;
}

int tranverse(const char* path)
{
    struct stat s;
    if(stat(path, &s) < 0){
        fprintf(stderr, "path not exist or permission denied, path:%s\n", path);
        return 0;
    }
    int no_of_line = 0;
    if(s.st_mode & S_IFDIR){
        DIR* dir = opendir(path);
        if(!dir){
            fprintf(stderr, "opendir error, dir: %s\n", path);
            return 0;
        }

        struct dirent* entp;
        while((entp = readdir(dir)) != NULL){
            if(!strcmp(entp->d_name, ".") || !strcmp(entp->d_name, "..")){
                continue;
            }
            char sub_path[1024];
            strcpy(sub_path, path);
            strcat(sub_path, "/");
            strcat(sub_path, entp->d_name);
            no_of_line += tranverse(sub_path);
        }
    }
    else if(s.st_mode & S_IFREG){
        no_of_line += handle_file(path);
    }
    else{
        // other type, ignore
    }
    return no_of_line;
}

int main(int argc, char* argv[])
{
    if(argc != 2){
        fprintf(stderr, "usage: ./a.out <path>\n");
        return 0;
    }
    const char* path = argv[1];

    int no_of_line = tranverse(path);
    printf("number of line: %d\n", no_of_line);
    
    return 0;
}
