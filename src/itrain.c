#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include "itrain.h"

#include "ipcam-itrain-server.h"

typedef struct _IpcamITrainPrivate
{
    IpcamITrainServer *itrain_server;
    GHashTable *connection_hash;
    GMutex connection_mutex;
} IpcamITrainPrivate;

typedef struct _IpcamITrainConnectionHashValue
{
    GSocket *socket;
    guint timeout;
} IpcamITrainConnectionHashValue;

#define CONNECTION IpcamITrainConnectionHashValue

G_DEFINE_TYPE_WITH_PRIVATE(IpcamITrain, ipcam_itrain, IPCAM_BASE_APP_TYPE);

static void ipcam_itrain_before_start(IpcamBaseService *base_service);
static void ipcam_itrain_in_loop(IpcamBaseService *base_service);

static void connection_value_destroy_func(gpointer value)
{
    CONNECTION *conn = (CONNECTION *)value;
    g_free(conn);
}

static void ipcam_itrain_finalize(GObject *object)
{
    IpcamITrainPrivate *priv = ipcam_itrain_get_instance_private(IPCAM_ITRAIN(object));

    g_object_unref(priv->itrain_server);

    g_mutex_lock(&priv->connection_mutex);
    g_hash_table_remove_all(priv->connection_hash);
    g_object_unref(priv->connection_hash);
    g_mutex_unlock(&priv->connection_mutex);
    g_mutex_clear(&priv->connection_mutex);
    
    G_OBJECT_CLASS(ipcam_itrain_parent_class)->finalize(object);
}

static void ipcam_itrain_init(IpcamITrain *self)
{
    IpcamITrainPrivate *priv = ipcam_itrain_get_instance_private(self);
    g_mutex_init(&priv->connection_mutex);
    priv->connection_hash = g_hash_table_new_full(g_str_hash,
                                                  g_str_equal,
                                                  g_free,
                                                  connection_value_destroy_func);
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

    if (addr != NULL && port != NULL)
    {
        priv->itrain_server = g_object_new(IPCAM_TYPE_ITRAIN_SERVER,
                                           "itrain", itrain,
                                           "address", addr,
                                           "port", strtoul(port, NULL, 0),
                                           NULL);
    }
}

static void ipcam_itrain_in_loop(IpcamBaseService *base_service)
{
}


