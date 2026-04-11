#include "hid_bridge.h"

#include <string.h>

bool gw_hid_decode_report(const uint8_t payload[GW_HID_PAYLOAD_SIZE], gw_hid_report_view_t *out) {
    uint8_t i;

    if ((payload == NULL) || (out == NULL)) {
        return false;
    }

    memset(out, 0, sizeof(*out));
    out->count = payload[0];
    if (out->count > GW_HID_MAX_PACKETS) {
        return false;
    }

    for (i = 0; i < out->count; ++i) {
        memcpy(out->slots[i].bytes, &payload[1U + (GW_PACKET_SIZE * i)], GW_PACKET_SIZE);
        out->meta_low[i] = payload[60U + i];
    }

    return true;
}

void gw_hid_encode_report(const gw_hid_report_view_t *view, uint8_t payload[GW_HID_PAYLOAD_SIZE]) {
    uint8_t i;

    memset(payload, 0, GW_HID_PAYLOAD_SIZE);
    payload[0] = view->count;

    for (i = 0; i < GW_HID_MAX_PACKETS; ++i) {
        memcpy(&payload[1U + (GW_PACKET_SIZE * i)], view->slots[i].bytes, GW_PACKET_SIZE);
        payload[60U + i] = view->meta_low[i];
    }
    /* Keep the reserved tail distinct from the four packed 14-byte slots. */
    memcpy(&payload[57], view->reserved_tail, sizeof(view->reserved_tail));
}
