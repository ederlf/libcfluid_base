#ifndef BASE_OF_CLIENT
#define BASE_OF_CLIENT 1

#include "base_of_conn.h"
#include <pthread.h>

struct base_of_client {
    struct base_of_handler ofh;
    uint64_t id;
    int blocking;
    struct ev_loop *evloop;
    pthread_t t;
};

void base_of_client_init(struct base_of_client *oc, uint64_t id);
void base_of_client_clean(struct base_of_client *oc);
int base_of_client_start(struct base_of_client *conn, int block);
void base_of_client_stop(struct base_of_client *conn);

/* Need to initialize  these 3 to ofhandle */
void base_of_client_start_conn(struct base_of_client *conn);
void base_of_client_base_connection_callback(struct base_of_conn *conn, enum conn_event event_type);
void base_of_client_free_data(void *data);

#endif