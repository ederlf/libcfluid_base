#include "of_client.h"
#include "base/of.h"
#include "base/vector.h"

static void try_connect(struct of_client *oc, struct of_settings *ofsc);
static void* send_echo(void* arg);
static void* try_connect_on_hold(void* arg);
static void base_message_callback(struct base_of_conn* c, 
                        void* data, size_t len);
static void base_connection_callback( struct base_of_conn* c, 
                              enum conn_event event_type);
static void free_data(void *data);

static void connection_callback(struct of_conn *conn, 
                                enum ofconn_event event);

static void message_callback(struct of_conn* conn, 
                             uint8_t type, void* data, size_t len);

struct of_client *of_client_new(uint64_t id)
{
    struct of_client *ofc = malloc(sizeof(struct of_client));
    base_of_client_init(&ofc->base, id);
    ofc->ofscs = NULL;
    ofc->active_conns = NULL;
    ofc->on_hold_conns = NULL;
    ofc->timed_callbacks = NULL;
    ofc->owner = NULL;
    ofc->base.ofh.base_connection_callback = base_connection_callback;
    ofc->base.ofh.base_message_callback = base_message_callback;
    ofc->base.ofh.free_data = free_data;
    ofc->connection_callback = connection_callback;
    ofc->message_callback = message_callback;
    return ofc;
}

void of_client_destroy(struct of_client *oc)
{
    struct of_conn *cconn, *tmp;
    struct of_settings *ofsc, *ofsc_tmp;
    struct conn_on_hold *coh, *tmp_coh;
    base_of_client_clean(&oc->base);
    HASH_ITER(hh, oc->ofscs, ofsc, ofsc_tmp) {
        /* Free the hash node before the element */
        HASH_DEL(oc->ofscs, ofsc);
        of_settings_destroy(ofsc);
    }
    HASH_ITER(hh, oc->active_conns, cconn, tmp) {
        HASH_DEL(oc->active_conns, cconn);
        of_conn_destroy(cconn);
    }
    HASH_ITER(hh, oc->on_hold_conns, coh, tmp_coh) {
        HASH_DEL(oc->on_hold_conns, coh);
        free(coh);
    }
    free(oc);
}

void of_client_add_ofsc(struct of_client *oc, struct of_settings *ofsc)
{
    HASH_ADD(hh, oc->ofscs, datapath_id, sizeof(uint64_t), ofsc);    
}

int of_client_start(struct of_client *oc, int block)
{
    int i, ret;
    struct of_settings *ofsc, *ofsc_tmp;
    /* Connect for every datapath present */
    HASH_ITER(hh, oc->ofscs, ofsc, ofsc_tmp) {
        try_connect(oc, ofsc);
    }
    ret = base_of_client_start(&oc->base, block);
    vector_push_back(oc->timed_callbacks, tc_new(oc->base.evloop->base, try_connect_on_hold, 5000, oc));
    return ret;
}

void of_client_start_conn(struct of_client *oc, uint64_t id){
    // base_of_client_start_conn(&oc->base);
}

void of_client_stop_conn(struct of_client *oc, uint64_t id){
    struct of_conn *conn;
    HASH_FIND(hh, oc->active_conns, &id, sizeof(uint64_t), conn);
    if (conn != NULL){
        HASH_DEL(oc->active_conns, conn);
        of_conn_close(conn);
        of_conn_destroy(conn);
    }
}

void of_client_stop(struct of_client *oc) {
    struct of_conn *cconn, *tmp;
    struct timed_callback **v = oc->timed_callbacks;
    HASH_ITER(hh, oc->active_conns, cconn, tmp) {
        of_client_stop_conn(oc, cconn->id);
    }
    /* Clean callbacks */
    if(oc->timed_callbacks) {
        struct timed_callback **it;
        for(it = vector_begin(v); it != vector_end(v); ++it) {
            tc_destroy(*it); 
        }
        vector_free(oc->timed_callbacks);
    }
    base_of_client_stop(&oc->base);
}

static void try_connect(struct of_client *oc, struct of_settings *ofsc)
{
    int sock;
    struct sockaddr_in echoserver;

    /* Create the TCP socket */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        fprintf(stderr, "Error creating socket");    
    }
    else { 
        memset(&echoserver, 0, sizeof(echoserver));
        echoserver.sin_family = AF_INET;
        echoserver.sin_addr.s_addr = inet_addr(ofsc->address);
        echoserver.sin_port = htons(ofsc->port);
        if (connect(sock, (struct sockaddr *) &echoserver, sizeof(echoserver)) < 0) {
            struct conn_on_hold *coh;
            HASH_FIND(hh, oc->on_hold_conns, 
                      &ofsc->datapath_id, sizeof(uint64_t), coh);
            if (!coh){
                coh = malloc(sizeof(struct conn_on_hold));
                coh->id = ofsc->datapath_id;
                HASH_ADD(hh, oc->on_hold_conns, id, sizeof(uint64_t), coh);
            }
            close(sock);
        }
        else {
            struct base_of_conn* c = base_of_conn_new(ofsc->datapath_id,
                                                   (struct base_of_handler*) &oc->base,
                                                   oc->base.evloop,
                                                   &oc->base,
                                                   sock);
        }
    }
}

static void base_message_callback(struct base_of_conn* c, 
                        void* data, size_t len) {
    uint8_t type = ((uint8_t*) data)[1];
    struct of_conn *cc = (struct of_conn*) c->manager;
    struct of_client *ofc = (struct of_client*) c->owner;
    struct of_settings *ofsc;
    // We trust that the other end is using the negotiated protocol
    // version. Should we?

    /* Jump to the end if there is not a configuration for the id */
    HASH_FIND(hh, ofc->ofscs, &cc->id, sizeof(uint64_t), ofsc);
    if(!ofsc){
        goto done;
    }

    if (ofsc->liveness_check && type == OFPT_ECHO_REQUEST) {
        uint8_t msg[8];
        memset((void*) msg, 0, 8);
        msg[0] = ((uint8_t*) data)[0];
        msg[1] = OFPT_ECHO_REPLY;
        ((uint16_t*) msg)[1] = htons(8);
        ((uint32_t*) msg)[1] = ((uint32_t*) data)[1];
        // TODO: copy echo data
        of_conn_send(cc, msg, 8);

        if (ofsc->dispatch_all_messages) goto dispatch; else goto done;
    }

    if (ofsc->handshake && type == OFPT_HELLO) {
        uint8_t version = ((uint8_t*) data)[0];
        if (!ofsc->supported_versions & (1 << (version - 1))) {
            uint8_t msg[12];
            memset((void*) msg, 0, 8);
            msg[0] = version;
            msg[1] = OFPT_ERROR;
            ((uint16_t*) msg)[1] = htons(12);
            ((uint32_t*) msg)[1] = ((uint32_t*) data)[1];
            ((uint16_t*) msg)[4] = htons(OFPET_HELLO_FAILED);
            ((uint16_t*) msg)[5] = htons(OFPHFC_INCOMPATIBLE);
            of_conn_send(cc, msg, 12);
            of_conn_close(cc);
            cc->state = STATE_FAILED;
            ofc->connection_callback(cc, OF_EVENT_FAILED_NEGOTIATION);
        } else {
            if (ofsc->is_controller) {
                struct ofp_header msg;
                msg.version = ((uint8_t*) data)[0];
                msg.type = OFPT_FEATURES_REQUEST;
                msg.length = htons(8);
                msg.xid = ((uint32_t*) data)[1];
                of_conn_send(cc, &msg, 8);
            }
        }

        if (ofsc->dispatch_all_messages) goto dispatch; else goto done;
    }

    if (ofsc->liveness_check && type == OFPT_ECHO_REPLY) {
        if (ntohl(((uint32_t*) data)[1]) == ECHO_XID) {
            cc->alive = true;
        }

        if (ofsc->dispatch_all_messages) goto dispatch; else goto done;
    }

    if (ofsc->handshake && !ofsc->is_controller && type == OFPT_FEATURES_REQUEST) {
        struct ofp_switch_features reply;
        memset(&reply, 0x0, sizeof(reply));
        cc->version = (((uint8_t*) data)[0]);
        cc->state = (STATE_RUNNING);
        reply.header.version = ((uint8_t*) data)[0];
        reply.header.type = OFPT_FEATURES_REPLY;
        reply.header.length = htons(sizeof(reply));
        reply.header.xid = ((uint32_t*) data)[1];
        /* Hton for 64 bits */
        reply.datapath_id =  (((uint64_t)htonl(ofsc->datapath_id)) << 32) +
                             htonl(ofsc->datapath_id >> 32); 
        reply.n_buffers = htonl(ofsc->n_buffers);
        reply.n_tables = ofsc->n_tables;
        reply.auxiliary_id = ofsc->auxiliary_id;
        reply.capabilities = htonl(ofsc->capabilities);
        of_conn_send(cc, &reply, sizeof(reply));

        if (ofsc->liveness_check){
            vector_push_back(c->timed_callbacks, tc_new(ofc->base.evloop->base, send_echo, ofsc->echo_interval * 1000, cc));
        }
        ofc->connection_callback(cc, OF_EVENT_ESTABLISHED);

        if (ofsc->dispatch_all_messages) goto dispatch; else goto done;
    }

    // Handle feature replies
    if (ofsc->handshake && ofsc->is_controller && type == OFPT_FEATURES_REPLY) {
        cc->version = (((uint8_t*) data)[0]);
        cc->state = (STATE_RUNNING);
        if (ofsc->liveness_check)
            vector_push_back(c->timed_callbacks, tc_new(ofc->base.evloop->base, send_echo, ofsc->echo_interval * 1000, cc));
        ofc->connection_callback(cc, OF_EVENT_ESTABLISHED);
        goto dispatch;
    }


    goto dispatch;

    // Dispatch a message and goto done
    dispatch:
        ofc->message_callback(cc, type, data, len);
        goto done;
    // Free the message and return
    done:
        free(data);
        // c->free_data(data);
        return;
}

static void base_connection_callback(struct base_of_conn* c, 
                              enum conn_event event_type) {
    /* If the connection was closed, destroy it.
    There's no need to notify the user, since a DOWN event already
    means a CLOSED event will happen and nothing should be expected from
    the connection. */
    struct of_client *ofc = (struct of_client*) c->owner;
    struct of_settings *ofsc;
    struct of_conn *conn;

    HASH_FIND(hh, ofc->ofscs, &c->id, sizeof(uint64_t), ofsc);
    if(!ofsc){
        return;
    }

    if (event_type == EVENT_CLOSED) {
        base_of_client_base_connection_callback(c, event_type);
        return;
    }

    uint64_t conn_id = c->id;
    if (event_type == EVENT_UP) {
        if (ofsc->handshake) {
            struct ofp_hello msg;
            msg.header.version = ofsc->max_supported_version;
            msg.header.type = OFPT_HELLO;
            msg.header.length = htons(8);
            msg.header.xid = htonl(HELLO_XID);
            base_of_conn_send(c, &msg, 8);
        }
        struct conn_on_hold *coh;
        HASH_FIND(hh, ofc->on_hold_conns, &conn_id, sizeof(uint64_t), coh);
        if (coh){
            HASH_DEL(ofc->on_hold_conns, coh);
            free(coh);
        }
        conn = of_conn_new(c);
        HASH_ADD(hh, ofc->active_conns, id, sizeof(uint64_t), conn );
        ofc->connection_callback(conn, OF_EVENT_STARTED);
    }
    else if (event_type == EVENT_DOWN) {
        HASH_FIND(hh, ofc->active_conns, &conn_id, sizeof(uint64_t), conn);
        ofc->connection_callback(conn, OF_EVENT_CLOSED);
    }
}

static void free_data(void *data)
{
    base_of_client_free_data(data);
}

static void* try_connect_on_hold(void* arg) {
    struct conn_on_hold *coh, *tmp;
    struct of_client *ofc = (struct of_client*) arg;
    struct of_settings *ofsc;
    HASH_ITER(hh, ofc->on_hold_conns, coh, tmp){
        HASH_FIND(hh, ofc->ofscs, &coh->id, sizeof(uint64_t), ofsc);
        if(ofsc){
            try_connect(ofc, ofsc);
        }
    }
    return NULL;
}

static void* send_echo(void* arg) {
    struct of_conn *cc = (struct of_conn*) arg;
    struct of_client *ofc = (struct of_client*) cc->conn->owner;
    if (!cc->alive) {
        of_conn_close(cc);
        ofc->connection_callback(cc, OF_EVENT_DEAD);
        return NULL;
    }

    uint8_t msg[8];
    memset((void*) msg, 0, 8);
    msg[0] = (uint8_t) cc->version;
    msg[1] = OFPT_ECHO_REQUEST;
    ((uint16_t*) msg)[1] = htons(8);
    ((uint32_t*) msg)[1] = htonl(ECHO_XID);

    cc->alive = false;
    of_conn_send(cc, msg, 8);

    return NULL;
}

static void connection_callback(struct of_conn *conn, 
                                enum ofconn_event event_type)
{
    struct of_client *oc = (struct of_client*) conn->conn->owner;
    if(event_type == OF_EVENT_CLOSED){
        struct conn_on_hold *coh;
        uint64_t id = conn->id;
        HASH_FIND(hh, oc->on_hold_conns, &id, sizeof(uint64_t), coh);
        if (!coh){
            coh = malloc(sizeof(struct conn_on_hold));
            coh->id = id;
            HASH_ADD_INT(oc->on_hold_conns, id, coh);
        }
        of_client_stop_conn(oc, id);
    }
}

static void message_callback(struct of_conn* conn, 
                             uint8_t type, void* data, size_t len)
{
    printf("Message type %d\n", type);
}
