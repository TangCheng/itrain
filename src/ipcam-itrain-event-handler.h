/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * ipcam-itrain-event-handler.h
 * Copyright (C) 2015 Watson Xu <xuhuashan@gmail.com>
 *
 * itrain is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * itrain is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _IPCAM_ITRAIN_EVENT_HANDLER_H_
#define _IPCAM_ITRAIN_EVENT_HANDLER_H_

#include <event_handler.h>

G_BEGIN_DECLS

#define IPCAM_TYPE_ITRAIN_EVENT_HANDLER             (ipcam_itrain_event_handler_get_type ())
#define IPCAM_ITRAIN_EVENT_HANDLER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), IPCAM_TYPE_ITRAIN_EVENT_HANDLER, IpcamITrainEventHandler))
#define IPCAM_ITRAIN_EVENT_HANDLER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), IPCAM_TYPE_ITRAIN_EVENT_HANDLER, IpcamITrainEventHandlerClass))
#define IPCAM_IS_ITRAIN_EVENT_HANDLER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IPCAM_TYPE_ITRAIN_EVENT_HANDLER))
#define IPCAM_IS_ITRAIN_EVENT_HANDLER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), IPCAM_TYPE_ITRAIN_EVENT_HANDLER))
#define IPCAM_ITRAIN_EVENT_HANDLER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), IPCAM_TYPE_ITRAIN_EVENT_HANDLER, IpcamITrainEventHandlerClass))

typedef struct _IpcamITrainEventHandlerClass IpcamITrainEventHandlerClass;
typedef struct _IpcamITrainEventHandler IpcamITrainEventHandler;


struct _IpcamITrainEventHandlerClass
{
    IpcamEventHandlerClass parent_class;
};

struct _IpcamITrainEventHandler
{
    IpcamEventHandler parent_instance;
};

GType ipcam_itrain_event_handler_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* _IPCAM_ITRAIN_EVENT_HANDLER_H_ */

