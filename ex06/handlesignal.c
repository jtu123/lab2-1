#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

static volatile sig_atomic_t received;

static void handler(int signo)
{
    received = signo;  /* printf는 일반 실행 흐름에서 호출한다. */
}

int main(void)
{
    struct sigaction act = {0};
    int i = 0;
    act.sa_handler = handler;
    sigfillset(&act.sa_mask);
    if (sigaction(SIGINT, &act, NULL) < 0) {
        perror("sigaction");
        return 1;
    }
    puts("SIGINT on. Press Ctrl+C; the program continues.");
    fflush(stdout);
    while (i < 5) {
        sleep(1);
        if (received) {
            printf("handler: signo=%d\n", (int)received);
            received = 0;
        }
        printf("Loop %d / 5\n", ++i);
        fflush(stdout);
    }
    return 0;
}
