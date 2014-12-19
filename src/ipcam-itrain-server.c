/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * ipcam-itrain-server.c
 * Copyright (C) 2014 Watson Xu <xuhuashan@gmail.com>
 *
 */

#include "itrain.h"
#include "ipcam-itrain-message.h"
#include "ipcam-itrain-connection.h"
#include "ipcam-proto-handler.h"
#include "ipcam-itrain-server.h"

struct _IpcamITrainServerPrivate
{
    IpcamITrain *itrain;
    gchar *address;
    guint port;
    gboolean terminated;
    GThread *server_thread;
    GThreadPool *thread_pool;
};


enum
{
    PROP_0,

    PROP_ITRAIN,
    PROP_ADDRESS,
    PROP_PORT
};



G_DEFINE_TYPE (IpcamITrainServer, ipcam_itrain_server, G_TYPE_OBJECT);

static gpointer itrain_server_handler(gpointer data);
static void itrain_connection_handler(gpointer data, gpointer user_data);

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
    priv->thread_pool = NULL;
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
                                       itrain_server_handler,
                                       itrain_server);

    priv->thread_pool = g_thread_pool_new(itrain_connection_handler,
                                          priv->itrain,
                                          10,
                                          FALSE,
                                          NULL);

    return obj;
}

static void
ipcam_itrain_server_finalize (GObject *object)
{
    IpcamITrainServer *itrain_server = IPCAM_ITRAIN_SERVER(object);
    IpcamITrainServerPrivate *priv = itrain_server->priv;

    g_free(priv->address);
    g_thread_pool_free(priv->thread_pool, TRUE, TRUE);
    priv->terminated = TRUE;
    g_thread_join(priv->server_thread);

    G_OBJECT_CLASS (ipcam_itrain_server_parent_class)->finalize (object);
}

static void
ipcam_itrain_server_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    IpcamITrainServer *itrain_server;
    IpcamITrainServerPrivate *priv;

    g_return_if_fail (IPCAM_IS_ITRAIN_SERVER (object));

    itrain_server = IPCAM_ITRAIN_SERVER(object);
    priv = itrain_server->priv;

    switch (prop_id)
    {
    case PROP_ITRAIN:
        priv->itrain = g_value_get_object(value);
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

typedef struct _IpcamITrainConnection IpcamITrainConnection;

static gpointer
itrain_server_handler(gpointer data)
{
    IpcamITrain *itrain;
    IpcamITrainServer *itrain_server = IPCAM_ITRAIN_SERVER(data);
    IpcamITrainServerPrivate *priv = itrain_server->priv;
    gchar *address;
    guint port;

    g_object_get(itrain_server, "itrain", &itrain, NULL);
    g_assert(IPCAM_IS_ITRAIN(itrain));

    g_object_get(itrain_server, "address", &address, "port", &port, NULL);
    GInetAddress *inet_addr = g_inet_address_new_from_string(address);
    GSocketAddress *server_addr = g_inet_socket_address_new(inet_addr, port);
    g_free(address);
    GSocket *server_sock = g_socket_new(G_SOCKET_FAMILY_IPV4,
                                        G_SOCKET_TYPE_STREAM,
                                        G_SOCKET_PROTOCOL_TCP,
                                        NULL);
    g_socket_set_blocking(server_sock, TRUE);

    g_assert(g_socket_bind(server_sock, server_addr, TRUE, NULL));
    g_assert(g_socket_listen(server_sock, NULL));

    while (!priv->terminated) {
        IpcamConnection *connection;
        GSocket *client_sock = g_socket_accept(server_sock, NULL, NULL);

        if (!client_sock) {
            g_warning("g_socket_accept() failed.\n");
            continue;
        }

        g_socket_set_blocking(client_sock, TRUE);

        connection = g_object_new(IPCAM_TYPE_CONNECTION,
                                  "user_data", itrain,
                                  "socket", client_sock,
                                  NULL);
        if (!connection) {
            g_warning("No memory for new connection.\n");
            g_socket_close(client_sock, NULL);
            continue;
        }

        if (!g_thread_pool_push(priv->thread_pool, connection, NULL)) {
            g_socket_close(client_sock, NULL);
            g_free(connection);
        }
    }

    g_socket_close(server_sock, NULL);
    g_object_unref(server_sock);

    return NULL;
}

static void
ipcam_connection_heartbeat(gpointer user_data)
{
    IpcamITrainMessage *message;
    IpcamConnection *connection = IPCAM_CONNECTION(user_data);

    message = g_object_new(IPCAM_TYPE_ITRAIN_MESSAGE, "user_data", user_data, NULL);
    ipcam_itrain_message_set_message_type (message, 0x01);
    ipcam_itrain_message_set_payload (message, NULL, 0);

    ipcam_connection_send_message (connection, message);
}

static void
itrain_connection_handler(gpointer data, gpointer user_data)
{
    IpcamITrain *itrain = IPCAM_ITRAIN(user_data);
    IpcamConnection *connection = IPCAM_CONNECTION(data);
    IpcamITrainMessage *message;
    IpcamITrainTimer *timer;

    ipcam_proto_register_all_handlers (connection);

    timer = ipcam_itrain_add_timer(itrain, 5, ipcam_connection_heartbeat, connection); 

    while ((message = ipcam_connection_get_message (connection)) != NULL) {
        ipcam_connection_dispatch_message (connection, message);
        g_object_unref(message);
    }

    ipcam_itrain_del_timer(itrain, timer);

    g_object_unref(connection);
}