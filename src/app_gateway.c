#include "app_gateway.h"

#include <string.h>

#include "board_leds.h"
#include "can_bridge.h"
#include "gateway_packets.h"
#include "hid_bridge.h"
#include "usb_device.h"

#include "stm32f0xx_hal.h"

#define MB_HID_IN_BATCH_MS 1U

static uint8_t g_out_payload[GW_HID_PAYLOAD_SIZE];
static uint8_t g_in_payload[GW_HID_PAYLOAD_SIZE];
static uint8_t g_in_pending;
static uint8_t g_connected;
static uint8_t g_batch_armed;
static uint32_t g_batch_deadline_ms;

void app_gateway_init(void) {
    memset(g_out_payload, 0, sizeof(g_out_payload));
    memset(g_in_payload, 0, sizeof(g_in_payload));
    g_in_pending = 0U;
    g_connected = 0U;
    g_batch_armed = 0U;
    g_batch_deadline_ms = 0U;

    board_leds_init();
    can_bridge_init();
    usb_device_init();
}

void app_gateway_poll(void) {
    const uint32_t now_ms = HAL_GetTick();
    uint8_t pending_can;
    gw_hid_report_view_t hid_view;

    usb_device_poll();
    can_bridge_poll();
    board_leds_poll(now_ms);

    if (usb_device_fetch_out_report(g_out_payload)) {
        uint8_t i;

        if (g_connected == 0U) {
            g_connected = 1U;
            board_leds_set_connected(1U);
        }

        if (gw_hid_decode_report(g_out_payload, &hid_view)) {
            for (i = 0U; i < hid_view.count; ++i) {
                gw_can_frame_t frame;

                if (!gw_packet14_to_can(&hid_view.slots[i], hid_view.meta_low[i], &frame)) {
                    continue;
                }
                (void)can_bridge_send_frame(&frame);
            }
        }
    }

    pending_can = can_bridge_pending_count();

    if ((g_in_pending == 0U) && (pending_can != 0U) && (g_batch_armed == 0U)) {
        /* Give CAN replies one USB frame to coalesce; this preserves the
           original gateway's bundled HID reports without adding the old 3 ms
           startup latency. */
        g_batch_armed = 1U;
        g_batch_deadline_ms = now_ms + MB_HID_IN_BATCH_MS;
    }

    if ((g_in_pending == 0U) && (pending_can != 0U) &&
        ((pending_can >= GW_HID_MAX_PACKETS) || (g_batch_armed != 0U && (int32_t)(now_ms - g_batch_deadline_ms) >= 0))) {
        if (can_bridge_pop_hid_report(&hid_view)) {
            gw_hid_encode_report(&hid_view, g_in_payload);
            g_in_pending = 1U;
        }
        g_batch_armed = 0U;
    }

    if (g_in_pending != 0U) {
        if (usb_device_send_in_report(g_in_payload)) {
            g_in_pending = 0U;
        }
    }

    if ((g_in_pending == 0U) && (can_bridge_pending_count() == 0U)) {
        g_batch_armed = 0U;
    }
}
