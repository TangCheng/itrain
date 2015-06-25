/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * ipcam-proto-handler.c
 * Copyright (C) 2014 Watson Xu <xuhuashan@gmail.com>
 *
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <json-glib/json-glib.h>
#include <request_message.h>
#include "ipcam-itrain.h"
#include "ipcam-proto-interface.h"
#include "ipcam-dctx-proto-handler.h"

#define DEFAULT_BUFFER_SIZE 1024

typedef struct IpcamDctxConnectionPriv
{
    guint8   *buffer;
    guint32  buffer_size;
    guint32  data_size;
} IpcamDctxConnectionPriv;

#define TIMEOUT_SEND_HEARTBEAT  0
#define TIMEOUT_RECV_HEARTBEAT  1


#define MSGTYPE_HEARTBEAT_REQUEST       0x01
#define MSGTYPE_HEARTBEAT_RESPONSE      0x51
#define MSGTYPE_SETIMAGEATTR_REQUEST    0x02
#define MSGTYPE_GETIMAGEATTR_REQUEST    0x03
#define MSGTYPE_GETIMAGEATTR_RESPONSE   0x53
#define MSGTYPE_SETOSD_REQUEST          0x05
#define MSGTYPE_TIMESYNC_REQUEST        0x06
#define MSGTYPE_QUERYSTATUS_REQUEST     0x08
#define MSGTYPE_QUERYSTATUS_RESPONSE    0x58
#define MSGTYPE_VIDEO_FAULT_EVENT       0x09

/* Payload definitions */

typedef struct HeartBeatRequest
{
     guint8 dummy[0];
} __attribute__((packed)) HeadBeatRequest;

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

typedef struct VideoFaultEvent
{
    guint8 carriage_num;
    guint8 position_num;
    guint8 occlusion_stat;
    guint8 loss_stat;
} __attribute__((packed)) VideoFaultEvent;


static gboolean
itrain_invocate_action(IpcamITrain *itrain, const char *action,
                       JsonNode *request, JsonNode **response)
{
	const gchar *token;
	IpcamRequestMessage *req_msg;
	IpcamMessage *resp_msg;
	gboolean ret = FALSE;

	token = ipcam_base_app_get_config(IPCAM_BASE_APP(itrain), "token");

	req_msg = g_object_new(IPCAM_REQUEST_MESSAGE_TYPE,
	                       "action", action,
	                       "body", request,
	                       NULL);

	ipcam_base_app_send_message(IPCAM_BASE_APP(itrain),
	                            IPCAM_MESSAGE(req_msg),
	                            "iconfig", token, NULL, 10);

	ret = ipcam_base_app_wait_response(IPCAM_BASE_APP(itrain),
	                                   ipcam_request_message_get_id(req_msg),
	                                   5000, &resp_msg);
	if (ret)
	{
		JsonNode *resp_body;

        if (response) {
            g_object_get(G_OBJECT(resp_msg), "body", &resp_body, NULL);
            *response = json_node_copy(resp_body);
        }

		g_object_unref(resp_msg);
	}

	g_object_unref(req_msg);

	return ret;
}

static gboolean
ipcam_dctx_do_set_image_attr(IpcamITrain *itrain, SetImageAttrRequest *payload)
{
    gboolean ret;
    JsonBuilder *builder = json_builder_new();

    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "items");
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "brightness");
    json_builder_add_int_value(builder, payload->brightness);
    json_builder_set_member_name(builder, "chrominance");
    json_builder_add_int_value(builder, payload->chrominance);
    json_builder_set_member_name(builder, "saturation");
    json_builder_add_int_value(builder, payload->saturation);
    json_builder_set_member_name(builder, "contrast");
    json_builder_add_int_value(builder, payload->contrast);
    json_builder_end_object(builder);
    json_builder_end_object(builder);

    ret = itrain_invocate_action(itrain, "set_image", json_builder_get_root(builder), NULL);

    g_object_unref(builder);

    return ret;
}

static gboolean
ipcam_dctx_do_get_image_attr(IpcamITrain *itrain, GetImageAttrResponse *payload)
{
    gboolean ret;
    JsonNode *response;
    JsonBuilder *builder = json_builder_new();

    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "items");
    json_builder_begin_array(builder);
    json_builder_add_string_value(builder, "brightness");
    json_builder_add_string_value(builder, "chrominance");
    json_builder_add_string_value(builder, "saturation");
    json_builder_add_string_value(builder, "contrast");
    json_builder_end_array(builder);
    json_builder_end_object(builder);

    ret = itrain_invocate_action(itrain, "get_image", json_builder_get_root(builder), &response);
    if (ret) {
        JsonObject *items = json_object_get_object_member(json_node_get_object(response), "items");
        payload->brightness = json_object_get_int_member(items, "brightness");
        payload->chrominance = json_object_get_int_member(items, "chrominance");
        payload->saturation = json_object_get_int_member(items, "saturation");
        payload->contrast = json_object_get_int_member(items, "contrast");

        json_node_free(response);
    }

    g_object_unref(builder);

    return ret;
}

static gboolean
ipcam_dctx_do_set_osd(IpcamITrain *itrain, SetOsdRequest *payload)
{
    gboolean ret;
    char buf[16];
    JsonBuilder *builder = json_builder_new();

    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "items");
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "train_num");
    json_builder_add_string_value(builder, (gchar *)payload->train_num);
    json_builder_set_member_name(builder, "carriage_num");
    g_snprintf(buf, sizeof(buf), "%d", payload->carriage_num);
    json_builder_add_string_value(builder, buf);
    json_builder_set_member_name(builder, "position_num");
    g_snprintf(buf, sizeof(buf), "%d", payload->position_num);
    json_builder_add_string_value(builder, buf);
    json_builder_end_object(builder);
    json_builder_end_object(builder);

    ret = itrain_invocate_action(itrain, "set_szyc", json_builder_get_root(builder), NULL);

    g_object_unref(builder);

    return ret;
}

static gboolean
ipcam_dctx_do_timesync(IpcamITrain *itrain, TimeSyncRequest *payload)
{
    gchar *str;
    gboolean ret;
    JsonBuilder *builder = json_builder_new();

    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "items");
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "datetime");
    str = g_strdup_printf("%04d-%02d-%02d %02d:%02d:%02d",
                          ntohs(payload->year),
                          payload->mon,
                          payload->day,
                          payload->hour,
                          payload->min,
                          payload->sec);
    json_builder_add_string_value (builder, str);
    g_free(str);

    json_builder_end_object(builder);
    json_builder_end_object(builder);

    ret = itrain_invocate_action(itrain, "set_datetime", json_builder_get_root(builder), NULL);

    g_object_unref(builder);

    return ret;
}

static gboolean
ipcam_dctx_do_query_status(IpcamITrain *itrain, QueryStatusResponse *payload)
{
    const char *carriage_num, *position_num;
    const char *device_type;
    const char *fw_ver;
    guint maj, min, rev;

    carriage_num = ipcam_itrain_get_string_property(itrain, "szyc:carriage_num");
    position_num = ipcam_itrain_get_string_property(itrain, "szyc:position_num");
    device_type = ipcam_itrain_get_string_property(itrain, "base_info:device_type");
    fw_ver = ipcam_itrain_get_string_property(itrain, "base_info:firmware");
    if (fw_ver && carriage_num && position_num) {
        payload->carriage_num = strtoul(carriage_num, NULL, 0);
        payload->position_num = strtoul(position_num, NULL, 0);
        if (sscanf(fw_ver, "%d.%d.%d", &maj, &min, &rev) == 3) {
            maj = maj <= 9 ? maj : (maj / 10);
            min = min <= 9 ? min : (min / 10);
            rev = rev <= 9 ? rev : (rev / 10);
            payload->version = htons(maj * 100 + min * 10 + rev);
        }
        payload->online_state = 0x01;
        if (device_type)
            payload->camera_type = strtoul(device_type, NULL, 0);
        else
            payload->camera_type = 0;
    }
    else {
        payload->carriage_num = 0;
        payload->position_num = 0;
        payload->version = 0;
        payload->camera_type = 0;
        payload->online_state = 0x00;
    }
    strncpy((char *)payload->manufacturer, "EASYWAY", sizeof(payload->manufacturer));

    return TRUE;
}

gboolean
ipcam_dctx_set_image_attr(IpcamConnection *conn, IpcamTrainPDU *rq_pdu)
{
    SetImageAttrRequest *imgattr;
    guint rq_size;
    IpcamITrain *itrain = conn->itrain;

    g_assert(IPCAM_IS_ITRAIN(itrain));

    ipcam_connection_reset_timeout(conn, TIMEOUT_RECV_HEARTBEAT);

    imgattr = ipcam_train_pdu_get_payload(rq_pdu);
    rq_size = ipcam_train_pdu_get_payload_size(rq_pdu);
    if (imgattr && rq_size >= sizeof(*imgattr)) {
        ipcam_dctx_do_set_image_attr(itrain, imgattr);
        return TRUE;
    }
    else {
        g_warning("%s: payload size is too small.\n", __func__);
    }

    return FALSE;
}

gboolean
ipcam_dctx_get_image_attr(IpcamConnection *conn, IpcamTrainPDU *rq_pdu)
{
    GetImageAttrResponse imgattr;
    IpcamTrainPDU *resp_pdu;
    IpcamITrain *itrain = conn->itrain;
    gboolean result = FALSE;

    g_assert(IPCAM_IS_ITRAIN(itrain));

    ipcam_connection_reset_timeout(conn, TIMEOUT_RECV_HEARTBEAT);

    if (ipcam_dctx_do_get_image_attr (itrain, &imgattr)) {
        resp_pdu = ipcam_train_pdu_new(MSGTYPE_GETIMAGEATTR_RESPONSE, sizeof(imgattr));
        ipcam_train_pdu_set_payload(resp_pdu, &imgattr);
        result = ipcam_connection_send_pdu(conn, resp_pdu);
        ipcam_train_pdu_free(resp_pdu);
    }

    return result;
}

gboolean
ipcam_dctx_set_osd(IpcamConnection *conn, IpcamTrainPDU *rq_pdu)
{
    SetOsdRequest *osd;
    guint rq_size;
    IpcamITrain *itrain = conn->itrain;

    g_assert(IPCAM_IS_ITRAIN(itrain));

    ipcam_connection_reset_timeout(conn, TIMEOUT_RECV_HEARTBEAT);

    osd = ipcam_train_pdu_get_payload(rq_pdu);
    rq_size = ipcam_train_pdu_get_payload_size(rq_pdu);
    if (osd && rq_size >= sizeof(*osd)) {
        ipcam_dctx_do_set_osd(itrain, osd);
        return TRUE;
    }
    else {
        g_warning("%s: payload size is too small.\n", __func__);
    }

    return FALSE;
}

gboolean
ipcam_dctx_timesync(IpcamConnection *conn, IpcamTrainPDU *rq_pdu)
{
    TimeSyncRequest *timesync;
    guint rq_size;
    IpcamITrain *itrain = conn->itrain;

    g_assert(IPCAM_IS_ITRAIN(itrain));

    ipcam_connection_reset_timeout(conn, TIMEOUT_RECV_HEARTBEAT);

    timesync = ipcam_train_pdu_get_payload(rq_pdu);
    rq_size = ipcam_train_pdu_get_payload_size(rq_pdu);
    if (timesync && rq_size >= sizeof(*timesync)) {
        ipcam_dctx_do_timesync (itrain, timesync);
        return TRUE;
    }
    else {
        g_warning("%s: payload size is too small.\n", __func__);
    }

    return FALSE;
}

gboolean
ipcam_dctx_heartbeat(IpcamConnection *conn, IpcamTrainPDU *rq_pdu)
{
    ipcam_connection_reset_timeout(conn, TIMEOUT_RECV_HEARTBEAT);

    return TRUE;
}

gboolean
ipcam_dctx_query_status(IpcamConnection *conn, IpcamTrainPDU *rq_pdu)
{
    QueryStatusResponse status;
    IpcamTrainPDU *resp_pdu;
    IpcamITrain *itrain = conn->itrain;
    gboolean result = FALSE;

    ipcam_connection_reset_timeout(conn, TIMEOUT_RECV_HEARTBEAT);

    g_assert(IPCAM_IS_ITRAIN(itrain));

    if (ipcam_dctx_do_query_status (itrain, &status)) {
        resp_pdu = ipcam_train_pdu_new(MSGTYPE_QUERYSTATUS_RESPONSE, sizeof(status));
        if (resp_pdu) {
            ipcam_train_pdu_set_payload(resp_pdu, &status);
            result = ipcam_connection_send_pdu(conn, resp_pdu);
            ipcam_train_pdu_free(resp_pdu);
        }
    }

    return result;
}

gboolean
ipcam_dctx_time_sync(IpcamConnection *conn, IpcamTrainPDU *rq_pdu)
{
    IpcamITrain *itrain = conn->itrain;
    TimeSyncRequest *payload;
    guint16 payload_size;

    ipcam_connection_reset_timeout(conn, TIMEOUT_RECV_HEARTBEAT);

    g_assert(IPCAM_IS_ITRAIN(itrain));

    payload = ipcam_train_pdu_get_payload(rq_pdu);
    payload_size = ipcam_train_pdu_get_payload_size(rq_pdu);
    if (payload && payload_size >= sizeof(*payload)) {
        ipcam_dctx_do_timesync(itrain, payload);
        return TRUE;
    }
    else {
        g_warning("%s: payload size is too small.\n", __func__);
    }

    return FALSE;
}

static gboolean ipcam_dctx_dispatch_pdu(IpcamConnection *conn, IpcamTrainPDU *pdu)
{
    gboolean ret = FALSE;
    guint8 pdu_type = ipcam_train_pdu_get_type(pdu);

    switch(pdu_type) {
    case MSGTYPE_HEARTBEAT_RESPONSE:
        ret = ipcam_dctx_heartbeat(conn, pdu);
        break;
    case MSGTYPE_QUERYSTATUS_REQUEST:
        ret = ipcam_dctx_query_status(conn, pdu);
        break;
    case MSGTYPE_TIMESYNC_REQUEST:
        ret = ipcam_dctx_time_sync(conn, pdu);
        break;
    case MSGTYPE_SETOSD_REQUEST:
        ret = ipcam_dctx_set_osd(conn, pdu);
        break;
    case MSGTYPE_GETIMAGEATTR_REQUEST:
        ret = ipcam_dctx_get_image_attr(conn, pdu);
        break;
    case MSGTYPE_SETIMAGEATTR_REQUEST:
        ret = ipcam_dctx_set_image_attr(conn, pdu);
        break;
    default:
        g_print("unhandled request %d\n", pdu_type);
        break;
    }

    return ret;
}

static gboolean ipcam_dctx_init_connection(IpcamConnection *conn)
{
    IpcamDctxConnectionPriv *priv = conn->priv;

    priv->buffer = g_malloc(DEFAULT_BUFFER_SIZE);
    if (!priv->buffer) {
        g_print("Out of memory for new connection buffer.\n");
        return FALSE;
    }
    priv->buffer_size = DEFAULT_BUFFER_SIZE;
    priv->data_size = 0;

    ipcam_connection_enable_timeout(conn, TIMEOUT_SEND_HEARTBEAT, TRUE);
    ipcam_connection_set_timeout(conn, TIMEOUT_SEND_HEARTBEAT, 5);
    ipcam_connection_enable_timeout(conn, TIMEOUT_RECV_HEARTBEAT, TRUE);
    ipcam_connection_set_timeout(conn, TIMEOUT_RECV_HEARTBEAT, 15);

    return TRUE;
}

static int ipcam_dctx_data_arrive(IpcamConnection *conn)
{
    IpcamDctxConnectionPriv *priv = conn->priv;
    gpointer buffer = &priv->buffer[priv->data_size];
    gssize space_left = priv->buffer_size - priv->data_size;
    int ret;

    g_return_val_if_fail(space_left > 0, 0);

    ret = recv(conn->sock, buffer, space_left, 0);
    if (ret > 0) {
        priv->data_size += ret;
    }

    /* find header */
    guint16 offset = 0;
    while((priv->buffer[offset] != PACKET_START) && offset < priv->data_size)
        offset++;

    /* header not found, discard current buffer */
    if (priv->buffer[offset] != PACKET_START) {
        priv->data_size = 0;
        return 0;
    }

    IpcamTrainPDU *pdu;

    pdu = ipcam_train_pdu_new_from_buffer(&priv->buffer[offset], priv->data_size - offset);
    if (pdu) {
        if (ipcam_train_pdu_verify_checksum(pdu)) {
            ipcam_dctx_dispatch_pdu(conn, pdu);
        }
        else {
            g_print("PDU checksum should be 0x%02X.\n",
                    ipcam_train_pdu_checksum(pdu));
        }

        ipcam_train_pdu_free(pdu);

        /* just discard the remain pdu */
        priv->data_size = 0;
    }

    return 0;
}

static void ipcam_dctx_timeout_send_heartbeat(IpcamConnection *conn)
{
    IpcamTrainPDU *pdu;

    pdu = ipcam_train_pdu_new(MSGTYPE_HEARTBEAT_REQUEST, 0);
    ipcam_train_pdu_set_payload(pdu, NULL); /* calculate checksum only */
    ipcam_connection_send_pdu (conn, pdu);
    ipcam_train_pdu_free(pdu);
}

static void ipcam_dctx_timeout_recv_heartbeat(IpcamConnection *conn)
{
    g_print("connection session timeout\n");
    ipcam_connection_free(conn);
}

static void ipcam_dctx_timeout(IpcamConnection *conn, guint32 timeout_id)
{
    switch(timeout_id) {
    case TIMEOUT_SEND_HEARTBEAT:
        ipcam_dctx_timeout_send_heartbeat(conn);
        break;
    case TIMEOUT_RECV_HEARTBEAT:
        ipcam_dctx_timeout_recv_heartbeat(conn);
        break;
    }
}

static void ipcam_dctx_report_status(IpcamConnection *conn,
                                     gboolean occlusion_stat,
                                     gboolean loss_stat)
{
    IpcamTrainPDU *pdu;
    VideoFaultEvent payload;
    const char *carriage_num, *position_num;

    carriage_num = ipcam_itrain_get_string_property(conn->itrain, "szyc:carriage_num");
    position_num = ipcam_itrain_get_string_property(conn->itrain, "szyc:position_num");

    payload.carriage_num = htonl(strtoul(carriage_num, NULL, 0));
    payload.position_num = strtoul(position_num, NULL, 0);
    payload.occlusion_stat = occlusion_stat;
    payload.loss_stat = loss_stat;

    pdu = ipcam_train_pdu_new(MSGTYPE_VIDEO_FAULT_EVENT, sizeof(payload));
    if (pdu == NULL) {
        g_critical("Out of memory\n");
        return;
    }

    ipcam_train_pdu_set_payload(pdu, &payload);
    ipcam_connection_send_pdu(conn, pdu);
    ipcam_train_pdu_free(pdu);
}

static void ipcam_dctx_deinit_connection(IpcamConnection *conn)
{
}

IpcamTrainProtocolType ipcam_dctx_protocol_type = {
    .user_data_size    = sizeof(IpcamDctxConnectionPriv),
    .init_connection   = ipcam_dctx_init_connection,
    .on_data_arrive    = ipcam_dctx_data_arrive,
    .on_timeout        = ipcam_dctx_timeout,
    .on_report_status  = ipcam_dctx_report_status,
    .deinit_connection = ipcam_dctx_deinit_connection
};

