#define _POSIX_C_SOURCE 200809L
#include <dirent.h>
#include <errno.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    DIR *pdir;
    struct dirent *pde;
    int i = 0, failed = 0;
    if (argc != 2) {
        fprintf(stderr, "Usage: %s dirname\n", argv[0]);
        return 1;
    }
    if ((pdir = opendir(argv[1])) == NULL) {
        perror("opendir"); return 1;
    }
    errno = 0;
    while ((pde = readdir(pdir)) != NULL) {
        printf("%20s ", pde->d_name);
        if (++i % 3 == 0) putchar('\n');
    }
    if (errno) { perror("readdir"); failed = 1; }
    putchar('\n');
    if (closedir(pdir) < 0) { perror("closedir"); failed = 1; }
    return failed;
}
