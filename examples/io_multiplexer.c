#include "poll.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include "stdio.h"

#define MULTITASK_IMPLEMENTATION
#include "../multitask.h"

struct pollfd polling_fds[10] = { 0 };

struct Task *in_event_callbacks[10] = { 0 };
struct Task *out_event_callbacks[10] = { 0 };

int number_of_fds = 0;

struct pollfd *fd_to_pollfd(int fd) {
    for (int i = 0; i < number_of_fds; i++) {
        if (polling_fds[i].fd == fd) {
            return &polling_fds[i];
        }
    }
}

void poll_events() {
    while (1) {
        int ready = poll(polling_fds, number_of_fds, 0);

        if (ready != 0) {
            for (int i = 0; i < number_of_fds; i++) {
                if (polling_fds[i].revents & POLLIN) {
                    multitask_queue_task(*in_event_callbacks[i]);
                }

                if (polling_fds[i].revents & POLLOUT) {
                    multitask_queue_task(*out_event_callbacks[i]);
                }
            }
        }

        multitask_yield(true);
    }
}

int setup_server_socket() {
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(25565);

    int addrlen = sizeof(address);

    int server_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    bind(server_fd, (struct sockaddr*)&address, addrlen);

    listen(server_fd, 10);

    return server_fd;
}

void set_event_callback(int fd, short events, struct Task *callback_task) {
    struct pollfd *pfd = fd_to_pollfd(fd);

    pfd->events = events;

    if (events == POLLIN) 
        in_event_callbacks[pfd - polling_fds] = callback_task;
    else
        out_event_callbacks[pfd - polling_fds] = callback_task;
}

int await_accept(int fd) {
    struct Task callback_task = multitask_create_task(&&callback, false, 0);

    set_event_callback(fd, POLLIN, &callback_task);

    printf("yielding\n");

    multitask_yield(false);
    callback: ;

    printf("called back\n");

    return accept(fd, NULL, NULL);
}

//void await_read() {}
//void await_write() {}

int main() {
    char *string = "array of characters";

    multitask_queue_task(multitask_create_task(poll_events, true, 0));

    int server_fd = setup_server_socket();

    polling_fds[0].fd = server_fd;
    number_of_fds++;

    while (1) {
        int fd = await_accept(server_fd);
        polling_fds[number_of_fds++].fd = fd;
        printf("client connected\n");
    }
    return;
}
