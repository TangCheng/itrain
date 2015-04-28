/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * ipcam-proto-types.h
 * Copyright (C) 2014 Watson Xu <xuhuashan@gmail.com>
 *
 */

#ifndef _IPCAM_PROTO_TYPES_H_
#define _IPCAM_PROTO_TYPES_H_

#include <glib.h>
#include <gio/gio.h>
#include <time.h>

#include "ipcam-itrain.h"
#include "ipcam-itrain-message.h"

struct IpcamConnection;
typedef struct IpcamConnection IpcamConnection;

#define NR_TIMEOUTS             4

typedef struct IpcamTimeout
{
    gboolean enabled;
    guint32  timeout_sec;
    time_t   expire;
} IpcamTimeout;

struct IpcamConnection
{
    int          sock;
    IpcamITrain  *itrain;
    IpcamTimeout timeouts[NR_TIMEOUTS];
    gpointer     priv;
};

void    ipcam_connection_enable_timeout(IpcamConnection *conn, guint32 id, gboolean enabled);
void    ipcam_connection_set_timeout(IpcamConnection *conn, guint32 id, gint32 timeout_sec);
void    ipcam_connection_reset_timeout(IpcamConnection *conn, guint32 id);
gssize  ipcam_connection_send_pdu(IpcamConnection *conn, IpcamTrainPDU *pdu);
void    ipcam_connection_free(IpcamConnection *conn);

typedef struct IpcamTrainProtocolType
{
    guint32  user_data_size;
    gboolean (*init_connection)  (IpcamConnection *conn);
    int      (*on_data_arrive)   (IpcamConnection *conn);
    void     (*on_timeout)       (IpcamConnection *conn,
                                  guint32 id);
    void     (*on_report_status) (IpcamConnection *conn,
                                  gboolean occlusion_stat,
                                  gboolean loss_stat);
    void     (*deinit_connection)(IpcamConnection *conn);
} IpcamTrainProtocolType;

#endif /* _IPCAM_PROTO_TYPES_H_ */

