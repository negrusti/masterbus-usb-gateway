#include "usbd_desc.h"
#include "usbd_conf.h"
#include "usbd_ctlreq.h"

#ifndef MB_USB_VID
#define MB_USB_VID 0x1A64U
#endif

#ifndef MB_USB_PID
#define MB_USB_PID 0x0000U
#endif

#define USBD_LANGID_STRING 0x0409U
#define USBD_MANUFACTURER_STRING "Generic"
#define USBD_PRODUCT_STRING "MasterBus Link"
#define USBD_CONFIGURATION_STRING "MasterBus Config"
#define USBD_INTERFACE_STRING "MasterBus HID"

static uint8_t *mb_device_descriptor(USBD_SpeedTypeDef speed, uint16_t *length);
static uint8_t *mb_langid_descriptor(USBD_SpeedTypeDef speed, uint16_t *length);
static uint8_t *mb_manufacturer_descriptor(USBD_SpeedTypeDef speed, uint16_t *length);
static uint8_t *mb_product_descriptor(USBD_SpeedTypeDef speed, uint16_t *length);
static uint8_t *mb_serial_descriptor(USBD_SpeedTypeDef speed, uint16_t *length);
static uint8_t *mb_config_descriptor(USBD_SpeedTypeDef speed, uint16_t *length);
static uint8_t *mb_interface_descriptor(USBD_SpeedTypeDef speed, uint16_t *length);

USBD_DescriptorsTypeDef MB_Desc = {
    mb_device_descriptor,
    mb_langid_descriptor,
    mb_manufacturer_descriptor,
    mb_product_descriptor,
    mb_serial_descriptor,
    mb_config_descriptor,
    mb_interface_descriptor,
};

__ALIGN_BEGIN static uint8_t g_device_desc[USB_LEN_DEV_DESC] __ALIGN_END = {
    0x12, USB_DESC_TYPE_DEVICE, 0x00, 0x02, 0x00, 0x00, 0x00, USB_MAX_EP0_SIZE,
    LOBYTE(MB_USB_VID), HIBYTE(MB_USB_VID), LOBYTE(MB_USB_PID), HIBYTE(MB_USB_PID),
    0x00, 0x03, USBD_IDX_MFC_STR, USBD_IDX_PRODUCT_STR, USBD_IDX_SERIAL_STR, 0x02};

__ALIGN_BEGIN static uint8_t g_lang_id_desc[USB_LEN_LANGID_STR_DESC] __ALIGN_END = {
    USB_LEN_LANGID_STR_DESC, USB_DESC_TYPE_STRING, LOBYTE(USBD_LANGID_STRING), HIBYTE(USBD_LANGID_STRING)};

static uint8_t g_serial_desc[USB_SIZ_STRING_SERIAL] = {USB_SIZ_STRING_SERIAL, USB_DESC_TYPE_STRING};
__ALIGN_BEGIN static uint8_t g_str_desc[USBD_MAX_STR_DESC_SIZ] __ALIGN_END;

static void mb_get_serial(void) {
    USBD_GetString((uint8_t *)"000000000", g_serial_desc, &(uint16_t){0});
}

static uint8_t *mb_device_descriptor(USBD_SpeedTypeDef speed, uint16_t *length) {
    (void)speed;
    *length = sizeof(g_device_desc);
    return g_device_desc;
}

static uint8_t *mb_langid_descriptor(USBD_SpeedTypeDef speed, uint16_t *length) {
    (void)speed;
    *length = sizeof(g_lang_id_desc);
    return g_lang_id_desc;
}

static uint8_t *mb_manufacturer_descriptor(USBD_SpeedTypeDef speed, uint16_t *length) {
    (void)speed;
    USBD_GetString((uint8_t *)USBD_MANUFACTURER_STRING, g_str_desc, length);
    return g_str_desc;
}

static uint8_t *mb_product_descriptor(USBD_SpeedTypeDef speed, uint16_t *length) {
    (void)speed;
    USBD_GetString((uint8_t *)USBD_PRODUCT_STRING, g_str_desc, length);
    return g_str_desc;
}

static uint8_t *mb_serial_descriptor(USBD_SpeedTypeDef speed, uint16_t *length) {
    (void)speed;
    *length = USB_SIZ_STRING_SERIAL;
    mb_get_serial();
    return g_serial_desc;
}

static uint8_t *mb_config_descriptor(USBD_SpeedTypeDef speed, uint16_t *length) {
    (void)speed;
    USBD_GetString((uint8_t *)USBD_CONFIGURATION_STRING, g_str_desc, length);
    return g_str_desc;
}

static uint8_t *mb_interface_descriptor(USBD_SpeedTypeDef speed, uint16_t *length) {
    (void)speed;
    USBD_GetString((uint8_t *)USBD_INTERFACE_STRING, g_str_desc, length);
    return g_str_desc;
}
