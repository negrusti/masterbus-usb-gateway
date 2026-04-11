#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "gateway_packets.h"

typedef struct {
    gw_packet14_t slots[GW_HID_MAX_PACKETS];
    uint8_t meta_low[GW_HID_MAX_PACKETS];
    /* Bytes 57..59 are not CAN payload. MasterAdjust depends on the
       original gateway's reserved tail values during the startup scan. */
    uint8_t reserved_tail[3];
    uint8_t count;
} gw_hid_report_view_t;

bool gw_hid_decode_report(const uint8_t payload[GW_HID_PAYLOAD_SIZE], gw_hid_report_view_t *out);
void gw_hid_encode_report(const gw_hid_report_view_t *view, uint8_t payload[GW_HID_PAYLOAD_SIZE]);
