#include "timed_callback.h"
#include <stdlib.h>

struct timed_callback {
    void* (*cb)(void*);
    void* cb_arg;
    void* data;
};

static void timer_callback(evutil_socket_t fd, short what, void *arg)
{
    struct timed_callback* tc = (struct timed_callback*) arg;
    tc->cb(tc->cb_arg);
}

struct timed_callback *tc_new(struct event_base* base, void* (*cb)                                      (void*), int interval, void *arg)
{
    struct timeval tv = { interval / 1000, (interval % 1000) * 1000 };
    struct timed_callback* tc = malloc(sizeof(struct timed_callback));
    tc->cb = cb;
    tc->cb_arg = arg;
    /* Is the cast necessary ? */
    struct event_base* b = (struct event_base*) base;
    struct event* ev = event_new(b,
                                 -1,
                                 EV_PERSIST,
                                 timer_callback,
                                 tc);
    tc->data = ev;
    event_add(ev, &tv);
    return tc;
}

void tc_destroy(struct timed_callback *tc)
{
    event_del((struct event*) tc->data);
    event_free((struct event*) tc->data);
    free(tc); 
}