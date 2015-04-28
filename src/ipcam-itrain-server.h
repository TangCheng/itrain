/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * ipcam-itrain-server.h
 * Copyright (C) 2014 Watson Xu <xuhuashan@gmail.com>
 *
 */

#ifndef _IPCAM_ITRAIN_SERVER_H_
#define _IPCAM_ITRAIN_SERVER_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define IPCAM_TYPE_ITRAIN_SERVER             (ipcam_itrain_server_get_type ())
#define IPCAM_ITRAIN_SERVER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPCAM_TYPE_ITRAIN_SERVER, IpcamITrainServer))
#define IPCAM_ITRAIN_SERVER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), IPCAM_TYPE_ITRAIN_SERVER, IpcamITrainServerClass))
#define IPCAM_IS_ITRAIN_SERVER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPCAM_TYPE_ITRAIN_SERVER))
#define IPCAM_IS_ITRAIN_SERVER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPCAM_TYPE_ITRAIN_SERVER))
#define IPCAM_ITRAIN_SERVER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), IPCAM_TYPE_ITRAIN_SERVER, IpcamITrainServerClass))

typedef struct _IpcamITrainServerClass IpcamITrainServerClass;
typedef struct _IpcamITrainServer IpcamITrainServer;
typedef struct _IpcamITrainServerPrivate IpcamITrainServerPrivate;


struct _IpcamITrainServerClass
{
    GObjectClass parent_class;
};

struct _IpcamITrainServer
{
    GObject parent_instance;

    IpcamITrainServerPrivate *priv;
};

GType ipcam_itrain_server_get_type (void) G_GNUC_CONST;
int ipcam_itrain_server_send_notify(IpcamITrainServer *itrain_server,
                                    gpointer notify,
                                    guint length);
void ipcam_itrain_server_report_status(IpcamITrainServer *ipcam_itrain_server,
                                       gboolean occlusion_stat, 
                                       gboolean loss_stat);

G_END_DECLS

#endif /* _IPCAM_ITRAIN_SERVER_H_ */

