#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <json-glib/json-glib.h>
#include <request_message.h>
#include "itrain.h"

#include "ipcam-itrain-server.h"
#include "ipcam-itrain-event-handler.h"

struct _IpcamITrainTimer
{
    gpointer                user_data;
    guint                   timeout_sec;
    guint                   counter;
    IpcamITrainTimerHandler *handler;
};

typedef struct _IpcamITrainPrivate
{
    IpcamITrainServer       *itrain_server;
    GMutex                  timer_lock;
    GHashTable              *timer_hash;
    GMutex                  prop_mutex;
    GHashTable              *cached_properties;
} IpcamITrainPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(IpcamITrain, ipcam_itrain, IPCAM_BASE_APP_TYPE);

static void ipcam_itrain_before_start(IpcamBaseService *base_service);
static void ipcam_itrain_in_loop(IpcamBaseService *base_service);
static void ipcam_itrain_timer_handler(GObject *obj);
static void base_info_message_handler(GObject *obj, IpcamMessage *msg, gboolean timeout);
static void szyc_message_handler(GObject *obj, IpcamMessage *msg, gboolean timeout);

static void ipcam_itrain_finalize(GObject *object)
{
    IpcamITrainPrivate *priv = ipcam_itrain_get_instance_private(IPCAM_ITRAIN(object));

    g_object_unref(priv->itrain_server);

    g_mutex_lock(&priv->timer_lock);
    g_hash_table_destroy(priv->timer_hash);
    g_mutex_unlock(&priv->timer_lock);
    g_mutex_clear(&priv->timer_lock);

    g_mutex_lock(&priv->prop_mutex);
    g_hash_table_destroy(priv->cached_properties);
    g_mutex_unlock(&priv->prop_mutex);
    g_mutex_clear(&priv->prop_mutex);
    
    G_OBJECT_CLASS(ipcam_itrain_parent_class)->finalize(object);
}

static void ipcam_itrain_init(IpcamITrain *self)
{
    IpcamITrainPrivate *priv = ipcam_itrain_get_instance_private(self);

    g_mutex_init(&priv->timer_lock);
    priv->timer_hash = g_hash_table_new_full(g_direct_hash,
                                             g_direct_equal,
                                             NULL,
                                             g_free);
    priv->cached_properties = g_hash_table_new_full(g_str_hash, g_str_equal,
                                                    g_free, g_free);
}

static void ipcam_itrain_class_init(IpcamITrainClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = &ipcam_itrain_finalize;
    
    IpcamBaseServiceClass *base_service_class = IPCAM_BASE_SERVICE_CLASS(klass);
    base_service_class->before = ipcam_itrain_before_start;
    base_service_class->in_loop = ipcam_itrain_in_loop;
}

static void ipcam_itrain_before_start(IpcamBaseService *base_service)
{
    IpcamITrain *itrain = IPCAM_ITRAIN(base_service);
    IpcamITrainPrivate *priv = ipcam_itrain_get_instance_private(itrain);
    const gchar *addr = ipcam_base_app_get_config(IPCAM_BASE_APP(itrain), "itrain:address");
    const gchar *port = ipcam_base_app_get_config(IPCAM_BASE_APP(itrain), "itrain:port");
	JsonBuilder *builder;
	const gchar *token = ipcam_base_app_get_config(IPCAM_BASE_APP(itrain), "token");
	IpcamRequestMessage *req_msg;

    if (!addr || !port)
    {
        g_critical("address and port must be specified.\n");
        return;
    }
    priv->itrain_server = g_object_new(IPCAM_TYPE_ITRAIN_SERVER,
                                       "itrain", itrain,
                                       "address", addr,
                                       "port", strtoul(port, NULL, 0),
                                       NULL);
    ipcam_base_app_add_timer(IPCAM_BASE_APP(itrain),
                             "itrain-timer-source", "1",
                             ipcam_itrain_timer_handler);

    ipcam_base_app_register_notice_handler(IPCAM_BASE_APP(itrain), "video_occlusion_event", IPCAM_TYPE_ITRAIN_EVENT_HANDLER);
    ipcam_base_app_register_notice_handler(IPCAM_BASE_APP(itrain), "set_base_info", IPCAM_TYPE_ITRAIN_EVENT_HANDLER);
    ipcam_base_app_register_notice_handler(IPCAM_BASE_APP(itrain), "set_szyc", IPCAM_TYPE_ITRAIN_EVENT_HANDLER);

	/* Request the Base Information */
	builder = json_builder_new();
	json_builder_begin_object(builder);
	json_builder_set_member_name(builder, "items");
	json_builder_begin_array(builder);
	json_builder_add_string_value(builder, "device_name");
	json_builder_add_string_value(builder, "comment");
	json_builder_add_string_value(builder, "location");
	json_builder_add_string_value(builder, "hardware");
	json_builder_add_string_value(builder, "firmware");
	json_builder_add_string_value(builder, "manufacturer");
	json_builder_add_string_value(builder, "model");
	json_builder_add_string_value(builder, "serial");
    json_builder_add_string_value(builder, "device_type");
	json_builder_end_array(builder);
	json_builder_end_object(builder);
	req_msg = g_object_new(IPCAM_REQUEST_MESSAGE_TYPE,
	                       "action", "get_base_info",
	                       "body", json_builder_get_root(builder),
	                       NULL);
	ipcam_base_app_send_message(IPCAM_BASE_APP(itrain), IPCAM_MESSAGE(req_msg),
	                            "iconfig", token,
	                            base_info_message_handler, 5);
	g_object_unref(req_msg);
	g_object_unref(builder);

    /* Request the SZYC Information */
	builder = json_builder_new();
	json_builder_begin_object(builder);
	json_builder_set_member_name(builder, "items");
	json_builder_begin_array(builder);
	json_builder_add_string_value(builder, "train_num");
	json_builder_add_string_value(builder, "carriage_num");
	json_builder_add_string_value(builder, "position_num");
	json_builder_end_array(builder);
	json_builder_end_object(builder);
	req_msg = g_object_new(IPCAM_REQUEST_MESSAGE_TYPE,
	                       "action", "get_szyc",
	                       "body", json_builder_get_root(builder),
	                       NULL);
	ipcam_base_app_send_message(IPCAM_BASE_APP(itrain), IPCAM_MESSAGE(req_msg),
	                            "iconfig", token,
	                            szyc_message_handler, 5);
	g_object_unref(req_msg);
	g_object_unref(builder);
}

static void ipcam_itrain_in_loop(IpcamBaseService *base_service)
{
}

const gpointer ipcam_itrain_get_property(IpcamITrain *itrain, const gchar *key)
{
    IpcamITrainPrivate *priv = ipcam_itrain_get_instance_private(itrain);
    gpointer ret;

    g_mutex_lock(&priv->prop_mutex);
    ret = g_hash_table_lookup(priv->cached_properties, key);
    g_mutex_unlock(&priv->prop_mutex);

    return ret;
}

void ipcam_itrain_set_property(IpcamITrain *itrain, const gchar *key, gpointer value)
{
    IpcamITrainPrivate *priv = ipcam_itrain_get_instance_private(itrain);

    g_mutex_lock(&priv->prop_mutex);
    g_hash_table_insert(priv->cached_properties, (gpointer)key, value);
    g_mutex_unlock(&priv->prop_mutex);
}

static void
ipcam_itrain_timer_handler(GObject *obj)
{
    GHashTableIter iter;
    gpointer key, value;
    IpcamITrainPrivate *priv = ipcam_itrain_get_instance_private(IPCAM_ITRAIN(obj));

    g_mutex_lock(&priv->timer_lock);
    g_hash_table_iter_init(&iter, priv->timer_hash);
    while (g_hash_table_iter_next (&iter, &key, &value)) {
        IpcamITrainTimer *timer = (IpcamITrainTimer *)key;

        g_assert(timer && timer->handler);

        if (--timer->counter == 0) {
            timer->handler(timer->user_data);
            timer->counter = timer->timeout_sec;    /* Reload the counter */
        }
    }
    g_mutex_unlock(&priv->timer_lock);
}

IpcamITrainTimer *
ipcam_itrain_add_timer(IpcamITrain *itrain, guint timeout_sec,
                       IpcamITrainTimerHandler *handler, gpointer user_data)
{
    IpcamITrainTimer *timer;
    IpcamITrainPrivate *priv = ipcam_itrain_get_instance_private(itrain);

    timer = g_malloc0(sizeof(*timer));
    if (!timer)
        return NULL;

    timer->user_data = user_data;
    timer->timeout_sec = timeout_sec;
    timer->counter = timeout_sec;
    timer->handler = handler;

    g_mutex_lock(&priv->timer_lock);
    g_hash_table_add(priv->timer_hash, timer);
    g_mutex_unlock(&priv->timer_lock);

    return timer;
}

void ipcam_itrain_del_timer(IpcamITrain *itrain, IpcamITrainTimer *timer)
{
    IpcamITrainPrivate *priv = ipcam_itrain_get_instance_private(itrain);

    g_mutex_lock(&priv->timer_lock);
    g_hash_table_remove(priv->timer_hash, timer);
    g_mutex_unlock (&priv->timer_lock);
}

void ipcam_itrain_timer_reset(IpcamITrainTimer *timer)
{
    timer->counter = timer->timeout_sec;
}

void ipcam_itrain_video_occlusion_handler(IpcamITrain *itrain, JsonNode *body)
{
    IpcamITrainPrivate *priv = ipcam_itrain_get_instance_private(itrain);
    JsonObject *evt_obj = json_object_get_object_member(json_node_get_object(body), "event");
    gint region = -1;
    gint state = -1;
    const gchar *carriage_num;
    const gchar *position_num;

    g_return_if_fail(evt_obj);

    carriage_num = ipcam_itrain_get_string_property(itrain, "szyc:carriage_num");
    position_num = ipcam_itrain_get_string_property(itrain, "szyc:position_num");

    if (json_object_has_member(evt_obj, "region"))
        region = json_object_get_int_member(evt_obj, "region");
    if (json_object_has_member(evt_obj, "state"))
        state = json_object_get_boolean_member(evt_obj, "state");

    if (region >= 0 && state >= 0 && carriage_num && position_num) {
        ipcam_itrain_server_report_state(priv->itrain_server,
                                         strtoul(carriage_num, NULL, 0),
                                         strtoul(position_num, NULL, 0),
                                         state,
                                         0);
    }
}

void ipcam_itrain_update_base_info_setting(IpcamITrain *itrain, JsonNode *body)
{
    JsonObject *items_obj = json_object_get_object_member(json_node_get_object(body), "items");
	GList *members, *item;

	members = json_object_get_members(items_obj);
	for (item = g_list_first(members); item; item = g_list_next(item)) {
		const gchar *name = (const gchar *)item->data;
		gchar *key;
		if (asprintf(&key, "base_info:%s", (const gchar *)item->data) > 0) {
			const gchar *value = json_object_get_string_member(items_obj, name);
			ipcam_itrain_set_string_property(itrain, key, value);
			g_free(key);
		}
	}
    g_list_free(members);
}

static void base_info_message_handler(GObject *obj, IpcamMessage *msg, gboolean timeout)
{
	IpcamITrain *itrain = IPCAM_ITRAIN(obj);
	g_assert(IPCAM_IS_ITRAIN(itrain));

	if (!timeout && msg) {
		JsonNode *body;
		g_object_get(msg, "body", &body, NULL);
		if (body)
			ipcam_itrain_update_base_info_setting(itrain, body);
	}
}

void ipcam_itrain_update_szyc_setting(IpcamITrain *itrain, JsonNode *body)
{
    JsonObject *items_obj = json_object_get_object_member(json_node_get_object(body), "items");
	GList *members, *item;

	members = json_object_get_members(items_obj);
	for (item = g_list_first(members); item; item = g_list_next(item)) {
		const gchar *name = (const gchar *)item->data;
		gchar *key;
		if (asprintf(&key, "szyc:%s", (const gchar *)item->data) > 0) {
			const gchar *value = json_object_get_string_member(items_obj, name);
			ipcam_itrain_set_string_property(itrain, key, value);
			g_free(key);
		}
	}
    g_list_free(members);
}

static void szyc_message_handler(GObject *obj, IpcamMessage *msg, gboolean timeout)
{
	IpcamITrain *itrain = IPCAM_ITRAIN(obj);
	g_assert(IPCAM_IS_ITRAIN(itrain));

	if (!timeout && msg) {
		JsonNode *body;
		g_object_get(msg, "body", &body, NULL);
		if (body)
			ipcam_itrain_update_szyc_setting(itrain, body);
	}
}
