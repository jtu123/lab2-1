#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#define MAX_READ 10

int main(int argc, char *argv[])
{
    int src_fd = -1, dst_fd = -1, result = 1;
    char buf[MAX_READ];
    ssize_t rcnt, tot_cnt = 0;
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    struct stat src_stat, dst_stat;
    if (argc != 3) {
        fprintf(stderr, "Usage: %s src_file dest_file\n", argv[0]);
        return 1;
    }
    src_fd = open(argv[1], O_RDONLY);
    if (src_fd < 0) { perror("src open"); goto done; }
    /* 같은 파일인지 검사한 뒤에만 대상 파일을 비운다. */
    dst_fd = open(argv[2], O_WRONLY | O_CREAT, mode);
    if (dst_fd < 0) { perror("dst open"); goto done; }
    if (fstat(src_fd, &src_stat) || fstat(dst_fd, &dst_stat)) {
        perror("fstat"); goto done;
    }
    if (src_stat.st_dev == dst_stat.st_dev &&
        src_stat.st_ino == dst_stat.st_ino) {
        fputs("Source and destination are the same file.\n", stderr);
        goto done;
    }
    if (ftruncate(dst_fd, 0) < 0) { perror("ftruncate"); goto done; }

    while ((rcnt = read(src_fd, buf, MAX_READ)) != 0) {
        if (rcnt < 0) {
            if (errno == EINTR) continue;
            perror("read"); goto done;
        }
        /* write가 일부만 기록한 경우 남은 바이트를 계속 쓴다. */
        ssize_t offset = 0;
        while (offset < rcnt) {
            ssize_t wcnt = write(dst_fd, buf + offset, rcnt - offset);
            if (wcnt < 0 && errno == EINTR) continue;
            if (wcnt <= 0) {
                fputs("write failed\n", stderr); goto done;
            }
            offset += wcnt;
            tot_cnt += wcnt;
        }
    }
    printf("total write count = %zd\n", tot_cnt);
    result = 0;
done:
    if (src_fd >= 0 && close(src_fd) < 0) {
        perror("src close"); result = 1;
    }
    if (dst_fd >= 0 && close(dst_fd) < 0) {
        perror("dst close"); result = 1;
    }
    return result;
}
