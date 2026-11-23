#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    pid_t pid;
    int status, exit_status;

    if ((pid = fork()) < 0) {
        perror("fork failed");
        return 1;
    }
    if (pid == 0) {
        sleep(1);
        exit(5);
    }

    /* 강의와 같이 부모는 wait로 자식의 종료 상태를 수거한다. */
    while ((pid = wait(&status)) == -1) {
        if (errno != EINTR) {
            perror("wait failed");
            return 1;
        }
    }
    if (WIFEXITED(status)) {
        exit_status = WEXITSTATUS(status);
        printf("Exit status from %ld was %d\n", (long)pid, exit_status);
        return 0;
    }
    fputs("Child did not exit normally.\n", stderr);
    return 1;
}
