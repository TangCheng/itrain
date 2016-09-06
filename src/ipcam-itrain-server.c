/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * ipcam-itrain-server.c
 * Copyright (C) 2014 Watson Xu <xuhuashan@gmail.com>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <notice_message.h>

#include "ipcam-itrain.h"
#include "ipcam-proto-interface.h"
#include "ipcam-itrain-server.h"
#include "ipcam-dctx-proto-handler.h"
#include "ipcam-dttx-proto-handler.h"


struct _IpcamITrainServerPrivate
{
    IpcamITrain *itrain;
    gchar *address;
    guint port;
    gchar *osd_address;
    guint osd_port;
    gboolean terminated;
    gboolean occlusion_stat;
    GThread *server_thread;
    GList *conn_list;
    IpcamTrainProtocolType *protocol;
    int server_sock;
    int osd_server_sock;
    int mcast_sock;
    int mcast_timer_count;
    int pipe_fds[2];
#define pipe_read_fd    pipe_fds[0]
#define pipe_write_fd   pipe_fds[1]
    int epoll_fd;
};


enum
{
    PROP_0,

    PROP_ITRAIN,
    PROP_PROTOCOL,
    PROP_ADDRESS,
    PROP_PORT,
    PROP_OSD_ADDRESS,
    PROP_OSD_PORT,
};



G_DEFINE_TYPE (IpcamITrainServer, ipcam_itrain_server, G_TYPE_OBJECT);

static gpointer itrain_server_thread_proc(gpointer data);

static void
ipcam_itrain_server_init (IpcamITrainServer *ipcam_itrain_server)
{
    ipcam_itrain_server->priv = G_TYPE_INSTANCE_GET_PRIVATE (ipcam_itrain_server, IPCAM_TYPE_ITRAIN_SERVER, IpcamITrainServerPrivate);
    IpcamITrainServerPrivate *priv = ipcam_itrain_server->priv;

    priv->itrain = NULL;
    priv->address = NULL;
    priv->port = 0;
    priv->osd_address = NULL;
    priv->osd_port = 0;
    priv->terminated = FALSE;
    priv->occlusion_stat = FALSE;
    priv->server_thread = NULL;
    priv->conn_list = NULL;
    priv->protocol = &ipcam_dctx_protocol_type;
    priv->server_sock = -1;
    priv->osd_server_sock = -1;
    priv->mcast_sock = -1;
    priv->mcast_timer_count = 0;
    priv->pipe_read_fd = -1;
    priv->pipe_write_fd = -1;
    priv->epoll_fd = -1;
}

static GObject *
ipcam_itrain_server_constructor(GType gtype,
                       guint  n_properties,
                       GObjectConstructParam *properties)
{
    GObject *obj;
    IpcamITrainServer *itrain_server;
    IpcamITrainServerPrivate *priv;

    /* Always chain up to the parent constructor */
    obj = G_OBJECT_CLASS(ipcam_itrain_server_parent_class)->constructor(gtype, n_properties, properties);

    itrain_server = IPCAM_ITRAIN_SERVER(obj);
    g_assert(IPCAM_IS_ITRAIN_SERVER(itrain_server));

    priv = itrain_server->priv;

    /* thread must be create after construction has alread initialized the properties */
    priv->terminated = FALSE;
    priv->server_thread = g_thread_new("itrain-server",
                                       itrain_server_thread_proc,
                                       itrain_server);

    return obj;
}

static void
ipcam_itrain_server_finalize (GObject *object)
{
    IpcamITrainServer *itrain_server = IPCAM_ITRAIN_SERVER(object);
    IpcamITrainServerPrivate *priv = itrain_server->priv;
    gchar *quit_cmd = "QUIT\n";

    g_free(priv->address);
    g_free(priv->osd_address);
    priv->terminated = TRUE;
    ipcam_itrain_server_send_notify(itrain_server, quit_cmd, strlen(quit_cmd));
    g_thread_join(priv->server_thread);

    G_OBJECT_CLASS (ipcam_itrain_server_parent_class)->finalize (object);
}

static void
ipcam_itrain_server_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    IpcamITrainServer *itrain_server;
    IpcamITrainServerPrivate *priv;
    const gchar *protocol;

    g_return_if_fail (IPCAM_IS_ITRAIN_SERVER (object));

    itrain_server = IPCAM_ITRAIN_SERVER(object);
    priv = itrain_server->priv;

    switch (prop_id)
    {
    case PROP_ITRAIN:
        priv->itrain = g_value_get_object(value);
        break;
    case PROP_PROTOCOL:
        protocol = g_value_get_string(value);
        if (protocol) {
            if (strcasecmp(protocol, "DTTX") == 0) {
                priv->protocol = &ipcam_dttx_protocol_type;
            }
            else if (strcasecmp(protocol, "DCTX") == 0) {
                priv->protocol = &ipcam_dctx_protocol_type;
            }
            else {
                g_warning("Invalid protocol, using default DCTX\n");
            }
        }
        else {
            g_warning("Protocol not specified, using default DCTX\n");
        }
        break;
    case PROP_ADDRESS:
        g_free(priv->address);
        priv->address =  g_value_dup_string(value);
        break;
    case PROP_PORT:
        priv->port = g_value_get_uint(value);
        break;
    case PROP_OSD_ADDRESS:
        g_free(priv->osd_address);
        priv->osd_address =  g_value_dup_string(value);
        break;
    case PROP_OSD_PORT:
        priv->osd_port = g_value_get_uint(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
ipcam_itrain_server_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    IpcamITrainServer *itrain_server;
    IpcamITrainServerPrivate *priv;

    g_return_if_fail (IPCAM_IS_ITRAIN_SERVER (object));

    itrain_server = IPCAM_ITRAIN_SERVER(object);
    priv = itrain_server->priv;

    switch (prop_id)
    {
    case PROP_ITRAIN:
        g_value_set_object(value, priv->itrain);
        break;
    case PROP_ADDRESS:
        g_value_set_string(value, priv->address);
        break;
    case PROP_PORT:
        g_value_set_uint(value, priv->port);
        break;
    case PROP_OSD_ADDRESS:
        g_value_set_string(value, priv->osd_address);
        break;
    case PROP_OSD_PORT:
        g_value_set_uint(value, priv->osd_port);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
ipcam_itrain_server_class_init (IpcamITrainServerClass *klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (klass, sizeof (IpcamITrainServerPrivate));

    object_class->constructor = ipcam_itrain_server_constructor;
    object_class->finalize = ipcam_itrain_server_finalize;
    object_class->set_property = ipcam_itrain_server_set_property;
    object_class->get_property = ipcam_itrain_server_get_property;

    g_object_class_install_property (object_class,
                                     PROP_ITRAIN,
                                     g_param_spec_object ("itrain",
                                                          "iTrain Application",
                                                          "iTrain Application",
                                                          IPCAM_TYPE_ITRAIN,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    g_object_class_install_property (object_class,
                                     PROP_PROTOCOL,
                                     g_param_spec_string ("protocol",
                                                          "Communication Protocol",
                                                          "Communication Protocol",
                                                          "DCTX",
                                                          G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));

    g_object_class_install_property (object_class,
                                     PROP_ADDRESS,
                                     g_param_spec_string ("address",
                                                          "Server Address",
                                                          "Server Address",
                                                          "0.0.0.0",
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    g_object_class_install_property (object_class,
                                     PROP_PORT,
                                     g_param_spec_uint ("port",
                                                        "Server Port",
                                                        "Server Port",
                                                        0,
                                                        G_MAXUINT,
                                                        10100,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    g_object_class_install_property (object_class,
                                     PROP_ADDRESS,
                                     g_param_spec_string ("osd-address",
                                                          "OSD Server Address",
                                                          "OSD Server Address",
                                                          "0.0.0.0",
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    g_object_class_install_property (object_class,
                                     PROP_PORT,
                                     g_param_spec_uint ("osd-port",
                                                        "OSD Server Port",
                                                        "OSD Server Port",
                                                        0,
                                                        G_MAXUINT,
                                                        10101,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

IpcamITrain *ipcam_itrain_server_get_itrain(IpcamITrainServer *itrain_server)
{
    IpcamITrainServerPrivate *priv = itrain_server->priv;

    return priv->itrain;
}

int ipcam_itrain_server_send_notify(IpcamITrainServer *itrain_server,
                                    gpointer notify,
                                    guint length)
{
    IpcamITrainServerPrivate *priv = itrain_server->priv;

    g_return_val_if_fail(notify != NULL, -1);

    return write(priv->pipe_write_fd, notify, length);
}

#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

typedef struct EpollEventHandler
{
    void (*event_handler)(struct epoll_event *event);
    gpointer data;
} EpollEventHandler;

typedef struct IpcamEpollConnection
{
    IpcamConnection     connection;
    EpollEventHandler   epoll_handler;
    IpcamITrainServer   *itrain_server;
    char                data[0];
} IpcamEpollConnection;


static void itrain_connection_epoll_handler(struct epoll_event *event);

/* IpcamConnection member functions */

static IpcamConnection *ipcam_connection_new(IpcamITrainServer *itrain_server,
                                             int sock)
{
    IpcamEpollConnection *epconn;
    IpcamITrainServerPrivate *priv = itrain_server->priv;
    IpcamTrainProtocolType *protocol = priv->protocol;

    epconn = g_malloc0(sizeof(IpcamEpollConnection) + protocol->user_data_size);

    if (!epconn) {
        g_print("No memory for new connection\n");
        return NULL;
    }

    epconn->connection.sock = sock;
    epconn->connection.itrain = priv->itrain;
    epconn->connection.priv = epconn->data;

    if (!protocol->init_connection(&epconn->connection)) {
        g_free(epconn);
        close(sock);
        return NULL;
    }

    epconn->epoll_handler.event_handler = itrain_connection_epoll_handler;
    epconn->epoll_handler.data = epconn;
    epconn->itrain_server = itrain_server;

    struct epoll_event conn_event = {
        .events = EPOLLIN | EPOLLRDHUP,
        .data = {
            .ptr = &epconn->epoll_handler
        }
    };

    /* add new connection fd to epoll */
    epoll_ctl(priv->epoll_fd, EPOLL_CTL_ADD, sock, &conn_event);

    /* add to the list */
    priv->conn_list = g_list_append(priv->conn_list, (gpointer)epconn);

    return &epconn->connection;
}

void ipcam_connection_free(IpcamConnection *conn)
{
    IpcamEpollConnection *epconn = container_of(conn, IpcamEpollConnection, connection);
    IpcamITrainServer *itrain_server = epconn->itrain_server;
    IpcamITrainServerPrivate *priv = itrain_server->priv; 
    IpcamTrainProtocolType *protocol = priv->protocol;

    priv->conn_list = g_list_remove(priv->conn_list, epconn);
    epoll_ctl(priv->epoll_fd, EPOLL_CTL_DEL, conn->sock, NULL);
    close(conn->sock);
    protocol->deinit_connection(conn);
    g_free(epconn);
}

void ipcam_connection_enable_timeout(IpcamConnection *conn, guint32 id, gboolean enabled)
{
    g_return_if_fail(id < NR_TIMEOUTS);
    conn->timeouts[id].enabled = enabled;
}

void ipcam_connection_set_timeout(IpcamConnection *conn, guint32 id, gint32 timeout_sec)
{
    IpcamTimeout *timeout;

    g_return_if_fail(id < NR_TIMEOUTS);

    timeout = &conn->timeouts[id];
    timeout->timeout_sec = timeout_sec;
    timeout->expire = time(NULL) + timeout->timeout_sec;
}

void ipcam_connection_reset_timeout(IpcamConnection *conn, guint32 id)
{
    IpcamTimeout *timeout;

    g_return_if_fail(id < NR_TIMEOUTS);

    timeout = &conn->timeouts[id];
    timeout->expire = time(NULL) + timeout->timeout_sec;
}

gssize ipcam_connection_send_pdu(IpcamConnection *conn, IpcamTrainPDU *pdu)
{
    guint8 *pkt_buffer = ipcam_train_pdu_get_packet_buffer(pdu);
    guint16 pkt_size = ipcam_train_pdu_get_packet_size(pdu);

    return send(conn->sock, (gchar *)pkt_buffer, pkt_size, 0);
}

static void
itrain_connection_epoll_handler(struct epoll_event *event)
{
    EpollEventHandler *handler = event->data.ptr;
    IpcamEpollConnection *epconn = handler->data;
    IpcamConnection *conn = &epconn->connection;
    IpcamITrainServer *itrain_server = epconn->itrain_server;
    IpcamITrainServerPrivate *priv = itrain_server->priv;
    IpcamTrainProtocolType *protocol = priv->protocol;

    if (event->events & EPOLLRDHUP) {
        /* release connection */
        ipcam_connection_free(conn);
        return;
    }

    if (event->events & EPOLLIN) {
        if (protocol->on_data_arrive) {
            protocol->on_data_arrive(conn);
        }
    }
}

static void
itrain_server_epoll_handler(struct epoll_event *event)
{
    EpollEventHandler *handler = event->data.ptr;
    IpcamITrainServer *itrain_server = (IpcamITrainServer *)handler->data;
    IpcamITrainServerPrivate *priv = itrain_server->priv;
    IpcamTrainProtocolType *protocol = priv->protocol;
    struct sockaddr_in peer_addr;
    socklen_t peer_len = sizeof(peer_addr);

    if (event->events & EPOLLIN) {
        IpcamConnection *conn;
        int cli_sock = accept(priv->server_sock,
                              (struct sockaddr *)&peer_addr,
                              &peer_len);

        if (!protocol) {
            g_print("No protocol selected, disconnect client.\n");

            close(cli_sock);

            return;
        }

        conn = ipcam_connection_new(itrain_server, cli_sock);
    }
}

void ipcam_itrain_server_report_status(IpcamITrainServer *itrain_server,
                                       gboolean occlusion_stat, 
                                       gboolean loss_stat)
{
    IpcamITrainServerPrivate *priv = itrain_server->priv;
    IpcamTrainProtocolType *protocol = priv->protocol;
    IpcamEpollConnection *epconn;
    GList *l;

    for (l = priv->conn_list; l != NULL; l = l->next) {
        epconn = l->data;
        IpcamConnection *conn = &epconn->connection;

        if (protocol && protocol->on_report_status) {
            protocol->on_report_status(conn, occlusion_stat, loss_stat);
        }
    }
}

static void
itrain_pipe_epoll_handler(struct epoll_event *event)
{
    EpollEventHandler *handler = event->data.ptr;
    IpcamITrainServer *itrain_server = (IpcamITrainServer *)handler->data;
    IpcamITrainServerPrivate *priv = itrain_server->priv;
    gchar buffer[256];

    if (event->events & EPOLLIN) {
        int bytes;

        bytes = read(priv->pipe_read_fd, buffer, sizeof(buffer) - 1);
        if (bytes > 0) {
            buffer[bytes] = 0;

            g_print(buffer);

            if (strncmp(buffer, "OCCLUSION", 9) == 0) {
                char cmd[16];
                int  region;
                int  state;

                if (sscanf(buffer, "%s %d %d", cmd, &region, &state) == 3) {
                    priv->occlusion_stat = !!state;
                    ipcam_itrain_server_report_status(itrain_server, state, 0);
                }
            }
        }
    }
}

#define MULTICAST_GROUP     ("224.0.0.88")
#define MULTICAST_PORT      (10100)

static void
itrain_server_timeout_handler(IpcamITrainServer *itrain_server)
{
    IpcamITrainServerPrivate *priv = itrain_server->priv;
    IpcamTrainProtocolType *protocol = priv->protocol;
    time_t now = time(NULL);
    GList *l;

    for (l = priv->conn_list; l != NULL; l = l->next) {
        IpcamEpollConnection *epconn = l->data;
        IpcamConnection *conn = &epconn->connection;
        int i;

        for (i = 0; i < NR_TIMEOUTS; i++) {
            IpcamTimeout *timeout = &conn->timeouts[i];
            if (!timeout->enabled)
                continue;

            if (now >= timeout->expire) {
                guint32 timeout_sec = timeout->timeout_sec;
                if (timeout_sec > 0) {
                    while(timeout->expire <= now)
                        timeout->expire += timeout_sec;
                }

                protocol->on_timeout(conn, i);
            }
        }
    }

    if (priv->mcast_timer_count == 0) {
        priv->mcast_timer_count = 10 * 5;

        const char *train_num, *position_num;
        train_num = ipcam_itrain_get_string_property(priv->itrain, "szyc:train_num");
        position_num = ipcam_itrain_get_string_property(priv->itrain, "szyc:position_num");

        if (train_num && position_num) {
            struct sockaddr_in mcast_addr;
            mcast_addr.sin_family = AF_INET;
            mcast_addr.sin_addr.s_addr = inet_addr(MULTICAST_GROUP);
            mcast_addr.sin_port = htons(MULTICAST_PORT);
            struct {
                guint32 train_num;
                guint8  position_num;
                guint8  occlusion_stat;
              guint8  loss_stat;
            } __attribute__((packed)) mcast_event;
            mcast_event.train_num = htonl(strtoul(train_num, NULL, 0));
            mcast_event.position_num = strtoul(position_num, NULL, 0);
            mcast_event.occlusion_stat = priv->occlusion_stat;
            mcast_event.loss_stat = 0;
            sendto(priv->mcast_sock, &mcast_event, sizeof(mcast_event), 0,
                   (struct sockaddr*)&mcast_addr, sizeof(mcast_addr));
        }
    }
    else {
        --priv->mcast_timer_count;
    }
}

typedef struct SetOSDRequest
{
    guint8 head;    /* 0xff */
    guint8 code;    /* function code */
    guint8 keeptime;   /* time to keep on screen */
    guint16 x;       /* X position */
    guint16 y;       /* Y position */
    guint16 fontsize;
    guint16 length;  /* max to 1024 */
    guint8 data[1024];
    guint8 csum;
} __attribute__((packed)) SetOSDRequest;

static void
itrain_osd_server_epoll_handler(struct epoll_event *event)
{
    IpcamITrain *itrain;
    EpollEventHandler *handler = event->data.ptr;
    IpcamITrainServer *itrain_server = (IpcamITrainServer *)handler->data;
    IpcamITrainServerPrivate *priv = itrain_server->priv;
    struct sockaddr_in peer_addr;
    socklen_t peer_len = sizeof(peer_addr);

    g_object_get(itrain_server, "itrain", &itrain, NULL);
    g_assert(IPCAM_IS_ITRAIN(itrain));

    if (event->events & EPOLLIN) {
        SetOSDRequest req;
        int n = recvfrom(priv->osd_server_sock, &req, sizeof(req), 0,
                         (struct sockaddr*)&peer_addr, &peer_len);
        if (n < sizeof(req)) {
            g_print("invalid set osd request\n");
            return;
        }
        if ((req.head != 0xff) || (req.code != 0x09)) {
            g_print("invalid request %02x %02x\n", (int)req.head, (int)req.code);
            return;
        }

        IpcamMessage *notice_msg;
        JsonBuilder *builder;
        JsonNode *notice_body;

        builder = json_builder_new();
        json_builder_begin_object(builder);
        json_builder_set_member_name(builder, "items");
        json_builder_begin_object(builder);
        json_builder_set_member_name(builder, "master");
        json_builder_begin_object(builder);
        json_builder_set_member_name(builder, "speed_gps");
        json_builder_begin_object(builder);
        json_builder_set_member_name(builder, "isshow");
        json_builder_add_boolean_value(builder, TRUE);
        json_builder_set_member_name(builder, "size");
        json_builder_add_int_value(builder, (guint64)ntohs(req.fontsize));
        json_builder_set_member_name(builder, "left");
        json_builder_add_int_value(builder, (guint64)ntohs(req.x));
        json_builder_set_member_name(builder, "top");
        json_builder_add_int_value(builder, (guint64)ntohs(req.y));
        json_builder_set_member_name(builder, "color");
        json_builder_begin_object(builder);
        json_builder_set_member_name(builder, "red");
        json_builder_add_int_value(builder, 0);
        json_builder_set_member_name(builder, "green");
        json_builder_add_int_value(builder, 0);
        json_builder_set_member_name(builder, "blue");
        json_builder_add_int_value(builder, 0);
        json_builder_set_member_name(builder, "alpha");
        json_builder_add_int_value(builder, 0);
        json_builder_end_object(builder); // color
        json_builder_set_member_name(builder, "text");
        json_builder_add_string_value(builder, req.data);
        json_builder_end_object(builder); // speed_gps
        json_builder_end_object(builder); // master
        json_builder_end_object(builder); // items
        json_builder_end_object(builder); // root

        notice_body = json_builder_get_root(builder);
        g_object_unref(builder);

        notice_msg = g_object_new(IPCAM_NOTICE_MESSAGE_TYPE,
                                  "event", "set_osd",
                                  "body", notice_body, NULL);
        ipcam_base_app_send_message(IPCAM_BASE_APP(itrain),
                                    notice_msg,
                                    "itrain_pub",
                                    "itrain_token",
                                    NULL,
                                    0);

        g_object_unref(notice_msg);
    }
}

static gpointer
itrain_server_thread_proc(gpointer data)
{
    IpcamITrain *itrain;
    IpcamITrainServer *itrain_server = IPCAM_ITRAIN_SERVER(data);
    IpcamITrainServerPrivate *priv = itrain_server->priv;
    gchar *address;
    gchar *osd_address;
    guint port;
    guint osd_port;
    struct epoll_event server_event;
    struct epoll_event osd_server_event;
    struct epoll_event pipe_event;
    EpollEventHandler server_handler;
    EpollEventHandler osd_server_handler;
    EpollEventHandler pipe_handler;
    int reuse_addr = 1;

    g_object_get(itrain_server, "itrain", &itrain, NULL);
    g_assert(IPCAM_IS_ITRAIN(itrain));

    /* create epoll fd */
    priv->epoll_fd = epoll_create(10);
    g_assert(priv->epoll_fd != -1);

    /* setup server socket */
    g_object_get(itrain_server, "address", &address, "port", &port, NULL);
    if (address && port) {
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        g_assert(inet_aton(address, &server_addr.sin_addr));
        server_addr.sin_port = (in_port_t)htons(port);

        priv->server_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        g_assert(priv->server_sock != -1);

        setsockopt(priv->server_sock, SOL_SOCKET, SO_REUSEADDR,
                   &reuse_addr, sizeof(reuse_addr));
        fcntl(priv->server_sock, F_SETFL, O_NONBLOCK);

        g_assert(bind(priv->server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == 0);
        g_assert(listen(priv->server_sock, 10) == 0);

        /* add server socket to epoll */
        server_handler.event_handler = itrain_server_epoll_handler;
        server_handler.data = itrain_server;

        server_event.events = EPOLLIN | EPOLLRDHUP;
        server_event.data.ptr = &server_handler;

        epoll_ctl(priv->epoll_fd,
                  EPOLL_CTL_ADD,
                  priv->server_sock,
                  &server_event);
    }
    g_free(address);

    /* setup osd server socket */
    g_object_get(itrain_server, "osd-address", &osd_address, "osd-port", &osd_port, NULL);
    if (osd_address && osd_port) {
        struct sockaddr_in osd_server_addr;
        osd_server_addr.sin_family = AF_INET;
        g_assert(inet_aton(osd_address, &osd_server_addr.sin_addr));
        osd_server_addr.sin_port = (in_port_t)htons(osd_port);

        priv->osd_server_sock = socket(PF_INET, SOCK_DGRAM, 0);
        g_assert(priv->osd_server_sock != -1);
        setsockopt(priv->osd_server_sock, SOL_SOCKET, SO_REUSEADDR,
                   &reuse_addr, sizeof(reuse_addr));
        fcntl(priv->osd_server_sock, F_SETFL, O_NONBLOCK);

        g_assert(bind(priv->osd_server_sock, (struct sockaddr*)&osd_server_addr,
                      sizeof(osd_server_addr)) == 0);

        /* add osd server socket to epoll */
        osd_server_handler.event_handler = itrain_osd_server_epoll_handler;
        osd_server_handler.data = itrain_server;

        osd_server_event.events = EPOLLIN | EPOLLRDHUP;
        osd_server_event.data.ptr = &osd_server_handler;

        epoll_ctl(priv->epoll_fd,
                  EPOLL_CTL_ADD,
                  priv->osd_server_sock,
                  &osd_server_event);
    }
    g_free(osd_address);

    /* create pipe and add it to epoll */
    g_assert(pipe(priv->pipe_fds) == 0);

    pipe_handler.event_handler = itrain_pipe_epoll_handler;
    pipe_handler.data = itrain_server;

    pipe_event.events = EPOLLIN;
    pipe_event.data.ptr = &pipe_handler;

    epoll_ctl(priv->epoll_fd,
              EPOLL_CTL_ADD,
              priv->pipe_read_fd,
              &pipe_event);

    /* setup multi-cast socket */
    priv->mcast_sock = socket(AF_INET, SOCK_DGRAM, 0);
    /* default interface to eth0 */
    struct ifreq ifr;
    strncpy(ifr.ifr_name, "eth0", IFNAMSIZ);
    if (ioctl(priv->mcast_sock, SIOCGIFINDEX, &ifr) == 0) {
        struct ip_mreqn mreqn;
        mreqn.imr_multiaddr.s_addr = inet_addr(MULTICAST_GROUP);
        mreqn.imr_address.s_addr = htonl(INADDR_ANY);
        mreqn.imr_ifindex = ifr.ifr_ifindex;
        if (setsockopt(priv->mcast_sock, IPPROTO_IP, IP_MULTICAST_IF,
                       &mreqn, sizeof(mreqn)) < 0) {
            perror("setsockopt():IP_MULTICAST_IF");
        }
    }

    while (!priv->terminated) {
        struct epoll_event ep_event;
        int ret;

        ret = epoll_wait(priv->epoll_fd, &ep_event, 1, 100);
        if (ret > 0) {
            EpollEventHandler *handler = ep_event.data.ptr;
            g_assert(handler);
            handler->event_handler(&ep_event);
        }
        else if (ret == 0) {
            /* epoll timeout */
            itrain_server_timeout_handler(itrain_server);
        }
        else {
            /* error occured */
            g_print("%s:error\n", __func__);
        }
    }

    /* free all connections */
    GList *list = priv->conn_list;
    while (list) {
        GList *next = list->next;
        IpcamEpollConnection *epconn = list->data;

        ipcam_connection_free(&epconn->connection);

        list = next;
    }
    g_list_free(priv->conn_list);

    epoll_ctl(priv->epoll_fd, EPOLL_CTL_DEL,
              priv->server_sock, NULL);
    close(priv->server_sock);
    close(priv->epoll_fd);

    return NULL;
}
