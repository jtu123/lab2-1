#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t interrupted = 0;
static void handle_signal(int number) { interrupted = number; }

int main(void)
{
    const char *texts[] = {
        "The magic thing is that you can change it.",
        "Linux is fun.",
        "Small steps matter."
    };
    const char *name = ttyname(STDIN_FILENO);
    if (!name) { fputs("Run this program in a terminal.\n", stderr); return 1; }
    int fd = open(name, O_RDWR), result = 1;
    if (fd < 0) { perror("open tty"); return 1; }
    struct termios init_attr, new_attr;
    if (tcgetattr(fd, &init_attr) < 0) {
        perror("tcgetattr"); close(fd); return 1;
    }
    struct sigaction action = {0};
    action.sa_handler = handle_signal;
    sigemptyset(&action.sa_mask);
    int signals[] = {SIGINT, SIGTERM, SIGHUP, SIGQUIT};
    for (size_t i = 0; i < sizeof(signals) / sizeof(signals[0]); i++) {
        if (sigaction(signals[i], &action, NULL) < 0) {
            perror("sigaction"); close(fd); return 1;
        }
    }
    new_attr = init_attr;
    new_attr.c_lflag &= ~(ICANON | ECHO);
    new_attr.c_cc[VMIN] = 1;
    new_attr.c_cc[VTIME] = 0;
    if (tcsetattr(fd, TCSANOW, &new_attr) < 0) {
        perror("tcsetattr"); close(fd); return 1;
    }

    struct timespec start, end;
    size_t typed = 0, errcnt = 0;
    if (clock_gettime(CLOCK_MONOTONIC, &start) < 0) {
        perror("clock_gettime"); goto restore;
    }
    for (size_t s = 0; s < sizeof(texts) / sizeof(texts[0]); s++) {
        const char *text = texts[s];
        size_t cnt = 0, length = strlen(text);
        char ch;
        printf("\nType the sentence, then press Enter:\n%s\n", text);
        fflush(stdout);
        for (;;) {
            if (interrupted) goto restore;
            ssize_t nread = read(fd, &ch, 1);
            if (nread < 0 && errno == EINTR) continue;
            if (nread <= 0) {
                fputs("Terminal read failed.\n", stderr); goto restore;
            }
            if (ch == '\n') break;
            /* 강의처럼 오타는 '*'로 표시하되 배열 범위를 검사한다. */
            int correct = cnt < length && ch == text[cnt];
            char shown = correct ? ch : '*';
            if (!correct) errcnt++;
            cnt++;
            typed++;
            if (write(fd, &shown, 1) != 1) {
                perror("write"); goto restore;
            }
        }
        if (cnt < length) errcnt += length - cnt;
        putchar('\n');
    }
    if (clock_gettime(CLOCK_MONOTONIC, &end) < 0) {
        perror("clock_gettime"); goto restore;
    }
    double seconds = end.tv_sec - start.tv_sec;
    seconds += (end.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("Errors: %zu\nTyped characters: %zu\n", errcnt, typed);
    printf("Elapsed seconds: %.2f\nAverage CPM: %.2f\n", seconds,
           seconds > 0 ? typed * 60.0 / seconds : 0);
    result = 0;
restore:
    if (interrupted) {
        fputs("\nInterrupted.\n", stderr);
        result = 128 + interrupted;
    }
    if (tcsetattr(fd, TCSANOW, &init_attr) < 0) {
        perror("restore terminal"); result = 1;
    }
    if (close(fd) < 0) { perror("close"); result = 1; }
    return result;
}
