#include "evloop.h"
#include <stdlib.h>

struct ev_loop *ev_loop_new(uint64_t id)
{
    struct ev_loop *evl = malloc(sizeof(struct ev_loop));
    evl->id = id;
    evl->base = event_base_new();
    evl->stopped = false;
    return evl;
}

void ev_loop_destroy(struct ev_loop *ev)
{
    event_base_free(ev->base);
    free(ev);
}

void ev_loop_run(struct ev_loop *ev)
{
    if (ev->stopped) return;

    event_base_dispatch(ev->base);
    event_base_loop(ev->base, EVLOOP_NONBLOCK & EVLOOP_NO_EXIT_ON_EMPTY);
}

void ev_loop_stop(struct ev_loop *ev)
{
    ev->stopped = true;
    event_base_loopbreak(ev->base);
}

void* thread_adapter(void* arg)
{
    struct ev_loop *ev = (struct ev_loop*) arg;
    ev_loop_run(ev);
    return NULL;
}
