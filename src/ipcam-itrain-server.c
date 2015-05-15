/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * ipcam-itrain-server.c
 * Copyright (C) 2014 Watson Xu <xuhuashan@gmail.com>
 *
 */

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

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
    gboolean terminated;
    GThread *server_thread;
    GList *conn_list;
    IpcamTrainProtocolType *protocol;
    int server_sock;
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
    PROP_PORT
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
    priv->terminated = FALSE;
    priv->server_thread = NULL;
    priv->conn_list = NULL;
    priv->protocol = &ipcam_dctx_protocol_type;
    priv->server_sock = -1;
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
                    ipcam_itrain_server_report_status(itrain_server, state, 0);
                }
            }
        }
    }
}

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
}

static gpointer
itrain_server_thread_proc(gpointer data)
{
    IpcamITrain *itrain;
    IpcamITrainServer *itrain_server = IPCAM_ITRAIN_SERVER(data);
    IpcamITrainServerPrivate *priv = itrain_server->priv;
    gchar *address;
    guint port;
    struct epoll_event server_event;
    struct epoll_event pipe_event;
    EpollEventHandler server_handler;
    EpollEventHandler pipe_handler;
    int reuse_addr = 1;

    g_object_get(itrain_server, "itrain", &itrain, NULL);
    g_assert(IPCAM_IS_ITRAIN(itrain));

    g_object_get(itrain_server, "address", &address, "port", &port, NULL);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    g_assert(inet_aton(address, &server_addr.sin_addr));
    server_addr.sin_port = (in_port_t)htons(port);
    g_free(address);

    priv->server_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    g_assert(priv->server_sock != -1);

    setsockopt(priv->server_sock, SOL_SOCKET, SO_REUSEADDR,
               &reuse_addr, sizeof(reuse_addr));
    fcntl(priv->server_sock, F_SETFL, O_NONBLOCK);

    g_assert(bind(priv->server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == 0);
    g_assert(listen(priv->server_sock, 10) == 0);

    /* create epoll fd */
    priv->epoll_fd = epoll_create(10);
    g_assert(priv->epoll_fd != -1);

    /* add server socket to epoll */
    server_handler.event_handler = itrain_server_epoll_handler;
    server_handler.data = itrain_server;

    server_event.events = EPOLLIN | EPOLLRDHUP;
    server_event.data.ptr = &server_handler;

    epoll_ctl(priv->epoll_fd,
              EPOLL_CTL_ADD,
              priv->server_sock,
              &server_event);

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
