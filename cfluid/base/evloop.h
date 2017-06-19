#ifndef EVLOOP_H
#define EVLOOP_H 1

#include <stdint.h>
#include <stdbool.h>
#include <event2/event.h>

struct ev_loop {
    uint64_t id;
    bool stopped;
    struct event_base *base;
};

struct ev_loop *ev_loop_new(uint64_t id);
void ev_loop_destroy(struct ev_loop *ev);
void ev_loop_run(struct ev_loop *ev);
void ev_loop_stop(struct ev_loop *ev);
void *thread_adapter(void* arg);

#endif