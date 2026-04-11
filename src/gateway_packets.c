#include "gateway_packets.h"

static uint32_t gw_unpack_low23(const gw_packet14_t *pkt) {
    const uint8_t b0 = pkt->bytes[0];
    const uint8_t b1 = pkt->bytes[1];
    const uint8_t b2 = pkt->bytes[2];
    const uint8_t b3 = pkt->bytes[3];
    const uint32_t top5 = (uint32_t)((((uint16_t)b0 << 8) | b1) >> 5) & 0x1FU;
    const uint32_t low18 = ((uint32_t)(b1 & 0x03U) << 16) | ((uint32_t)b2 << 8) | b3;
    return (top5 << 18) | low18;
}

static void gw_pack_low23(uint32_t low23, gw_packet14_t *pkt) {
    /* The original HID transport splits the 29-bit CAN ID around three flag
       bits in byte 1. Preserve those bits while packing the lower 23 ID bits. */
    pkt->bytes[0] = (uint8_t)((pkt->bytes[0] & 0xFCU) | ((low23 >> 21) & 0x03U));
    pkt->bytes[1] = (uint8_t)(((low23 >> 13) & 0xE0U) | (pkt->bytes[1] & 0x1CU) | ((low23 >> 16) & 0x03U));
    pkt->bytes[2] = (uint8_t)((low23 >> 8) & 0xFFU);
    pkt->bytes[3] = (uint8_t)(low23 & 0xFFU);
}

bool gw_packet14_to_can(const gw_packet14_t *pkt, uint8_t meta_low, gw_can_frame_t *out) {
    uint8_t i;

    if ((pkt == NULL) || (out == NULL)) {
        return false;
    }

    out->ext_id = ((uint32_t)(pkt->bytes[0] >> 2) << 23) | gw_unpack_low23(pkt);
    out->dlc = (uint8_t)(pkt->bytes[4] & 0x0FU);
    for (i = 0; i < 8U; ++i) {
        out->data[i] = pkt->bytes[5U + i];
    }
    out->meta16 = ((uint16_t)pkt->bytes[13] << 8) | meta_low;
    return true;
}

void gw_can_to_packet14(const gw_can_frame_t *frame, gw_packet14_t *pkt, uint8_t *meta_low_out) {
    uint8_t i;
    const uint8_t type = (uint8_t)((frame->ext_id >> 23) & 0x3FU);
    const uint32_t low23 = frame->ext_id & 0x7FFFFFU;

    for (i = 0; i < GW_PACKET_SIZE; ++i) {
        pkt->bytes[i] = 0U;
    }

    pkt->bytes[0] = (uint8_t)(type << 2);
    gw_pack_low23(low23, pkt);
    pkt->bytes[4] = (uint8_t)(frame->dlc & 0x0FU);
    for (i = 0; i < 8U; ++i) {
        pkt->bytes[5U + i] = frame->data[i];
    }
    pkt->bytes[13] = (uint8_t)(frame->meta16 >> 8);
    if (meta_low_out != NULL) {
        *meta_low_out = (uint8_t)(frame->meta16 & 0xFFU);
    }
}
