#ifndef BASE_OF_CONN
#define BASE_OF_CONN 1

#include "evloop.h"
#include "ofp_buffer.h"
#include "timed_callback.h"


struct leventbaseconn;
struct base_of_handler;

enum conn_event {
        /** The connection has been successfully established */
        EVENT_UP,
        /** The other end has ended the connection */
        EVENT_DOWN,
        /** The connection resources have been released and freed */
        EVENT_CLOSED
    };

struct base_of_conn {
    uint64_t id;
    struct ev_loop *evl;
    struct ofp_buffer *ofb;
    struct base_of_handler *ofh;
    void *manager;
    void *owner;
    int running;
    struct timed_callback **timed_callbacks; /* dynamic array of callbacks */
    struct leventbaseconn *lev_base; 
};

struct base_of_handler {
    void (*base_connection_callback) (struct base_of_conn *conn,
                                    enum conn_event event_type);
    void (*base_message_callback) (struct base_of_conn* conn,
                                       void* data,
                                       size_t len);
    void (*free_data) (void* data);                            
};

struct base_of_conn *base_of_conn_new(uint64_t id,
                        struct base_of_handler *ofhandler,
                        struct ev_loop* evloop,
                        void *owner,
                        int fd);
void base_of_conn_destroy(struct base_of_conn *conn);
void base_of_conn_send(struct base_of_conn *conn, void* data, size_t len);
void base_of_conn_close(struct base_of_conn *conn);

#endif