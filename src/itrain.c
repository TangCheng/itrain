#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include "itrain.h"

#include "ipcam-itrain-server.h"

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
    GHashTable              *timer_hash;
    GMutex                  timer_lock;
} IpcamITrainPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(IpcamITrain, ipcam_itrain, IPCAM_BASE_APP_TYPE);

static void ipcam_itrain_before_start(IpcamBaseService *base_service);
static void ipcam_itrain_in_loop(IpcamBaseService *base_service);
static void ipcam_itrain_timer_handler(GObject *obj);

static void ipcam_itrain_finalize(GObject *object)
{
    IpcamITrainPrivate *priv = ipcam_itrain_get_instance_private(IPCAM_ITRAIN(object));

    g_object_unref(priv->itrain_server);

    g_mutex_lock(&priv->timer_lock);
    g_hash_table_destroy(priv->timer_hash);
    g_mutex_unlock(&priv->timer_lock);
    g_mutex_clear(&priv->timer_lock);
    
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
        ipcam_base_app_add_timer(IPCAM_BASE_APP(itrain),
                                 "itrain-timer-source", "1",
                                 ipcam_itrain_timer_handler);
    }
}

static void ipcam_itrain_in_loop(IpcamBaseService *base_service)
{
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
