#include "usbd_customhid_if.h"

#include <string.h>

#include "gateway_packets.h"
#include "usb_device.h"
#include "usbd_core.h"
#include "usbd_customhid.h"

extern USBD_HandleTypeDef g_usb_device_fs;

__ALIGN_BEGIN static uint8_t g_report_desc[USBD_CUSTOM_HID_REPORT_DESC_SIZE] __ALIGN_END = {
    0x06, 0x00, 0xFF,
    0x09, 0x01,
    0xA1, 0x01,
    0x15, 0x00,
    0x26, 0xFF, 0x00,
    0x75, 0x08,
    0x95, 0x40,
    0x09, 0x01,
    0x81, 0x02,
    0x95, 0x40,
    0x09, 0x02,
    0x91, 0x02,
    0x95, 0x40,
    0x09, 0x03,
    0xB1, 0x02,
    0xC0};

static volatile uint8_t g_last_out_report[GW_HID_PAYLOAD_SIZE];
static volatile uint8_t g_out_pending;

static int8_t mb_custom_hid_init(void);
static int8_t mb_custom_hid_deinit(void);
static int8_t mb_custom_hid_out_event(uint8_t event_idx, uint8_t state);

USBD_CUSTOM_HID_ItfTypeDef g_mb_custom_hid_fops = {
    g_report_desc,
    mb_custom_hid_init,
    mb_custom_hid_deinit,
    mb_custom_hid_out_event,
};

static int8_t mb_custom_hid_init(void) {
    memset((uint8_t *)g_last_out_report, 0, sizeof(g_last_out_report));
    g_out_pending = 0U;
    return 0;
}

static int8_t mb_custom_hid_deinit(void) {
    g_out_pending = 0U;
    return 0;
}

static int8_t mb_custom_hid_out_event(uint8_t event_idx, uint8_t state) {
    USBD_CUSTOM_HID_HandleTypeDef *hhid;
    (void)event_idx;
    (void)state;

    hhid = (USBD_CUSTOM_HID_HandleTypeDef *)g_usb_device_fs.pClassData;
    if (hhid != NULL) {
        memcpy((uint8_t *)g_last_out_report, hhid->Report_buf, sizeof(g_last_out_report));
        g_out_pending = 1U;
    }
    return 0;
}

bool usb_device_fetch_out_report(uint8_t payload[GW_HID_PAYLOAD_SIZE]) {
    if (g_out_pending == 0U) {
        return false;
    }
    memcpy(payload, (const uint8_t *)g_last_out_report, GW_HID_PAYLOAD_SIZE);
    g_out_pending = 0U;
    return true;
}

bool usb_device_send_in_report(const uint8_t payload[GW_HID_PAYLOAD_SIZE]) {
    return USBD_CUSTOM_HID_SendReport(&g_usb_device_fs, (uint8_t *)payload, GW_HID_PAYLOAD_SIZE) == USBD_OK;
}
