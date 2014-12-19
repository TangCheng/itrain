/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * ipcam-proto-message.h
 * Copyright (C) 2014 Watson Xu <xuhuashan@gmail.com>
 *
 */

#ifndef _IPCAM_ITRAIN_MESSAGE_H_
#define _IPCAM_ITRAIN_MESSAGE_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define IPCAM_TYPE_ITRAIN_MESSAGE             (ipcam_itrain_message_get_type ())
#define IPCAM_ITRAIN_MESSAGE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPCAM_TYPE_ITRAIN_MESSAGE, IpcamITrainMessage))
#define IPCAM_ITRAIN_MESSAGE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), IPCAM_TYPE_ITRAIN_MESSAGE, IpcamITrainMessageClass))
#define IPCAM_IS_ITRAIN_MESSAGE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPCAM_TYPE_ITRAIN_MESSAGE))
#define IPCAM_IS_ITRAIN_MESSAGE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPCAM_TYPE_ITRAIN_MESSAGE))
#define IPCAM_ITRAIN_MESSAGE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), IPCAM_TYPE_ITRAIN_MESSAGE, IpcamITrainMessageClass))

typedef struct _IpcamITrainMessageClass IpcamITrainMessageClass;
typedef struct _IpcamITrainMessage IpcamITrainMessage;
typedef struct _IpcamITrainMessagePrivate IpcamITrainMessagePrivate;


struct _IpcamITrainMessageClass
{
    GObjectClass parent_class;
};

struct _IpcamITrainMessage
{
    GObject parent_instance;

    IpcamITrainMessagePrivate *priv;
};

GType ipcam_itrain_message_get_type (void) G_GNUC_CONST;

gpointer ipcam_itrain_message_get_userdata(IpcamITrainMessage *message);
void ipcam_itrain_message_set_userdata(IpcamITrainMessage *message, gpointer user_data);
guint ipcam_itrain_message_get_message_type(IpcamITrainMessage *message);
void ipcam_itrain_message_set_message_type(IpcamITrainMessage *message, guint type);
guint ipcam_itrain_message_get_payload_length(IpcamITrainMessage *message);
gpointer ipcam_itrain_message_get_payload(IpcamITrainMessage *message, guint *length);
void ipcam_itrain_message_set_payload(IpcamITrainMessage *message, gpointer payload, guint length);

G_END_DECLS

#endif /* _IPCAM_ITRAIN_MESSAGE_H_ */

