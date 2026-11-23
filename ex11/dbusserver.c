/* 09_프로세스간통신.pdf p98~99 기반: Notification 문자열 신호 추가 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <systemd/sd-bus.h>

static uint32_t count = 0;

static int hello(sd_bus_message *m, void *userdata, sd_bus_error *error)
{
    (void)userdata;
    (void)error;
    const char *client;
    char reply[128];
    int ret = sd_bus_message_read(m, "s", &client);
    if (ret < 0) return ret;
    snprintf(reply, sizeof reply, "Hello %s! I have been called %u times.",
             client, ++count);
    return sd_bus_reply_method_return(m, "s", reply);
}

static int get_count(sd_bus *bus, const char *path, const char *interface,
                     const char *property, sd_bus_message *reply,
                     void *userdata, sd_bus_error *error)
{
    (void)bus;
    (void)path;
    (void)interface;
    (void)property;
    (void)error;
    return sd_bus_message_append(reply, "u", *(uint32_t *)userdata);
}

static const sd_bus_vtable dbusexmaple_vtable[] = {
    SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD_WITH_ARGS("Hello", SD_BUS_ARGS("s", client),
        SD_BUS_RESULT("s", reply), hello, SD_BUS_VTABLE_UNPRIVILEGED),
    /* Count는 Hello 호출마다 바뀌므로 CONST로 선언하지 않는다. */
    SD_BUS_PROPERTY("Count", "u", get_count, 0, 0),
    SD_BUS_SIGNAL("Tick", "u", 0),
    SD_BUS_SIGNAL("Notification", "s", 0),
    SD_BUS_VTABLE_END
};

int main(int argc, char *argv[])
{
    sd_bus *bus = NULL;
    sd_bus_slot *slot = NULL;
    uint32_t tickcount = 0;
    time_t last = 0;
    const char *notice = argc > 1 ? argv[1] : "Lab2 notification";

    int ret = sd_bus_default_user(&bus);
    if (ret < 0) goto finish;
    ret = sd_bus_request_name(bus, "org.example.MyService", 0);
    if (ret < 0) goto finish;
    ret = sd_bus_add_object_vtable(bus, &slot, "/org/example/MyService",
        "org.example.MyService", dbusexmaple_vtable, &count);
    if (ret < 0) goto finish;
    setvbuf(stdout, NULL, _IOLBF, 0);
    puts("[server] Ready");

    for (;;) {
        ret = sd_bus_process(bus, NULL);
        if (ret < 0) break;
        time_t now = time(NULL);
        if (last == 0 || now - last >= 1) {
            ++tickcount;
            ret = sd_bus_emit_signal(bus, "/org/example/MyService",
                "org.example.MyService", "Tick", "u", tickcount);
            if (ret < 0) break;
            ret = sd_bus_emit_signal(bus, "/org/example/MyService",
                "org.example.MyService", "Notification", "s", notice);
            if (ret < 0) break;
            printf("[server] Tick(count=%u), Notification: %s\n",
                   tickcount, notice);
            last = now;
        }
        if (ret == 0) {
            ret = sd_bus_wait(bus, 1000000); /* 최대 1초 대기 */
            if (ret < 0) break;
        }
    }

finish:
    if (ret < 0) fprintf(stderr, "dbusserver: %s\n", strerror(-ret));
    sd_bus_slot_unref(slot);
    sd_bus_unref(bus);
    return ret < 0 ? 1 : 0;
}
