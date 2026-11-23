#define _POSIX_C_SOURCE 200809L
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* 강의 file_dir.c의 목록 출력에 두 번째 순회와 재귀를 더한다. */
static int list_dir(const char *path)
{
    DIR *pdir;
    struct dirent *pde;
    int i = 0, failed = 0;
    if ((pdir = opendir(path)) == NULL) {
        perror(path); return 1;
    }
    printf("%s:\n", path);
    errno = 0;
    while ((pde = readdir(pdir)) != NULL) {
        if (pde->d_name[0] == '.') continue;
        printf("%20s ", pde->d_name);
        if (++i % 3 == 0) putchar('\n');
    }
    if (errno) { perror("readdir"); failed = 1; }
    printf(i % 3 ? "\n\n" : "\n");

    /* 먼저 현재 목록을 모두 출력하고, 그다음 하위 폴더를 방문한다. */
    rewinddir(pdir);
    for (;;) {
        errno = 0;
        pde = readdir(pdir);
        if (!pde) {
            if (errno) { perror("readdir"); failed = 1; }
            break;
        }
        if (pde->d_name[0] == '.') continue;
        size_t size = strlen(path) + strlen(pde->d_name) + 2;
        char *child = malloc(size);
        if (!child) { perror("malloc"); failed = 1; break; }
        snprintf(child, size, "%s/%s", path, pde->d_name);
        struct stat buf;
        if (lstat(child, &buf) < 0) {
            perror(child); failed = 1;
        } else if (S_ISDIR(buf.st_mode)) {
            failed |= list_dir(child);
        }
        free(child);
    }
    if (closedir(pdir) < 0) { perror("closedir"); failed = 1; }
    return failed;
}

int main(int argc, char *argv[])
{
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [directory]\n", argv[0]);
        return 1;
    }
    return list_dir(argc == 2 ? argv[1] : ".");
}
