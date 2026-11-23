/* 09장 p74 sendpmq.c + p75 receivepmq.c를 교대 채팅으로 확장 */
#define _POSIX_C_SOURCE 200809L
#include <fcntl.h>
#include <mqueue.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define BUFSIZE 256
#define QNAME "/my_queue"
#define PRIORITY 1

int main(int argc, char **argv)
{
    if (argc != 3 || !*argv[2] || strlen(argv[2]) > 32 ||
        strspn(argv[2], "abcdefghijklmnopqrstuvwxyz0123456789_-")
            != strlen(argv[2])) {
        fprintf(stderr, "Usage: %s 1|2|clean session\n", argv[0]);
        return 1;
    }
    char name[2][100], send_data[BUFSIZE + 2], recv_data[BUFSIZE + 1];
    for (int i = 0; i < 2; i++)
        snprintf(name[i], sizeof name[i], "%s_%lu_%s_%d", QNAME,
                 (unsigned long)getuid(), argv[2], i);
    if (!strcmp(argv[1], "clean")) {
        mq_unlink(name[0]); mq_unlink(name[1]); return 0;
    }
    int role = !strcmp(argv[1], "1") ? 1 :
               !strcmp(argv[1], "2") ? 2 : 0;
    if (!role) { fprintf(stderr, "Role must be 1 or 2.\n"); return 1; }
    mqd_t qd[2] = {(mqd_t)-1, (mqd_t)-1};
    struct mq_attr q_attr = {0}, old_q_attr;
    q_attr.mq_maxmsg = 10;
    q_attr.mq_msgsize = BUFSIZE;
    int out = role - 1, in = 2 - role, status = 1;
    unsigned int prio;
    for (int i = 0; i < 2; i++) {
        qd[i] = mq_open(name[i], O_CREAT | O_RDWR | O_NONBLOCK,
                        0600, &q_attr);
        if (qd[i] == (mqd_t)-1) { perror("mq_open failed"); goto done; }
        /* 원본 receivepmq.c처럼 비봉쇄 플래그를 해제한다. */
        q_attr.mq_flags = 0;
        if (mq_setattr(qd[i], &q_attr, NULL) < 0) {
            perror("mq_setattr failed"); goto done;
        }
    }
    if (mq_getattr(qd[in], &old_q_attr) < 0) {
        perror("mq_getattr failed"); goto done;
    }
    if (old_q_attr.mq_msgsize > BUFSIZE) {
        fprintf(stderr, "Old queue size differs. Run clean first.\n"); goto done;
    }
    if (!(old_q_attr.mq_flags & O_NONBLOCK)) puts("O_NONBLOCK not set");
    puts("Waiting for the other terminal..."); fflush(stdout);
    /* 상대도 큐를 연 뒤에만 대화와 종료 정리를 시작한다. */
    if (mq_send(qd[out], "READY", 6, PRIORITY) < 0 ||
        mq_receive(qd[in], recv_data, BUFSIZE, &prio) < 0) {
        perror("queue handshake"); goto done;
    }
    int my_turn = role == 1;
    while (1) {
        if (my_turn) {
            printf("Me> "); fflush(stdout);
            if (!fgets(send_data, sizeof send_data, stdin))
                strcpy(send_data, "/quit");
            else if (!strchr(send_data, '\n') && !feof(stdin)) {
                int ch;
                while ((ch = getchar()) != '\n' && ch != EOF) {}
                puts("Use at most 255 bytes per line."); continue;
            }
            send_data[strcspn(send_data, "\n")] = '\0';
            if (strlen(send_data) >= BUFSIZE) {
                puts("Use at most 255 bytes per line."); continue;
            }
            if (mq_send(qd[out], send_data, strlen(send_data) + 1,
                        PRIORITY) < 0) { perror("mq_send failed"); goto done; }
            if (!strcmp(send_data, "/quit")) break;
        } else {
            ssize_t n = mq_receive(qd[in], recv_data, BUFSIZE, &prio);
            if (n < 0) { perror("mq_receive failed"); goto done; }
            recv_data[n] = '\0';
            if (!strcmp(recv_data, "/quit")) { puts("Peer left."); break; }
            printf("received from message queue: %s, prio: %u\n",
                   recv_data, prio);
        }
        my_turn = !my_turn;
    }
    status = 0;
done:
    for (int i = 0; i < 2; i++) {
        if (qd[i] != (mqd_t)-1) mq_close(qd[i]);
        mq_unlink(name[i]);
    }
    return status;
}
