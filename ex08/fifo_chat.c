/* 09장 p37 readfifo.c + p38 writefifo.c를 교대 채팅으로 확장 */
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define MSGSIZE 64

int main(int argc, char **argv)
{
    if (argc != 3 || !*argv[2] || strlen(argv[2]) > 32 ||
        strspn(argv[2], "abcdefghijklmnopqrstuvwxyz0123456789_-")
            != strlen(argv[2])) {
        fprintf(stderr, "Usage: %s 1|2|clean session\n", argv[0]);
        return 1;
    }
    char testfifo[2][100], buf[MSGSIZE];
    for (int i = 0; i < 2; i++)
        snprintf(testfifo[i], sizeof testfifo[i], "fifo_%lu_%s_%d",
                 (unsigned long)getuid(), argv[2], i);
    if (!strcmp(argv[1], "clean")) {
        unlink(testfifo[0]); unlink(testfifo[1]); return 0;
    }
    int role = !strcmp(argv[1], "1") ? 1 :
               !strcmp(argv[1], "2") ? 2 : 0;
    if (!role) { fprintf(stderr, "Role must be 1 or 2.\n"); return 1; }
    for (int i = 0; i < 2; i++) {
        struct stat st;
        if (mkfifo(testfifo[i], 0600) < 0 && errno != EEXIST) {
            perror("mkfifo failed"); return 1;
        }
        if (lstat(testfifo[i], &st) < 0 || !S_ISFIFO(st.st_mode) ||
            st.st_uid != getuid()) {
            fprintf(stderr, "Invalid FIFO: %s\n", testfifo[i]); return 1;
        }
    }
    puts("Waiting for the other terminal..."); fflush(stdout);
    int fd[2]; /* fd[0]: 수신, fd[1]: 송신 */
    if (role == 1) {
        fd[1] = open(testfifo[0], O_WRONLY);
        fd[0] = open(testfifo[1], O_RDONLY);
    } else {
        fd[0] = open(testfifo[0], O_RDONLY);
        fd[1] = open(testfifo[1], O_WRONLY);
    }
    if (fd[0] < 0 || fd[1] < 0) { perror("fifo open failed"); return 1; }
    signal(SIGPIPE, SIG_IGN);
    int my_turn = role == 1, status = 0;
    while (1) {
        if (my_turn) {
            memset(buf, 0, sizeof buf);
            printf("Me> "); fflush(stdout);
            if (!fgets(buf, sizeof buf, stdin)) strcpy(buf, "/quit");
            else if (!strchr(buf, '\n') && !feof(stdin)) {
                int ch;
                while ((ch = getchar()) != '\n' && ch != EOF) {}
                puts("Use at most 62 bytes per line."); continue;
            }
            buf[strcspn(buf, "\n")] = '\0';
            size_t nwrite = 0;
            while (nwrite < MSGSIZE) {
                ssize_t n = write(fd[1], buf + nwrite, MSGSIZE - nwrite);
                if (n < 0 && errno == EINTR) continue;
                if (n <= 0) { perror("fifo write failed"); status = 1; goto done; }
                nwrite += (size_t)n;
            }
            if (!strcmp(buf, "/quit")) break;
        } else {
            size_t nread = 0;
            while (nread < MSGSIZE) {
                ssize_t n = read(fd[0], buf + nread, MSGSIZE - nread);
                if (n < 0 && errno == EINTR) continue;
                if (n < 0) { perror("fifo read failed"); status = 1; goto done; }
                if (n == 0) goto done;
                nread += (size_t)n;
            }
            buf[MSGSIZE - 1] = '\0';
            if (!strcmp(buf, "/quit")) { puts("Peer left."); break; }
            printf("received message: %s\n", buf);
        }
        my_turn = !my_turn;
    }
done:
    close(fd[0]); close(fd[1]);
    unlink(testfifo[0]); unlink(testfifo[1]);
    return status;
}
