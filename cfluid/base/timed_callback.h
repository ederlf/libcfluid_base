#ifndef TIMED_CALLBACK_H
#define TIMED_CALLBACK_H 1

#include <event2/event.h>

struct timed_callback;

struct timed_callback *tc_new(struct event_base* base, void* (*cb)                                      (void*), int interval, void *arg);
void tc_destroy(struct timed_callback *tc);

#endif  