/* 09_프로세스간통신.pdf p100~101 기반: 알림 문자열 수신 추가 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <systemd/sd-bus.h>

static int on_tick(sd_bus_message *m, void *userdata, sd_bus_error *error)
{
    (void)userdata;
    (void)error;
    uint32_t count;
    int ret = sd_bus_message_read(m, "u", &count);
    if (ret < 0) return ret;
    printf("[client] Tick(count=%u)\n", count);
    return 0;
}

static int on_notification(sd_bus_message *m, void *userdata,
                           sd_bus_error *error)
{
    (void)userdata;
    (void)error;
    const char *notice;
    int ret = sd_bus_message_read(m, "s", &notice);
    if (ret < 0) return ret;
    printf("[client] Notification: %s\n", notice);
    return 0;
}

int main(void)
{
    sd_bus *bus = NULL;
    sd_bus_message *m = NULL;
    sd_bus_slot *sigslot = NULL, *noticeslot = NULL;
    sd_bus_error error = SD_BUS_ERROR_NULL;
    const char *reply;
    uint32_t count;
    int ret = sd_bus_default_user(&bus);
    if (ret < 0) goto finish;

    ret = sd_bus_call_method(bus, "org.example.MyService",
        "/org/example/MyService", "org.example.MyService", "Hello",
        &error, &m, "s", "client");
    if (ret < 0) goto finish;
    ret = sd_bus_message_read(m, "s", &reply);
    if (ret < 0) goto finish;
    printf("[client] %s\n", reply);
    ret = sd_bus_get_property_trivial(bus, "org.example.MyService",
        "/org/example/MyService", "org.example.MyService", "Count",
        &error, 'u', &count);
    if (ret < 0) goto finish;
    printf("[client] Count = %u\n", count);

    ret = sd_bus_add_match(bus, &sigslot,
        "type='signal',sender='org.example.MyService',"
        "interface='org.example.MyService',path='/org/example/MyService',"
        "member='Tick'", on_tick, NULL);
    if (ret < 0) goto finish;
    ret = sd_bus_add_match(bus, &noticeslot,
        "type='signal',sender='org.example.MyService',"
        "interface='org.example.MyService',path='/org/example/MyService',"
        "member='Notification'", on_notification, NULL);
    if (ret < 0) goto finish;

    puts("Listening Tick and Notification signals for ~10 seconds...");
    time_t end = time(NULL) + 10;
    while (time(NULL) < end) {
        ret = sd_bus_process(bus, NULL);
        if (ret < 0) goto finish;
        if (ret == 0) {
            ret = sd_bus_wait(bus, 100000); /* 최대 100ms 대기 */
            if (ret < 0) goto finish;
        }
    }
    ret = 0;

finish:
    if (ret < 0)
        fprintf(stderr, "dbusclient: %s\n", error.message ?
                error.message : strerror(-ret));
    sd_bus_error_free(&error);
    sd_bus_message_unref(m);
    sd_bus_slot_unref(sigslot);
    sd_bus_slot_unref(noticeslot);
    sd_bus_unref(bus);
    return ret < 0 ? 1 : 0;
}
