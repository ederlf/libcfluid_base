#ifndef OF_CLIENT_H
#define OF_CLIENT_H 1

#include "base/base_of_client.h"
#include "of_conn.h"
#include "of_settings.h"

struct conn_on_hold {
    uint64_t id;
    UT_hash_handle hh;
};

struct of_client {
    struct base_of_client base;
    struct of_conn *active_conns;
    struct conn_on_hold *on_hold_conns;
    struct of_settings *ofscs;
    void *owner;
    struct timed_callback **timed_callbacks;
    void (*connection_callback)(struct of_conn* conn, enum ofconn_event event_type);
    void (*message_callback)(struct of_conn* conn, uint8_t type, void* data, size_t len);
};

struct of_client *of_client_new(uint64_t id);
void of_client_destroy(struct of_client *oc);
void of_client_add_ofsc(struct of_client *oc, struct of_settings *ofsc);
int of_client_start(struct of_client *oc, int block);
void of_client_start_conn(struct of_client *oc, uint64_t id);
void of_client_stop_conn(struct of_client *oc, uint64_t id);
void of_client_stop(struct of_client *oc);

#endif