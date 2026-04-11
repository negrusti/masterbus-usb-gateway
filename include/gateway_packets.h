#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define GW_HID_PAYLOAD_SIZE 64U
#define GW_HID_MAX_PACKETS 4U
#define GW_PACKET_SIZE 14U

typedef struct {
    uint8_t bytes[GW_PACKET_SIZE];
} gw_packet14_t;

typedef struct {
    uint32_t ext_id;
    uint8_t dlc;
    uint8_t data[8];
    uint16_t meta16;
} gw_can_frame_t;

bool gw_packet14_to_can(const gw_packet14_t *pkt, uint8_t meta_low, gw_can_frame_t *out);
void gw_can_to_packet14(const gw_can_frame_t *frame, gw_packet14_t *pkt, uint8_t *meta_low_out);

