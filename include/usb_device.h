#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "gateway_packets.h"

void usb_device_init(void);
void usb_device_poll(void);
bool usb_device_fetch_out_report(uint8_t payload[GW_HID_PAYLOAD_SIZE]);
bool usb_device_send_in_report(const uint8_t payload[GW_HID_PAYLOAD_SIZE]);
