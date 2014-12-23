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

     gpointer buffer;
     guint buffer_size;
     guint data_len;
};


enum
{
    PROP_0,

    PROP_USERDATA,
    PROP_SOCKET,
    PROP_BUFFER_SIZE
};

#define PACKET_START        0xFF
#define DEFAULT_BUFFER_SIZE 1024

G_DEFINE_TYPE (IpcamConnection, ipcam_connection, G_TYPE_OBJECT);

static void
ipcam_connection_init (IpcamConnection *ipcam_connection)
{
    IpcamConnectionPrivate *priv;

    priv = G_TYPE_INSTANCE_GET_PRIVATE (ipcam_connection, IPCAM_TYPE_CONNECTION, IpcamConnectionPrivate);
    ipcam_connection->priv = priv;

    priv->user_data = NULL;
    priv->socket = NULL;
    priv->proto_handler = g_hash_table_new(g_direct_hash, g_direct_equal);
    priv->buffer = g_malloc(DEFAULT_BUFFER_SIZE);
    priv->buffer_size = DEFAULT_BUFFER_SIZE;
    priv->data_len = 0;
}

static void
ipcam_connection_finalize (GObject *object)
{
    IpcamConnection *connection = IPCAM_CONNECTION(object);

    g_return_if_fail(IPCAM_IS_CONNECTION(object));

    g_hash_table_destroy (connection->priv->proto_handler);

    g_free(connection->priv->buffer);

    G_OBJECT_CLASS (ipcam_connection_parent_class)->finalize (object);
}

static void
ipcam_connection_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    IpcamConnection *connection;
    IpcamConnectionPrivate *priv;
    guint buffer_size;

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
    case PROP_BUFFER_SIZE:
        buffer_size = g_value_get_uint(value);
        if (buffer_size > 0 && buffer_size != priv->buffer_size) {
            priv->buffer_size = g_value_get_uint(value);
            priv->buffer = g_realloc(priv->buffer, buffer_size);
            if (priv->data_len > priv->buffer_size)
                priv->data_len = priv->buffer_size;
        }
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
    case PROP_BUFFER_SIZE:
        g_value_set_uint(value, priv->buffer_size);
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

    g_object_class_install_property (object_class,
                                     PROP_BUFFER_SIZE,
                                     g_param_spec_uint ("buffer_size",
                                                        "Receive Buffer Size",
                                                        "Receive Buffer Size",
                                                        128,
                                                        G_MAXUINT16,
                                                        1024,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
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
    gssize rcv_len;
    guint data_len = priv->data_len;

    g_socket_set_blocking(sock, TRUE);

    while (TRUE) {
        TrainProtocolHeader *head;
        gpointer payload;
        guint payload_len;
        guint msg_len;
        guint8 checksum;

        if (data_len < sizeof(*head)) {
            rcv_len = g_socket_receive(sock, priv->buffer + data_len,
                                       priv->buffer_size - data_len,
                                       NULL, NULL);
            if (rcv_len <= 0) {
                priv->data_len = 0;
                return NULL;
            }

            data_len += rcv_len;

            continue;
        }

        head = (TrainProtocolHeader *)priv->buffer;
        /* lookup start condition */
        if ( head->start != PACKET_START) {
            while (head->start != PACKET_START && data_len > 0) {
                head = (TrainProtocolHeader *)((gchar *)head + 1);
                data_len--;
            }
            if (data_len > 0) {
                memmove(priv->buffer, head, data_len);
                head = (TrainProtocolHeader *)priv->buffer;
            }
            if (data_len < sizeof(*head)) {
                continue;
            }
        }

        payload_len = ntohs(head->len);
        msg_len = sizeof(*head) + payload_len + 1; /* head + payload + checksum */
        /* discard very large packet */
        if (msg_len > priv->buffer_size) {
            data_len = 0;
            continue;
        }
        /* check length */
        while (data_len < msg_len) {
            rcv_len = g_socket_receive(sock, priv->buffer + data_len,
                                       priv->buffer_size - data_len,
                                       NULL, NULL);
            if (rcv_len <= 0) {
                priv->data_len = 0;
                return NULL;
            }
            data_len += rcv_len;
        }

        checksum = train_protocol_checksum((guint8*)head, msg_len - 1);
        if (checksum != ((guint8*)head)[msg_len - 1]) {
            g_warning("checksum not match [0x%02x=>0x%02x].\n",
                      ((guint8*)head)[msg_len - 1], checksum);
            data_len -= msg_len;
            if (data_len) {
                memmove(priv->buffer, priv->buffer + msg_len, data_len);
            }
            continue;
        }

        payload = g_memdup(head->data, payload_len);
        message = g_object_new(IPCAM_TYPE_ITRAIN_MESSAGE, NULL);
        ipcam_itrain_message_set_message_type (message, head->type);
        ipcam_itrain_message_set_payload (message, payload, payload_len);

        data_len -= msg_len;
        if (data_len) {
            memmove(priv->buffer, priv->buffer + msg_len, data_len);
        }
        priv->data_len = data_len;

        return message;
    }

    return NULL;
}

gboolean
ipcam_connection_send_message (IpcamConnection *connection, IpcamITrainMessage *message)
{
    IpcamConnectionPrivate *priv = connection->priv;
    TrainProtocolHeader *head;
    GSocket *sock = priv->socket;
    gpointer payload;
    guint payload_len;
    guint8 *msg;

    payload = ipcam_itrain_message_get_payload(message, &payload_len);

    guint msg_len = sizeof(*head) + payload_len + 1;

    msg = g_malloc(msg_len);
    if (msg) {
        head = (TrainProtocolHeader*)msg;
        head->start = 0xff;
        head->type = ipcam_itrain_message_get_message_type(message);
        head->len = htons(payload_len);
        if (payload)
            memcpy(head->data, payload, payload_len);
        msg[msg_len - 1] = train_protocol_checksum(msg, msg_len - 1);
        if (g_socket_send(sock, (gchar *)msg, msg_len, NULL, NULL) > 0)
            return TRUE;
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
