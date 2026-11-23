/* 09_프로세스간통신.pdf p22 pipetest.c 보완 */
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

#define MSGSIZE 16

int main(void)
{
    char buf[MSGSIZE] = {0};
    int p[2], i, status;
    if (pipe(p) == -1) { perror("pipe call failed"); return 1; }
    pid_t pid = fork();
    if (pid == 0) {
        close(p[0]);
        for (i = 0; i < 2; i++) {
            snprintf(buf, sizeof buf, "Hello, world #%d", i + 1);
            /* 파이프는 바이트 스트림이므로 일부만 써지면 이어 쓴다. */
            for (size_t sent = 0; sent < MSGSIZE;) {
                ssize_t n = write(p[1], buf + sent, MSGSIZE - sent);
                if (n < 0 && errno == EINTR) continue;
                if (n <= 0) { perror("write"); _exit(1); }
                sent += (size_t)n;
            }
        }
        close(p[1]);
        _exit(0);
    }
    if (pid < 0) {
        perror("fork failed"); close(p[0]); close(p[1]); return 1;
    }
    close(p[1]);
    int ok = 1;
    for (i = 0; i < 2; i++) {
        size_t received = 0;
        while (received < MSGSIZE) {
            ssize_t n = read(p[0], buf + received, MSGSIZE - received);
            if (n < 0 && errno == EINTR) continue;
            if (n <= 0) { ok = 0; break; }
            received += (size_t)n;
        }
        if (!ok) { fprintf(stderr, "Incomplete message.\n"); break; }
        buf[MSGSIZE - 1] = '\0';
        printf("%s\n", buf);
    }
    close(p[0]);
    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) { perror("waitpid"); return 1; }
    }
    return ok && WIFEXITED(status) && WEXITSTATUS(status) == 0 ? 0 : 1;
}
