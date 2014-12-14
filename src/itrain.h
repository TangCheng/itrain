#ifndef __ITRAIN_H__
#define __ITRAIN_H__

#include <gio/gio.h>
#include <base_app.h>

#define IPCAM_ITRAIN_TYPE (ipcam_itrain_get_type())
#define IPCAM_ITRAIN(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), IPCAM_ITRAIN_TYPE, IpcamItrain))
#define IPCAM_ITRAIN_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), IPCAM_ITRAIN_TYPE, IpcamItrainClass))
#define IPCAM_IS_ITRAIN(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), IPCAM_ITRAIN_TYPE))
#define IPCAM_IS_ITRAIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), IPCAM_ITRAIN_TYPE))
#define IPCAM_ITRAIN_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), IPCAM_ITRAIN_TYPE, IpcamItrainClass))

typedef struct _IpcamItrain IpcamItrain;
typedef struct _IpcamItrainClass IpcamItrainClass;

struct _IpcamItrain
{
  IpcamBaseApp parent;
};

struct _IpcamItrainClass
{
  IpcamBaseAppClass parent_class;
};

GType ipcam_itrain_get_type(void);

#endif /* __ITRAIN_H__ */
