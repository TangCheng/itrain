/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*-  */
/*
 * ipcam-itrain-message.c
 * Copyright (C) 2014 Watson Xu <xuhuashan@gmail.com>
 *
 */

#include "ipcam-itrain-message.h"
#include <string.h>
#include <arpa/inet.h>

typedef struct IpcamTrainPDUHeader
{
    guint8 start;   /* should always be 0xff */
    guint8 type;
    guint16 payload_size;
} __attribute__((packed)) IpcamTrainPDUHeader;

struct IpcamTrainPDU
{
    IpcamTrainPDUHeader header;
    guint8 payload[0];
};

static guint8 calculate_checksum(guint8 *p, guint len)
{
    guint8 checksum = 0;
    int i;

    for (i = 0; i < len; i++)
        checksum ^= p[i];

    return checksum;
}

IpcamTrainPDU *ipcam_train_pdu_new(guint8 type, guint16 payload_size)
{
    guint16 packet_size = sizeof(IpcamTrainPDUHeader) + payload_size + 1;
    IpcamTrainPDU *pdu = g_malloc0(packet_size);

    pdu->header.start = PACKET_START;
    pdu->header.type = type;
    pdu->header.payload_size = htons(payload_size);

    return pdu;
}

IpcamTrainPDU *ipcam_train_pdu_new_from_buffer(guint8 *buffer, guint16 buffer_size)
{
    IpcamTrainPDU *pdu;
    IpcamTrainPDUHeader *header = (IpcamTrainPDUHeader *)buffer;
    guint16 payload_size;
    guint16 pkt_size;

    g_return_val_if_fail(buffer_size > sizeof(*header), NULL);
    g_return_val_if_fail(header->start == PACKET_START, NULL);
    
    payload_size = ntohs(header->payload_size);
    g_return_val_if_fail(buffer_size >= sizeof(*header) + payload_size + 1, NULL);

    pkt_size = sizeof(*header) + payload_size + 1;

    pdu = g_malloc(pkt_size);

    if (pdu) {
        memcpy(&pdu->header, buffer, pkt_size);
    }

    return pdu;
}

void ipcam_train_pdu_free(IpcamTrainPDU *pdu)
{
    g_free(pdu);
}

guint8 ipcam_train_pdu_get_type(IpcamTrainPDU *pdu)
{
    return pdu->header.type;
}

void ipcam_train_pdu_set_payload(IpcamTrainPDU *pdu, gpointer payload)
{
    guint16 payload_size = ntohs(pdu->header.payload_size);
    guint8 *buffer;
    guint16 buffer_size;

    if (payload && payload_size > 0)
        memcpy(pdu->payload, payload, payload_size);

    /* recalculate checksum */
    buffer = (guint8 *)&pdu->header;
    buffer_size = sizeof(pdu->header) + payload_size;
    guint8 checksum = calculate_checksum(buffer, buffer_size);
    buffer[buffer_size] = checksum;
}

gpointer ipcam_train_pdu_get_payload(IpcamTrainPDU *pdu)
{
    return pdu->payload;
}

guint16 ipcam_train_pdu_get_payload_size(IpcamTrainPDU *pdu)
{
    return ntohs(pdu->header.payload_size);
}

gboolean ipcam_train_pdu_verify_checksum(IpcamTrainPDU *pdu)
{
    guint16 payload_size = ntohs(pdu->header.payload_size);
    guint8 *buffer = (guint8 *)&pdu->header;
    guint16 buffer_size = sizeof(pdu->header) + payload_size;

    guint8 packet_checksum = buffer[buffer_size];
    guint8 calculated_checksum = calculate_checksum(buffer, buffer_size);

    return (packet_checksum == calculated_checksum);
}

guint8 ipcam_train_pdu_checksum(IpcamTrainPDU *pdu)
{
    guint16 payload_size = ntohs(pdu->header.payload_size);
    guint8 *buffer = (guint8 *)&pdu->header;
    guint16 buffer_size = sizeof(pdu->header) + payload_size;

    return calculate_checksum(buffer, buffer_size);
}

guint8 ipcam_train_pdu_get_checksum(IpcamTrainPDU *pdu)
{
    guint16 payload_size = ntohs(pdu->header.payload_size);
    guint8 *buffer = (guint8 *)&pdu->header;
    guint16 buffer_size = sizeof(pdu->header) + payload_size;

    return buffer[buffer_size];
}

void ipcam_train_pdu_set_checksum(IpcamTrainPDU *pdu, guint8 checksum)
{
    guint16 payload_size = ntohs(pdu->header.payload_size);
    guint8 *buffer = (guint8 *)&pdu->header;
    guint16 buffer_size = sizeof(pdu->header) + payload_size;

    buffer[buffer_size] = checksum;
}

gpointer ipcam_train_pdu_get_packet_buffer(IpcamTrainPDU *pdu)
{
    return &pdu->header;
}

guint16 ipcam_train_pdu_get_packet_size(IpcamTrainPDU *pdu)
{
    guint16 pkt_size;

    /* packet = header + payload + checksum */
    pkt_size = sizeof(pdu->header) +ntohs(pdu->header.payload_size) + 1;

    return pkt_size;
}
