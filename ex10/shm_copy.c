/* 09장 p83 writepshm.c + p84 readshm.c의 매핑 흐름을 파일 복사로 확장 */
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define SHMSIZE 1024
#define SHMNAME "/my_shm"

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s source destination\n", argv[0]); return 1;
    }
    int shmd = -1, source = -1, destination = -1, status = 1;
    void *shmaddr = MAP_FAILED;
    char name[64];
    struct stat src, dst;
    source = open(argv[1], O_RDONLY);
    if (source < 0) { perror("open source"); goto done; }
    if (fstat(source, &src) < 0) { perror("fstat source"); goto done; }
    if (!S_ISREG(src.st_mode)) {
        fprintf(stderr, "Source must be a regular file.\n"); goto done;
    }
    destination = open(argv[2], O_WRONLY | O_CREAT, 0644);
    if (destination < 0) { perror("open destination"); goto done; }
    if (fstat(destination, &dst) < 0) { perror("fstat destination"); goto done; }
    if (!S_ISREG(dst.st_mode) ||
        (src.st_dev == dst.st_dev && src.st_ino == dst.st_ino)) {
        fprintf(stderr, "Use a different regular destination file.\n"); goto done;
    }
    snprintf(name, sizeof name, "%s_%ld", SHMNAME, (long)getpid());
    shmd = shm_open(name, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (shmd < 0) { perror("shm_open failed"); goto done; }
    /* 열린 FD는 유지되므로 이름을 바로 제거해도 매핑할 수 있다. */
    if (shm_unlink(name) < 0) { perror("shm_unlink failed"); goto done; }
    if (ftruncate(shmd, SHMSIZE) < 0) {
        perror("ftruncate failed"); goto done;
    }
    shmaddr = mmap(0, SHMSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmd, 0);
    if (shmaddr == MAP_FAILED) { perror("mmap failed"); goto done; }
    if (ftruncate(destination, 0) < 0) { perror("truncate destination"); goto done; }
    /* 원본의 strcpy/printf 대신 매핑 영역으로 파일을 읽고 기록한다. */
    while (1) {
        ssize_t len = read(source, shmaddr, SHMSIZE);
        if (len < 0 && errno == EINTR) continue;
        if (len < 0) { perror("read"); goto done; }
        if (len == 0) break;
        for (ssize_t written = 0; written < len;) {
            ssize_t n = write(destination, (char *)shmaddr + written,
                              (size_t)(len - written));
            if (n < 0 && errno == EINTR) continue;
            if (n <= 0) {
                if (n == 0) errno = EIO;
                perror("write"); goto done;
            }
            written += n;
        }
    }
    if (close(destination) < 0) {
        destination = -1; perror("close destination"); goto done;
    }
    destination = -1;
    puts("Copy complete."); status = 0;
done:
    if (shmaddr != MAP_FAILED) munmap(shmaddr, SHMSIZE);
    if (shmd >= 0) close(shmd);
    if (source >= 0) close(source);
    if (destination >= 0) close(destination);
    return status;
}
