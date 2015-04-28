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
#include "ipcam-dttx-proto-handler.h"

#define DEFAULT_BUFFER_SIZE 1024

typedef struct IpcamDttxConnectionPriv
{
    guint8   *buffer;
    guint32  buffer_size;
    guint32  data_size;
} IpcamDttxConnectionPriv;

#define TIMEOUT_SEND_HEARTBEAT  0
#define TIMEOUT_RECV_HEARTBEAT  1

#define MSGTYPE_HEARTBEAT_REQUEST       0x01
#define MSGTYPE_HEARTBEAT_RESPONSE      0x51
#define MSGTYPE_QUERYSTATUS_REQUEST     0x07
#define MSGTYPE_QUERYSTATUS_RESPONSE    0x57
#define MSGTYPE_VIDEO_FAULT_EVENT       0x08
#define MSGTYPE_SETNETWORK_REQUEST      0x12

/* Payload definitions */

typedef struct HeartBeatRequest
{
     guint8 dummy[0];
} __attribute__((packed)) HeadBeatRequest;

typedef struct QueryStatusRequest
{
    guint8 dummy[0];
} __attribute__((packed)) QueryStatusRequest;

typedef struct QueryStatusResponse
{
    guint32 train_num;
    guint8  position_num;
    guint8  online_state;
    guint8  camera_type;
    guint8  manufacturer[8];
    guint16 version;
} __attribute__((packed)) QueryStatusResponse;

typedef struct VideoFaultEvent
{
    guint32 train_num;
    guint8  position_num;
    guint8  occlusion_stat;
    guint8  loss_stat;
} __attribute__((packed)) VideoFaultEvent;

typedef struct SetNetworkRequest
{
    guint8  network_num;
} SetNetworkRequest;


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
ipcam_proto_do_set_network(IpcamITrain *itrain, SetNetworkRequest *payload)
{
    gboolean ret;
    char buf[32];
    const char *position_num;
    JsonBuilder *builder = json_builder_new();

    position_num = ipcam_itrain_get_string_property(itrain, "szyc:position_num");

    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "items");
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "address");
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "ipaddr");
    g_snprintf(buf, sizeof(buf), "192.168.%d.%d",
               payload->network_num,
               (int)(strtoul(position_num, NULL, 0) + 70));
    json_builder_add_string_value(builder, buf);
    json_builder_set_member_name(builder, "netmask");
    json_builder_add_string_value(builder, "255.255.0.0");
    json_builder_end_object(builder);
    json_builder_end_object(builder);
    json_builder_end_object(builder);

    ret = itrain_invocate_action(itrain, "set_network", json_builder_get_root(builder), NULL);

    g_object_unref(builder);

    return ret;
}

static gboolean
ipcam_proto_do_query_status(IpcamITrain *itrain, QueryStatusResponse *payload)
{
    const char *train_num, *position_num;
    const char *device_type;
    const char *fw_ver;
    guint maj, min, rev;

    train_num = ipcam_itrain_get_string_property(itrain, "szyc:train_num");
    position_num = ipcam_itrain_get_string_property(itrain, "szyc:position_num");
    device_type = ipcam_itrain_get_string_property(itrain, "base_info:device_type");
    fw_ver = ipcam_itrain_get_string_property(itrain, "base_info:firmware");
    if (fw_ver && train_num && position_num) {
        payload->train_num = htonl(strtoul(train_num, NULL, 0));
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
        payload->train_num = 0;
        payload->position_num = 0;
        payload->version = 0;
        payload->camera_type = 0;
        payload->online_state = 0x00;
    }
    strncpy((char *)payload->manufacturer, "EASYWAY", sizeof(payload->manufacturer));

    return TRUE;
}

gboolean
ipcam_dttx_heartbeat(IpcamConnection *conn, IpcamTrainPDU *request_pdu)
{
    ipcam_connection_reset_timeout(conn, TIMEOUT_RECV_HEARTBEAT);

    return TRUE;
}

gboolean
ipcam_dttx_query_status(IpcamConnection *conn, IpcamTrainPDU *request_pdu)
{
    QueryStatusResponse status;
    IpcamTrainPDU *response_pdu;
    IpcamITrain *itrain = conn->itrain;
    gboolean result = FALSE;

    g_assert(IPCAM_IS_ITRAIN(itrain));

    ipcam_connection_reset_timeout(conn, TIMEOUT_RECV_HEARTBEAT);

    if (ipcam_proto_do_query_status (itrain, &status)) {
        response_pdu = ipcam_train_pdu_new(MSGTYPE_QUERYSTATUS_RESPONSE, sizeof(status));
        if (response_pdu) {
            ipcam_train_pdu_set_payload(response_pdu, &status);
            result = ipcam_connection_send_pdu(conn, response_pdu);
            ipcam_train_pdu_free(response_pdu);
        }
    }

    return result;
}

gboolean
ipcam_dttx_set_network(IpcamConnection *conn, IpcamTrainPDU *request_pdu)
{
    IpcamITrain *itrain = conn->itrain;
    SetNetworkRequest *payload;
    guint16 payload_size;

    ipcam_connection_reset_timeout(conn, TIMEOUT_RECV_HEARTBEAT);

    g_assert(IPCAM_IS_ITRAIN(itrain));

    payload = ipcam_train_pdu_get_payload(request_pdu);
    payload_size = ipcam_train_pdu_get_payload_size(request_pdu);
    if (payload && payload_size >= sizeof(*payload)) {
        ipcam_proto_do_set_network(itrain, payload);
        return TRUE;
    }
    else {
        g_warning("%s: payload size is too small.\n", __func__);
    }

    return TRUE;
}

static gboolean ipcam_dttx_dispatch_pdu(IpcamConnection *conn, IpcamTrainPDU *pdu)
{
    gboolean ret = FALSE;
    guint8 pdu_type = ipcam_train_pdu_get_type(pdu);

    switch(pdu_type) {
    case MSGTYPE_HEARTBEAT_RESPONSE:
        ret = ipcam_dttx_heartbeat(conn, pdu);
        break;
    case MSGTYPE_QUERYSTATUS_REQUEST:
        ret = ipcam_dttx_query_status(conn, pdu);
        break;
    case MSGTYPE_SETNETWORK_REQUEST:
        ret = ipcam_dttx_set_network(conn, pdu);
        break;
    }

    return ret;
}

static gboolean ipcam_dttx_init_connection(IpcamConnection *conn)
{
    IpcamDttxConnectionPriv *priv = conn->priv;

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

static int ipcam_dttx_data_arrive(IpcamConnection *conn)
{
    IpcamDttxConnectionPriv *priv = conn->priv;
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
            ipcam_dttx_dispatch_pdu(conn, pdu);
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

static void ipcam_dttx_timeout_send_heartbeat(IpcamConnection *conn)
{
    IpcamTrainPDU *pdu;

    pdu = ipcam_train_pdu_new(MSGTYPE_HEARTBEAT_REQUEST, 0);
    ipcam_train_pdu_set_payload(pdu, NULL); /* calculate checksum only */
    ipcam_connection_send_pdu (conn, pdu);
    ipcam_train_pdu_free(pdu);
}

static void ipcam_dttx_timeout_recv_heartbeat(IpcamConnection *conn)
{
    g_print("connection session timeout\n");
    ipcam_connection_free(conn);
}

static void ipcam_dttx_timeout(IpcamConnection *conn, guint32 timeout_id)
{
    switch(timeout_id) {
    case TIMEOUT_SEND_HEARTBEAT:
        ipcam_dttx_timeout_send_heartbeat(conn);
        break;
    case TIMEOUT_RECV_HEARTBEAT:
        ipcam_dttx_timeout_recv_heartbeat(conn);
        break;
    }
}

static void ipcam_dttx_report_status(IpcamConnection *conn,
                                     gboolean occlusion_stat,
                                     gboolean loss_stat)
{
    IpcamTrainPDU *pdu;
    VideoFaultEvent payload;
    const char *train_num, *position_num;

    train_num = ipcam_itrain_get_string_property(conn->itrain, "szyc:train_num");
    position_num = ipcam_itrain_get_string_property(conn->itrain, "szyc:position_num");

    payload.train_num = htonl(strtoul(train_num, NULL, 0));
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

static void ipcam_dttx_deinit_connection(IpcamConnection *conn)
{
}

IpcamTrainProtocolType ipcam_dttx_protocol_type = {
    .user_data_size    = sizeof(IpcamDttxConnectionPriv),
    .init_connection   = ipcam_dttx_init_connection,
    .on_data_arrive    = ipcam_dttx_data_arrive,
    .on_timeout        = ipcam_dttx_timeout,
    .on_report_status  = ipcam_dttx_report_status,
    .deinit_connection = ipcam_dttx_deinit_connection
};
