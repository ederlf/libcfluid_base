#include "base_of_client.h"
#include <event2/event-config.h>

void base_of_client_init(struct base_of_client *oc, uint64_t id)
{
    evthread_use_pthreads();
    oc->id = id;    
    oc->evloop = ev_loop_new(0);
}

void base_of_client_clean(struct base_of_client *oc)
{
    ev_loop_destroy(oc->evloop);
}

int base_of_client_start(struct base_of_client *bofc, int block)
{
    bofc->blocking = block;
    if (!bofc->blocking) {
        pthread_create(&bofc->t,
                       NULL,
                       thread_adapter,
                       bofc->evloop);
    }
    else {
        ev_loop_run(bofc->evloop);
    }
    return 1;
}

void base_of_client_stop(struct base_of_client *bofc) {
    ev_loop_stop(bofc->evloop);
    if (!bofc->blocking) {
        pthread_join(bofc->t, NULL);
    }
}

void base_of_client_base_connection_callback(struct base_of_conn *conn, enum conn_event event_type) {
    if (event_type == EVENT_CLOSED) {
        base_of_conn_destroy(conn);
    }
}

void base_of_client_free_data(void *data)
{
    free(data);
}
