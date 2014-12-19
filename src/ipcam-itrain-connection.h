/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * ipcam-connection.h
 * Copyright (C) 2014 Watson Xu <xuhuashan@gmail.com>
 *
 */

#ifndef _IPCAM_CONNECTION_H_
#define _IPCAM_CONNECTION_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define IPCAM_TYPE_CONNECTION             (ipcam_connection_get_type ())
#define IPCAM_CONNECTION(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPCAM_TYPE_CONNECTION, IpcamConnection))
#define IPCAM_CONNECTION_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), IPCAM_TYPE_CONNECTION, IpcamConnectionClass))
#define IPCAM_IS_CONNECTION(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPCAM_TYPE_CONNECTION))
#define IPCAM_IS_CONNECTION_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPCAM_TYPE_CONNECTION))
#define IPCAM_CONNECTION_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), IPCAM_TYPE_CONNECTION, IpcamConnectionClass))

typedef struct _IpcamConnectionClass IpcamConnectionClass;
typedef struct _IpcamConnection IpcamConnection;
typedef struct _IpcamConnectionPrivate IpcamConnectionPrivate;


struct _IpcamConnectionClass
{
    GObjectClass parent_class;
};

struct _IpcamConnection
{
    GObject parent_instance;

    IpcamConnectionPrivate *priv;
};

GType ipcam_connection_get_type (void) G_GNUC_CONST;

struct _TrainProtocolHeader
{
    guint8 start;   /* should always be 0xff */
    guint8 type;
    guint16 len;
    guint8 data[0];
} __attribute__((packed));
typedef struct _TrainProtocolHeader TrainProtocolHeader;

typedef gboolean IpcamProtoHandler(IpcamConnection *, IpcamITrainMessage *);

gpointer ipcam_connection_get_userdata(IpcamConnection *connection);
void ipcam_connection_register_message_handler(IpcamConnection *connection,
                                                     guint type,
                                                     IpcamProtoHandler *handler);
void ipcam_connection_unregister_message_handler(IpcamConnection *connection,
                                                       guint type,
                                                       IpcamProtoHandler *handler);
IpcamITrainMessage *ipcam_connection_get_message (IpcamConnection *connection);
gboolean ipcam_connection_send_message(IpcamConnection *connection, IpcamITrainMessage *message);
gboolean ipcam_connection_dispatch_message (IpcamConnection *connection, IpcamITrainMessage *message);

G_END_DECLS

#endif /* _IPCAM_CONNECTION_H_ */

