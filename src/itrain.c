#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include "itrain.h"

typedef struct _IpcamItrainPrivate
{
    GHashTable *connection_hash;
    GMutex connection_mutex;
} IpcamItrainPrivate;

typedef struct _IpcamItrainConnectionHashValue
{
    GSocket *socket;
    guint timeout;
} IpcamItrainConnectionHashValue;

#define CONNECTION IpcamItrainConnectionHashValue

G_DEFINE_TYPE_WITH_PRIVATE(IpcamItrain, ipcam_itrain, IPCAM_BASE_APP_TYPE);

static void ipcam_itrain_before_start(IpcamBaseService *base_service);
static void ipcam_itrain_in_loop(IpcamBaseService *base_service);

static void connection_value_destroy_func(gpointer value)
{
    CONNECTION *conn = (CONNECTION *)value;
    g_free(conn);
}

static void ipcam_itrain_finalize(GObject *object)
{
    IpcamItrainPrivate *priv = ipcam_itrain_get_instance_private(IPCAM_ITRAIN(object));
    g_mutex_lock(&priv->connection_mutex);
    g_hash_table_remove_all(priv->connection_hash);
    g_object_unref(priv->connection_hash);
    g_mutex_unlock(&priv->connection_mutex);
    g_mutex_clear(&priv->connection_mutex);
    
    G_OBJECT_CLASS(ipcam_itrain_parent_class)->finalize(object);
}

static void ipcam_itrain_init(IpcamItrain *self)
{
    IpcamItrainPrivate *priv = ipcam_itrain_get_instance_private(self);
    g_mutex_init(&priv->connection_mutex);
    priv->connection_hash = g_hash_table_new_full(g_str_hash,
                                                  g_str_equal,
                                                  g_free,
                                                  connection_value_destroy_func);
}

static void ipcam_itrain_class_init(IpcamItrainClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = &ipcam_itrain_finalize;
    
    IpcamBaseServiceClass *base_service_class = IPCAM_BASE_SERVICE_CLASS(klass);
    base_service_class->before = ipcam_itrain_before_start;
    base_service_class->in_loop = ipcam_itrain_in_loop;
}

static void ipcam_itrain_before_start(IpcamBaseService *base_service)
{
    IpcamItrain *itrain = IPCAM_ITRAIN(base_service);
    IpcamItrainPrivate *priv = ipcam_itrain_get_instance_private(itrain);
    const gchar *ajax_addr = ipcam_base_app_get_config(IPCAM_BASE_APP(itrain), "ajax:address");
    const gchar *port = ipcam_base_app_get_config(IPCAM_BASE_APP(itrain), "ajax:port");

    if (ajax_addr != NULL && port != NULL)
    {
    }
}

static void ipcam_itrain_in_loop(IpcamBaseService *base_service)
{
}


