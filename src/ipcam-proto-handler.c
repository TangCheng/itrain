/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * ipcam-proto-handler.c
 * Copyright (C) 2014 Watson Xu <xuhuashan@gmail.com>
 *
 */

#include <string.h>
#include <stdio.h>
#include <netinet/in.h>
#include <json-glib/json-glib.h>
#include <request_message.h>
#include "itrain.h"
#include "ipcam-itrain-message.h"
#include "ipcam-itrain-connection.h"
#include "ipcam-proto-handler.h"

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
ipcam_proto_do_set_image_attr(IpcamITrain *itrain, SetImageAttrRequest *payload)
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
ipcam_proto_do_get_image_attr(IpcamITrain *itrain, GetImageAttrResponse *payload)
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
    }

    g_object_unref(builder);

    return ret;
}

static gboolean
ipcam_proto_do_set_osd(IpcamITrain *itrain, SetOsdRequest *payload)
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
ipcam_proto_do_timesync(IpcamITrain *itrain, TimeSyncRequest *payload)
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
ipcam_proto_do_query_status(IpcamITrain *itrain, QueryStatusResponse *payload)
{
    gboolean ret;
    JsonNode *response;
    JsonBuilder *builder = json_builder_new();

    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "items");
    json_builder_begin_array(builder);
    json_builder_add_string_value(builder, "carriage_num");
    json_builder_add_string_value(builder, "position_num");
    json_builder_end_array(builder);
    json_builder_end_object(builder);

    ret = itrain_invocate_action(itrain, "get_szyc", json_builder_get_root(builder), &response);
    if (ret) {
        JsonObject *items = json_object_get_object_member(json_node_get_object(response), "items");
        payload->carriage_num = json_object_get_int_member(items, "carriage_num");
        payload->position_num = json_object_get_int_member(items, "position_num");
    }

    json_builder_reset(builder);

    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "items");
    json_builder_begin_array(builder);
    json_builder_add_string_value(builder, "firmware");
    json_builder_add_string_value(builder, "manufacturer");
    json_builder_add_string_value(builder, "model");
    json_builder_end_array(builder);
    json_builder_end_object(builder);
    ret = itrain_invocate_action(itrain, "get_base_info", json_builder_get_root(builder), &response);
    if (ret) {
        const gchar *ver;
        guint maj, min, rev;
        JsonObject *items = json_object_get_object_member(json_node_get_object(response), "items");
        ver = json_object_get_string_member(items, "version");
        if (sscanf(ver, "%d.%d.%d", &maj, &min, &rev) == 3)
            htons(payload->version = maj * 100 + min * 10 + rev);
        strncpy((char *)payload->manufacturer, "EASYWAY", sizeof(payload->manufacturer));
    }
    /* always online */
    payload->online_state = 0x01;

    g_object_unref(builder);

    return ret;
}

gboolean
ipcam_proto_set_image_attr(IpcamConnection *connection, IpcamITrainMessage *message)
{
    SetImageAttrRequest *img_attr;
    guint size;
    IpcamITrain *itrain = (IpcamITrain *)ipcam_connection_get_userdata (connection);

    g_assert(IPCAM_IS_ITRAIN(itrain));

    img_attr = ipcam_itrain_message_get_payload(message, &size);
    if (img_attr && size >= sizeof(*img_attr)) {
        ipcam_proto_do_set_image_attr(itrain, img_attr);
        return TRUE;
    }
    else {
        g_warning("%s: payload size is too small.\n", __func__);
    }
    return FALSE;
}

gboolean
ipcam_proto_get_image_attr(IpcamConnection *connection, IpcamITrainMessage *message)
{
    GetImageAttrResponse *img_attr;
    IpcamITrainMessage *itrain_resp;
    IpcamITrain *itrain = (IpcamITrain *)ipcam_connection_get_userdata (connection);

    g_assert(IPCAM_IS_ITRAIN(itrain));

    img_attr = g_malloc0(sizeof(*img_attr));
    g_return_val_if_fail(img_attr, FALSE);

    if (ipcam_proto_do_get_image_attr (itrain, img_attr)) {
        itrain_resp = g_object_new(IPCAM_TYPE_ITRAIN_MESSAGE, NULL);
        ipcam_itrain_message_set_message_type (itrain_resp, MSGTYPE_GETIMAGEATTR_RESPONSE);
        ipcam_itrain_message_set_payload (itrain_resp, img_attr, sizeof(*img_attr));
        ipcam_connection_send_message (connection, itrain_resp);
        g_object_unref(itrain_resp);
        return TRUE;
    }
    return FALSE;
}

gboolean
ipcam_proto_set_osd(IpcamConnection *connection, IpcamITrainMessage *message)
{
    SetOsdRequest *osd;
    guint size;
    IpcamITrain *itrain = (IpcamITrain *)ipcam_connection_get_userdata (connection);

    g_assert(IPCAM_IS_ITRAIN(itrain));

    osd = ipcam_itrain_message_get_payload(message, &size);
    if (osd && size >= sizeof(*osd)) {
        ipcam_proto_do_set_osd(itrain, osd);
        return TRUE;
    }
    else {
        g_warning("%s: payload size is too small.\n", __func__);
    }
    return FALSE;
}

gboolean
ipcam_proto_timesync(IpcamConnection *connection, IpcamITrainMessage *message)
{
    TimeSyncRequest *timesync;
    guint size;
    IpcamITrain *itrain = (IpcamITrain *)ipcam_connection_get_userdata (connection);

    g_assert(IPCAM_IS_ITRAIN(itrain));

    timesync = ipcam_itrain_message_get_payload(message, &size);
    if (timesync && size >= sizeof(*timesync)) {
        ipcam_proto_do_timesync (itrain, timesync);
        return TRUE;
    }
    else {
        g_warning("%s: payload size is too small.\n", __func__);
    }
    return FALSE;
}

gboolean
ipcam_proto_query_status(IpcamConnection *connection, IpcamITrainMessage *message)
{
    QueryStatusResponse *status;
    IpcamITrainMessage *itrain_resp;
    IpcamITrain *itrain = (IpcamITrain *)ipcam_connection_get_userdata (connection);

    g_assert(IPCAM_IS_ITRAIN(itrain));

    status = g_malloc0(sizeof(*status));
    g_return_val_if_fail(status, FALSE);

    if (ipcam_proto_do_query_status (itrain, status)) {
        itrain_resp = g_object_new(IPCAM_TYPE_ITRAIN_MESSAGE, NULL);
        ipcam_itrain_message_set_message_type (itrain_resp, MSGTYPE_GETIMAGEATTR_RESPONSE);
        ipcam_itrain_message_set_payload (itrain_resp, status, sizeof(*status));
        ipcam_connection_send_message (connection, itrain_resp);
        g_object_unref(itrain_resp);
        return TRUE;
    }
    return FALSE;
}

void ipcam_proto_register_all_handlers(IpcamConnection *connection)
{
    ipcam_connection_register_message_handler(connection,
                                              MSGTYPE_SETIMAGEATTR_REQUEST,
                                              ipcam_proto_set_image_attr);
    ipcam_connection_register_message_handler(connection,
                                              MSGTYPE_GETIMAGEATTR_REQUEST,
                                              ipcam_proto_get_image_attr);
    ipcam_connection_register_message_handler(connection,
                                              MSGTYPE_SETOSD_REQUEST,
                                              ipcam_proto_set_osd);
    ipcam_connection_register_message_handler(connection,
                                              MSGTYPE_TIMESYNC_REQUEST,
                                              ipcam_proto_timesync);
    ipcam_connection_register_message_handler(connection,
                                              MSGTYPE_QUERYSTATUS_REQUEST,
                                              ipcam_proto_query_status);
}