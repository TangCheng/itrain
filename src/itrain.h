#ifndef __ITRAIN_H__
#define __ITRAIN_H__

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

typedef void IpcamITrainTimerHandler(gpointer user_data);

struct _IpcamITrainTimer;
typedef struct _IpcamITrainTimer IpcamITrainTimer;

GType ipcam_itrain_get_type(void);

IpcamITrainTimer *ipcam_itrain_add_timer(IpcamITrain *itrain,
                                         guint timeout_sec,
                                         IpcamITrainTimerHandler *handler,
                                         gpointer user_data);
void ipcam_itrain_del_timer(IpcamITrain *itrain, IpcamITrainTimer *timer);
void ipcam_itrain_timer_reset(IpcamITrainTimer *timer);

#endif /* __ITRAIN_H__ */
