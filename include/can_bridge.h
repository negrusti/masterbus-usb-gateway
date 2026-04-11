#pragma once

#include <stdbool.h>

#include "gateway_packets.h"
#include "hid_bridge.h"

void can_bridge_init(void);
bool can_bridge_send_frame(const gw_can_frame_t *frame);
void can_bridge_poll(void);
bool can_bridge_pop_hid_report(gw_hid_report_view_t *out);
uint8_t can_bridge_pending_count(void);
