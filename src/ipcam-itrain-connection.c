/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * ipcam-connection.c
 * Copyright (C) 2014 Watson Xu <xuhuashan@gmail.com>
 *
 */

#include <string.h>
#include <netinet/in.h>
#include <gio/gio.h>
#include "ipcam-itrain-message.h"
#include "ipcam-itrain-connection.h"

struct _IpcamConnectionPrivate
{
     gpointer user_data;
     GSocket *socket;
     GHashTable *proto_handler;
};


enum
{
    PROP_0,

    PROP_USERDATA,
    PROP_SOCKET
};



G_DEFINE_TYPE (IpcamConnection, ipcam_connection, G_TYPE_OBJECT);

static void
ipcam_connection_init (IpcamConnection *ipcam_connection)
{
    ipcam_connection->priv = G_TYPE_INSTANCE_GET_PRIVATE (ipcam_connection, IPCAM_TYPE_CONNECTION, IpcamConnectionPrivate);

    ipcam_connection->priv->user_data = NULL;
    ipcam_connection->priv->socket = NULL;
    ipcam_connection->priv->proto_handler = g_hash_table_new(g_direct_hash, g_direct_equal);
}

static void
ipcam_connection_finalize (GObject *object)
{
    IpcamConnection *connection = IPCAM_CONNECTION(object);

    g_return_if_fail(IPCAM_IS_CONNECTION(object));

    g_hash_table_destroy (connection->priv->proto_handler);

    G_OBJECT_CLASS (ipcam_connection_parent_class)->finalize (object);
}

static void
ipcam_connection_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    IpcamConnection *connection;
    IpcamConnectionPrivate *priv;

    g_return_if_fail (IPCAM_IS_CONNECTION (object));

    connection = IPCAM_CONNECTION(object);
    priv = connection->priv;

    switch (prop_id)
    {
    case PROP_USERDATA:
        connection->priv->user_data = g_value_get_pointer(value);
        break;
    case PROP_SOCKET:
        priv->socket = g_value_get_object(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
ipcam_connection_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    IpcamConnection *connection;
    IpcamConnectionPrivate *priv;

    g_return_if_fail (IPCAM_IS_CONNECTION (object));

    connection = IPCAM_CONNECTION(object);
    priv = connection->priv;

    switch (prop_id)
    {
    case PROP_USERDATA:
        g_value_set_pointer(value, priv->user_data);
        break;
    case PROP_SOCKET:
        g_value_set_object(value, priv->socket);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
ipcam_connection_class_init (IpcamConnectionClass *klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (klass, sizeof (IpcamConnectionPrivate));

    object_class->finalize = ipcam_connection_finalize;
    object_class->set_property = ipcam_connection_set_property;
    object_class->get_property = ipcam_connection_get_property;

    g_object_class_install_property (object_class,
                                     PROP_USERDATA,
                                     g_param_spec_pointer ("user_data",
                                                           "User Data",
                                                           "User Data",
                                                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

    g_object_class_install_property (object_class,
                                     PROP_SOCKET,
                                     g_param_spec_object ("socket",
                                                          "Client Socket",
                                                          "Client Socket",
                                                          G_TYPE_SOCKET,
                                                          G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}


gpointer ipcam_connection_get_userdata(IpcamConnection *connection)
{
    IpcamConnectionPrivate *priv = connection->priv;

    return priv->user_data;
}

void
ipcam_connection_register_message_handler(IpcamConnection *connection,
                                          guint type,
                                          IpcamProtoHandler *handler)
{
    IpcamConnectionPrivate *priv = connection->priv;

    g_hash_table_replace(priv->proto_handler, GUINT_TO_POINTER(type), handler);
}

void
ipcam_connection_unregister_message_handler(IpcamConnection *connection,
                                            guint type,
                                            IpcamProtoHandler *handler)
{
    IpcamConnectionPrivate *priv = connection->priv;

    g_hash_table_remove(priv->proto_handler, GUINT_TO_POINTER(type));
}

static guint8 train_protocol_checksum(guint8 *p, guint len)
{
    guint8 checksum = 0;
    int i;

    for (i = 0; i < len; i++)
        checksum ^= p[i];

    return checksum;
}

IpcamITrainMessage *
ipcam_connection_get_message (IpcamConnection *connection)
{
    IpcamConnectionPrivate *priv = connection->priv;
    IpcamITrainMessage *message = NULL;
    GSocket *sock = priv->socket;
    guint8 buf[1024];
    gssize rcv_len;

    while (TRUE) {
        TrainProtocolHeader *head;
        gpointer payload;
        guint payload_len;
        guint msg_len;
        guint8 checksum;

        rcv_len = g_socket_receive(sock, buf, sizeof(buf), NULL, NULL);
        if (rcv_len <= 0)
            break;
        if (rcv_len <= sizeof(*head))
            continue;
        head = (TrainProtocolHeader *)buf;
        /* check start condition */
        if (head->start != 0xFF)
            continue;
        payload_len = ntohs(head->len);
        msg_len = sizeof(*head) + payload_len + 1; /* head + payload + checksum */
        /* check length */
        if (rcv_len < msg_len) {
            g_warning("message too small.\n");
            continue;
        }
        checksum = train_protocol_checksum((guint8*)buf, msg_len - 1);
        if (checksum != buf[msg_len - 1]) {
            g_warning("checksum is 0x%x, should be 0x%02x.\n", buf[msg_len - 1], checksum);
            /*g_warning("checksum not match.\n");*/
            continue;
        }
        payload = g_memdup(head->data, head->len);
        message = g_object_new(IPCAM_TYPE_ITRAIN_MESSAGE, NULL);
        ipcam_itrain_message_set_message_type (message, head->type);
        ipcam_itrain_message_set_payload (message, payload, head->len);
    }

    return message;
}

gboolean
ipcam_connection_send_message (IpcamConnection *connection, IpcamITrainMessage *message)
{
    IpcamConnectionPrivate *priv = connection->priv;
    GSocket *sock = priv->socket;
    gpointer payload;
    guint payload_len;

    payload = ipcam_itrain_message_get_payload(message, &payload_len);
    if (payload) {
        TrainProtocolHeader *head;
        guint8 *p;
        guint msg_len = sizeof(*head) + payload_len + 1;

        p = g_malloc(msg_len);
        if (p) {
            head = (TrainProtocolHeader*)p;
            head->start = 0xff;
            head->type = ipcam_itrain_message_get_message_type(message);
            head->len = htons(payload_len);
            memcpy(head->data, payload, payload_len);
            p[msg_len - 1] = train_protocol_checksum(p, msg_len - 1);
            if (g_socket_send(sock, (gchar *)p, msg_len, NULL, NULL) > 0)
                return TRUE;
        }
    }

    return FALSE;
}

gboolean
ipcam_connection_dispatch_message (IpcamConnection *connection, IpcamITrainMessage *message)
{
    IpcamConnectionPrivate *priv = connection->priv;
    IpcamProtoHandler *handler;
    guint message_type = ipcam_itrain_message_get_message_type (message);

    handler = g_hash_table_lookup(priv->proto_handler, GUINT_TO_POINTER(message_type));
    if (handler) {
        return handler(connection, message);
    }

    g_warning("Unhandled message type %d\n", message_type);

    return FALSE;
}
