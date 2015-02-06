/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * ipcam-itrain-event-handler.c
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

#include "itrain.h"
#include "ipcam-itrain-event-handler.h"


G_DEFINE_TYPE (IpcamITrainEventHandler, ipcam_itrain_event_handler, IPCAM_EVENT_HANDLER_TYPE);

static void
ipcam_itrain_event_handler_init (IpcamITrainEventHandler *ipcam_itrain_event_handler)
{
}

static void
ipcam_itrain_event_handler_run_impl(IpcamEventHandler *event_handler,
                                    IpcamMessage *message);

static void
ipcam_itrain_event_handler_class_init (IpcamITrainEventHandlerClass *klass)
{
    IpcamEventHandlerClass *event_handler_class = IPCAM_EVENT_HANDLER_CLASS(klass);
    event_handler_class->run = ipcam_itrain_event_handler_run_impl;
}

static void
ipcam_itrain_event_handler_run_impl(IpcamEventHandler *event_handler,
                                    IpcamMessage *message)
{
    IpcamITrainEventHandler *itrain_event_handler = IPCAM_ITRAIN_EVENT_HANDLER(event_handler);
    IpcamITrain *itrain;
    const gchar *event;
    JsonNode *body;

    g_object_get(G_OBJECT(message), "event", &event, NULL);
    g_object_get(G_OBJECT(itrain_event_handler), "service", &itrain, NULL);
    g_object_get(G_OBJECT(message), "body", &body, NULL);

    if (g_strcmp0(event, "video_occlusion_event") == 0)
        ipcam_itrain_video_occlusion_handler(itrain, body);
    else if (g_strcmp0(event, "set_base_info") == 0)
        ipcam_itrain_update_base_info_setting(itrain, body);
    else if (g_strcmp0(event, "set_szyc") == 0)
        ipcam_itrain_update_szyc_setting(itrain, body);

    g_free(event);
}
