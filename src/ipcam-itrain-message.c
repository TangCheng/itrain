/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * ipcam-proto-message.c
 * Copyright (C) 2014 Watson Xu <xuhuashan@gmail.com>
 *
 */

#include "ipcam-itrain-message.h"

struct _IpcamITrainMessagePrivate
{
     gpointer user_data;
     guint8 type;
     guint16 length;
     gpointer payload;
};


enum
{
    PROP_0,

    PROP_USERDATA,
    PROP_TYPE,
    PROP_LENGTH,
    PROP_PAYLOAD
};



G_DEFINE_TYPE (IpcamITrainMessage, ipcam_itrain_message, G_TYPE_OBJECT);

static void
ipcam_itrain_message_init (IpcamITrainMessage *ipcam_itrain_message)
{
    ipcam_itrain_message->priv = G_TYPE_INSTANCE_GET_PRIVATE (ipcam_itrain_message, IPCAM_TYPE_ITRAIN_MESSAGE, IpcamITrainMessagePrivate);

    ipcam_itrain_message->priv->user_data = NULL;
    ipcam_itrain_message->priv->type = 0;
    ipcam_itrain_message->priv->length = 0;
    ipcam_itrain_message->priv->payload = NULL;
}

static void
ipcam_itrain_message_finalize (GObject *object)
{
    IpcamITrainMessage *message = IPCAM_ITRAIN_MESSAGE(object);

    if (message->priv->payload)
        g_free(message->priv->payload);

    G_OBJECT_CLASS (ipcam_itrain_message_parent_class)->finalize (object);
}

static void
ipcam_itrain_message_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
    IpcamITrainMessage *message;

    g_return_if_fail (IPCAM_IS_ITRAIN_MESSAGE (object));

    message = IPCAM_ITRAIN_MESSAGE(object);

    switch (prop_id)
    {
    case PROP_USERDATA:
        message->priv->user_data = g_value_get_pointer(value);
        break;
    case PROP_TYPE:
        message->priv->type = g_value_get_uint(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
ipcam_itrain_message_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
    IpcamITrainMessage *message = IPCAM_ITRAIN_MESSAGE(object);

    g_return_if_fail (IPCAM_IS_ITRAIN_MESSAGE (object));

    switch (prop_id)
    {
    case PROP_USERDATA:
        g_value_set_pointer(value, message->priv->user_data);
        break;
    case PROP_TYPE:
        g_value_set_uint(value, message->priv->type);
        break;
    case PROP_LENGTH:
        g_value_set_uint(value, message->priv->type);
        break;
    case PROP_PAYLOAD:
        g_value_set_pointer(value, message->priv->payload);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
ipcam_itrain_message_class_init (IpcamITrainMessageClass *klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS (klass);

    g_type_class_add_private (klass, sizeof (IpcamITrainMessagePrivate));

    object_class->finalize = ipcam_itrain_message_finalize;
    object_class->set_property = ipcam_itrain_message_set_property;
    object_class->get_property = ipcam_itrain_message_get_property;

    g_object_class_install_property (object_class,
                                     PROP_USERDATA,
                                     g_param_spec_pointer ("user_data",
                                                           "User Data",
                                                           "User Data",
                                                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

    g_object_class_install_property (object_class,
                                     PROP_TYPE,
                                     g_param_spec_uint ("type",
                                                        "Message Type",
                                                        "Message Type",
                                                        0,
                                                        G_MAXUINT8,
                                                        0,
                                                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

    g_object_class_install_property (object_class,
                                     PROP_LENGTH,
                                     g_param_spec_uint ("length",
                                                        "Message Length",
                                                        "Message Length",
                                                        0,
                                                        G_MAXUINT16,
                                                        0,
                                                        G_PARAM_READABLE));

    g_object_class_install_property (object_class,
                                     PROP_USERDATA,
                                     g_param_spec_pointer ("data",
                                                           "User Data",
                                                           "User Data",
                                                           G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
}


gpointer
ipcam_itrain_message_get_userdata(IpcamITrainMessage *message)
{
    IpcamITrainMessagePrivate *priv = message->priv;

    return priv->user_data;
}

void 
ipcam_itrain_message_set_userdata(IpcamITrainMessage *message, gpointer user_data)
{
    IpcamITrainMessagePrivate *priv = message->priv;

    priv->user_data = user_data;
}

guint 
ipcam_itrain_message_get_message_type(IpcamITrainMessage *message)
{
    IpcamITrainMessagePrivate *priv = message->priv;

    return priv->type;
}

void ipcam_itrain_message_set_message_type(IpcamITrainMessage *message, guint type)
{
    IpcamITrainMessagePrivate *priv = message->priv;

    priv->type = type;
}

guint ipcam_itrain_message_get_payload_length(IpcamITrainMessage *message)
{
    IpcamITrainMessagePrivate *priv = message->priv;

    return priv->length;
}

gpointer ipcam_itrain_message_get_payload(IpcamITrainMessage *message, guint *length)
{
    IpcamITrainMessagePrivate *priv = message->priv;

    *length = priv->length;
    return priv->payload;
}

void ipcam_itrain_message_set_payload(IpcamITrainMessage *message, gpointer payload, guint length)
{
    IpcamITrainMessagePrivate *priv = message->priv;

    if (priv->payload)
        g_free(priv->payload);

    priv->payload = payload;
    priv->length = length;
}
