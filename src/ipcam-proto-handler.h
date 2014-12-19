/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * ipcam-proto-handler.h
 * Copyright (C) 2014 Watson Xu <xuhuashan@gmail.com>
 *
 */

#ifndef _IPCAM_PROTO_HANDLER_H_
#define _IPCAM_PROTO_HANDLER_H_

#include <glib-object.h>

#define MSGTYPE_SETIMAGEATTR_REQUEST    0x02
#define MSGTYPE_GETIMAGEATTR_REQUEST    0x03
#define MSGTYPE_GETIMAGEATTR_RESPONSE   0x53
#define MSGTYPE_SETOSD_REQUEST          0x05
#define MSGTYPE_TIMESYNC_REQUEST        0x06
#define MSGTYPE_QUERYSTATUS_REQUEST     0x08
#define MSGTYPE_QUERYSTATUS_RESPONSE    0x58

/* Payload definitions */

typedef struct SetImageAttrRequest
{
     guint8 brightness;
     guint8 chrominance;
     guint8 saturation;
     guint8 contrast;
} __attribute__((packed)) SetImageAttrRequest;

typedef struct GetImageAttrRequest
{
    guint8 dummy[0];
} __attribute__((packed)) GetImageAttrRequest;

typedef struct GetImageAttrResponse
{
     guint8 brightness;
     guint8 chrominance;
     guint8 saturation;
     guint8 contrast;
} __attribute__((packed)) GetImageAttrResponse;

typedef struct SetOsdRequest
{
    struct {
        guint16 year;
        guint8  mon;
        guint8  day;
        guint8  hour;
        guint8  min;
        guint8  sec;
    } __attribute__((packed)) datetime;
    guint16     speed;
    guint8      train_num[7];
    guint8      carriage_num;
    guint8      position_num;
} __attribute__((packed)) SetOsdRequest;

typedef struct TimeSyncRequest
{
    guint16 year;
    guint8  mon;
    guint8  day;
    guint8  hour;
    guint8  min;
    guint8  sec;
} __attribute__((packed)) TimeSyncRequest;

typedef struct QueryStatusRequest
{
    guint8 dummy[0];
} __attribute__((packed)) QueryStatusRequest;

typedef struct QueryStatusResponse
{
    guint8  carriage_num;
    guint8  position_num;
    guint8  online_state;
    guint8  camera_type;
    guint8  manufacturer[10];
    guint16 version;
} __attribute__((packed)) QueryStatusResponse;

gboolean ipcam_proto_set_image_attr(IpcamConnection *connection, IpcamITrainMessage *message);
gboolean ipcam_proto_get_image_attr(IpcamConnection *connection, IpcamITrainMessage *message);
gboolean ipcam_proto_set_osd(IpcamConnection *connection, IpcamITrainMessage *message);
gboolean ipcam_proto_timesync(IpcamConnection *connection, IpcamITrainMessage *message);
gboolean ipcam_proto_query_status(IpcamConnection *connection, IpcamITrainMessage *message);
void ipcam_proto_register_all_handlers(IpcamConnection *connection);

#endif /* _IPCAM_PROTO_HANDLER_H_ */

