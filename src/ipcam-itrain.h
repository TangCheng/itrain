#ifndef __ITRAIN_H__
#define __ITRAIN_H__

#include <json-glib/json-glib.h>
#include <gio/gio.h>
#include <base_app.h>

#define IPCAM_TYPE_ITRAIN (ipcam_itrain_get_type())
#define IPCAM_ITRAIN(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), IPCAM_TYPE_ITRAIN, IpcamITrain))
#define IPCAM_ITRAIN_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), IPCAM_TYPE_ITRAIN, IpcamITrainClass))
#define IPCAM_IS_ITRAIN(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), IPCAM_TYPE_ITRAIN))
#define IPCAM_IS_ITRAIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), IPCAM_TYPE_ITRAIN))
#define IPCAM_ITRAIN_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), IPCAM_TYPE_ITRAIN, IpcamITrainClass))

typedef struct _IpcamITrain IpcamITrain;
typedef struct _IpcamITrainClass IpcamITrainClass;

struct _IpcamITrain
{
  IpcamBaseApp parent;
};

struct _IpcamITrainClass
{
  IpcamBaseAppClass parent_class;
};


GType ipcam_itrain_get_type(void);

const gpointer ipcam_itrain_get_property(IpcamITrain *itrain, const gchar *key);
void ipcam_itrain_set_property(IpcamITrain *itrain, const gchar *key, gpointer value);

static inline const gchar *ipcam_itrain_get_string_property(IpcamITrain *itrain, const gchar *key)
{
	return (const gchar *)ipcam_itrain_get_property(itrain, key);
}

static inline void ipcam_itrain_set_string_property(IpcamITrain *itrain, const gchar *key, const gchar *value)
{
	ipcam_itrain_set_property(itrain, g_strdup(key), g_strdup(value));
}

static inline gint ipcam_itrain_get_int_property(IpcamITrain *itrain, const gchar *key)
{
	gpointer pvalue = ipcam_itrain_get_property(itrain, key);

	return pvalue ? *(gint *)pvalue : -1;
}

static inline void ipcam_itrain_set_int_property(IpcamITrain *itrain, gchar *key, gint value)
{
	gint *pvalue = g_malloc(sizeof(value));
	*pvalue = value;
	ipcam_itrain_set_property(itrain, g_strdup(key), pvalue);
}

void ipcam_itrain_video_occlusion_handler(IpcamITrain *itrain, JsonNode *body);
void ipcam_itrain_update_base_info_setting(IpcamITrain *itrain, JsonNode *body);
void ipcam_itrain_update_szyc_setting(IpcamITrain *itrain, JsonNode *body);

#endif /* __ITRAIN_H__ */
