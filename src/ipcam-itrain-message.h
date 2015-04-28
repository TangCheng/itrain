/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * ipcam-proto-message.h
 * Copyright (C) 2014 Watson Xu <xuhuashan@gmail.com>
 *
 */

#ifndef _IPCAM_ITRAIN_MESSAGE_H_
#define _IPCAM_ITRAIN_MESSAGE_H_

#include <glib.h>

#define PACKET_START        0xFF

struct IpcamTrainPDU;
typedef struct IpcamTrainPDU IpcamTrainPDU;

IpcamTrainPDU *ipcam_train_pdu_new(guint8 type, guint16 payload_size);
IpcamTrainPDU *ipcam_train_pdu_new_from_buffer(guint8 *buffer, guint16 buffer_size);
void ipcam_train_pdu_free(IpcamTrainPDU *pdu);

guint8 ipcam_train_pdu_get_type(IpcamTrainPDU *pdu);

void ipcam_train_pdu_set_payload(IpcamTrainPDU *pdu, gpointer payload);
gpointer ipcam_train_pdu_get_payload(IpcamTrainPDU *pdu);
guint16 ipcam_train_pdu_get_payload_size(IpcamTrainPDU *pdu);

gboolean ipcam_train_pdu_verify_checksum(IpcamTrainPDU *pdu);
guint8 ipcam_train_pdu_checksum(IpcamTrainPDU *pdu);
guint8 ipcam_train_pdu_get_checksum(IpcamTrainPDU *pdu);
void   ipcam_train_pdu_set_checksum(IpcamTrainPDU *pdu, guint8 checksum);

gpointer ipcam_train_pdu_get_packet_buffer(IpcamTrainPDU *pdu);
guint16 ipcam_train_pdu_get_packet_size(IpcamTrainPDU *pdu);

#endif /* _IPCAM_ITRAIN_MESSAGE_H_ */

