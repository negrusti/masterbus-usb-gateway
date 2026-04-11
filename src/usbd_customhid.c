#include "usbd_customhid.h"
#include "usbd_ctlreq.h"

static uint8_t USBD_CUSTOM_HID_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_CUSTOM_HID_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_CUSTOM_HID_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t *USBD_CUSTOM_HID_GetFSCfgDesc(uint16_t *length);
static uint8_t *USBD_CUSTOM_HID_GetHSCfgDesc(uint16_t *length);
static uint8_t *USBD_CUSTOM_HID_GetOtherSpeedCfgDesc(uint16_t *length);
static uint8_t *USBD_CUSTOM_HID_GetDeviceQualifierDesc(uint16_t *length);
static uint8_t USBD_CUSTOM_HID_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_CUSTOM_HID_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_CUSTOM_HID_EP0_RxReady(USBD_HandleTypeDef *pdev);

USBD_ClassTypeDef USBD_CUSTOM_HID = {
    USBD_CUSTOM_HID_Init,
    USBD_CUSTOM_HID_DeInit,
    USBD_CUSTOM_HID_Setup,
    NULL,
    USBD_CUSTOM_HID_EP0_RxReady,
    USBD_CUSTOM_HID_DataIn,
    USBD_CUSTOM_HID_DataOut,
    NULL,
    NULL,
    NULL,
    USBD_CUSTOM_HID_GetHSCfgDesc,
    USBD_CUSTOM_HID_GetFSCfgDesc,
    USBD_CUSTOM_HID_GetOtherSpeedCfgDesc,
    USBD_CUSTOM_HID_GetDeviceQualifierDesc,
};

__ALIGN_BEGIN static uint8_t USBD_CUSTOM_HID_CfgFSDesc[USB_CUSTOM_HID_CONFIG_DESC_SIZ] __ALIGN_END = {
    0x09, USB_DESC_TYPE_CONFIGURATION, USB_CUSTOM_HID_CONFIG_DESC_SIZ, 0x00,
    0x01, 0x01, 0x00, 0x80, 0xFA,
    0x09, USB_DESC_TYPE_INTERFACE, 0x00, 0x00, 0x02, 0x03, 0x00, 0x00, 0x00,
    0x09, CUSTOM_HID_DESCRIPTOR_TYPE, 0x01, 0x01, 0x00, 0x01, 0x22, USBD_CUSTOM_HID_REPORT_DESC_SIZE, 0x00,
    0x07, USB_DESC_TYPE_ENDPOINT, CUSTOM_HID_EPIN_ADDR, 0x03, CUSTOM_HID_EPIN_SIZE, 0x00, CUSTOM_HID_FS_BINTERVAL,
    0x07, USB_DESC_TYPE_ENDPOINT, CUSTOM_HID_EPOUT_ADDR, 0x03, CUSTOM_HID_EPOUT_SIZE, 0x00, CUSTOM_HID_FS_BINTERVAL,
};

__ALIGN_BEGIN static uint8_t USBD_CUSTOM_HID_CfgHSDesc[USB_CUSTOM_HID_CONFIG_DESC_SIZ] __ALIGN_END = {
    0x09, USB_DESC_TYPE_CONFIGURATION, USB_CUSTOM_HID_CONFIG_DESC_SIZ, 0x00,
    0x01, 0x01, 0x00, 0x80, 0xFA,
    0x09, USB_DESC_TYPE_INTERFACE, 0x00, 0x00, 0x02, 0x03, 0x00, 0x00, 0x00,
    0x09, CUSTOM_HID_DESCRIPTOR_TYPE, 0x01, 0x01, 0x00, 0x01, 0x22, USBD_CUSTOM_HID_REPORT_DESC_SIZE, 0x00,
    0x07, USB_DESC_TYPE_ENDPOINT, CUSTOM_HID_EPIN_ADDR, 0x03, CUSTOM_HID_EPIN_SIZE, 0x00, CUSTOM_HID_HS_BINTERVAL,
    0x07, USB_DESC_TYPE_ENDPOINT, CUSTOM_HID_EPOUT_ADDR, 0x03, CUSTOM_HID_EPOUT_SIZE, 0x00, CUSTOM_HID_HS_BINTERVAL,
};

__ALIGN_BEGIN static uint8_t USBD_CUSTOM_HID_OtherSpeedCfgDesc[USB_CUSTOM_HID_CONFIG_DESC_SIZ] __ALIGN_END = {
    0x09, USB_DESC_TYPE_CONFIGURATION, USB_CUSTOM_HID_CONFIG_DESC_SIZ, 0x00,
    0x01, 0x01, 0x00, 0x80, 0xFA,
    0x09, USB_DESC_TYPE_INTERFACE, 0x00, 0x00, 0x02, 0x03, 0x00, 0x00, 0x00,
    0x09, CUSTOM_HID_DESCRIPTOR_TYPE, 0x01, 0x01, 0x00, 0x01, 0x22, USBD_CUSTOM_HID_REPORT_DESC_SIZE, 0x00,
    0x07, USB_DESC_TYPE_ENDPOINT, CUSTOM_HID_EPIN_ADDR, 0x03, CUSTOM_HID_EPIN_SIZE, 0x00, CUSTOM_HID_FS_BINTERVAL,
    0x07, USB_DESC_TYPE_ENDPOINT, CUSTOM_HID_EPOUT_ADDR, 0x03, CUSTOM_HID_EPOUT_SIZE, 0x00, CUSTOM_HID_FS_BINTERVAL,
};

__ALIGN_BEGIN static uint8_t USBD_CUSTOM_HID_Desc[USB_CUSTOM_HID_DESC_SIZ] __ALIGN_END = {
    0x09, CUSTOM_HID_DESCRIPTOR_TYPE, 0x01, 0x01, 0x00, 0x01, 0x22, USBD_CUSTOM_HID_REPORT_DESC_SIZE, 0x00,
};

__ALIGN_BEGIN static uint8_t USBD_CUSTOM_HID_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END = {
    USB_LEN_DEV_QUALIFIER_DESC, USB_DESC_TYPE_DEVICE_QUALIFIER, 0x00, 0x02, 0x00, 0x00, 0x00, 0x40, 0x01, 0x00,
};

static uint8_t USBD_CUSTOM_HID_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx) {
    uint8_t ret = 0U;
    USBD_CUSTOM_HID_HandleTypeDef *hhid;
    (void)cfgidx;

    USBD_LL_OpenEP(pdev, CUSTOM_HID_EPIN_ADDR, USBD_EP_TYPE_INTR, CUSTOM_HID_EPIN_SIZE);
    pdev->ep_in[CUSTOM_HID_EPIN_ADDR & 0xFU].is_used = 1U;

    USBD_LL_OpenEP(pdev, CUSTOM_HID_EPOUT_ADDR, USBD_EP_TYPE_INTR, CUSTOM_HID_EPOUT_SIZE);
    pdev->ep_out[CUSTOM_HID_EPOUT_ADDR & 0xFU].is_used = 1U;

    pdev->pClassData = USBD_malloc(sizeof(USBD_CUSTOM_HID_HandleTypeDef));
    if (pdev->pClassData == NULL) {
        ret = 1U;
    } else {
        hhid = (USBD_CUSTOM_HID_HandleTypeDef *)pdev->pClassData;
        hhid->state = CUSTOM_HID_IDLE;
        hhid->Protocol = 0U;
        hhid->IdleState = 0U;
        hhid->AltSetting = 0U;
        hhid->IsReportAvailable = 0U;
        ((USBD_CUSTOM_HID_ItfTypeDef *)pdev->pUserData)->Init();
        USBD_LL_PrepareReceive(pdev, CUSTOM_HID_EPOUT_ADDR, hhid->Report_buf, USBD_CUSTOMHID_OUTREPORT_BUF_SIZE);
    }
    return ret;
}

static uint8_t USBD_CUSTOM_HID_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx) {
    (void)cfgidx;
    USBD_LL_CloseEP(pdev, CUSTOM_HID_EPIN_ADDR);
    pdev->ep_in[CUSTOM_HID_EPIN_ADDR & 0xFU].is_used = 0U;
    USBD_LL_CloseEP(pdev, CUSTOM_HID_EPOUT_ADDR);
    pdev->ep_out[CUSTOM_HID_EPOUT_ADDR & 0xFU].is_used = 0U;
    if (pdev->pClassData != NULL) {
        ((USBD_CUSTOM_HID_ItfTypeDef *)pdev->pUserData)->DeInit();
        USBD_free(pdev->pClassData);
        pdev->pClassData = NULL;
    }
    return USBD_OK;
}

static uint8_t USBD_CUSTOM_HID_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req) {
    USBD_CUSTOM_HID_HandleTypeDef *hhid = (USBD_CUSTOM_HID_HandleTypeDef *)pdev->pClassData;
    uint16_t len = 0U;
    uint8_t *pbuf = NULL;
    uint16_t status_info = 0U;
    uint8_t ret = USBD_OK;

    switch (req->bmRequest & USB_REQ_TYPE_MASK) {
        case USB_REQ_TYPE_CLASS:
            switch (req->bRequest) {
                case CUSTOM_HID_REQ_SET_PROTOCOL:
                    hhid->Protocol = (uint8_t)(req->wValue);
                    break;
                case CUSTOM_HID_REQ_GET_PROTOCOL:
                    USBD_CtlSendData(pdev, (uint8_t *)(void *)&hhid->Protocol, 1U);
                    break;
                case CUSTOM_HID_REQ_SET_IDLE:
                    hhid->IdleState = (uint8_t)(req->wValue >> 8);
                    break;
                case CUSTOM_HID_REQ_GET_IDLE:
                    USBD_CtlSendData(pdev, (uint8_t *)(void *)&hhid->IdleState, 1U);
                    break;
                case CUSTOM_HID_REQ_GET_REPORT:
                    len = MIN(USBD_CUSTOMHID_OUTREPORT_BUF_SIZE, req->wLength);
                    USBD_CtlSendData(pdev, hhid->Report_buf, len);
                    break;
                case CUSTOM_HID_REQ_SET_REPORT: {
                    uint16_t rx_len = req->wLength;
                    if (rx_len > USBD_CUSTOMHID_OUTREPORT_BUF_SIZE) {
                        rx_len = USBD_CUSTOMHID_OUTREPORT_BUF_SIZE;
                    }
                    hhid->IsReportAvailable = 1U;
                    USBD_CtlPrepareRx(pdev, hhid->Report_buf, rx_len);
                    break;
                }
                default:
                    USBD_CtlError(pdev, req);
                    ret = USBD_FAIL;
                    break;
            }
            break;

        case USB_REQ_TYPE_STANDARD:
            switch (req->bRequest) {
                case USB_REQ_GET_STATUS:
                    if (pdev->dev_state == USBD_STATE_CONFIGURED) {
                        USBD_CtlSendData(pdev, (uint8_t *)(void *)&status_info, 2U);
                    } else {
                        USBD_CtlError(pdev, req);
                        ret = USBD_FAIL;
                    }
                    break;
                case USB_REQ_GET_DESCRIPTOR:
                    if ((req->wValue >> 8) == CUSTOM_HID_REPORT_DESC) {
                        len = MIN(USBD_CUSTOM_HID_REPORT_DESC_SIZE, req->wLength);
                        pbuf = ((USBD_CUSTOM_HID_ItfTypeDef *)pdev->pUserData)->pReport;
                    } else if ((req->wValue >> 8) == CUSTOM_HID_DESCRIPTOR_TYPE) {
                        pbuf = USBD_CUSTOM_HID_Desc;
                        len = MIN(USB_CUSTOM_HID_DESC_SIZ, req->wLength);
                    }
                    USBD_CtlSendData(pdev, pbuf, len);
                    break;
                case USB_REQ_GET_INTERFACE:
                    if (pdev->dev_state == USBD_STATE_CONFIGURED) {
                        USBD_CtlSendData(pdev, (uint8_t *)(void *)&hhid->AltSetting, 1U);
                    } else {
                        USBD_CtlError(pdev, req);
                        ret = USBD_FAIL;
                    }
                    break;
                case USB_REQ_SET_INTERFACE:
                    if (pdev->dev_state == USBD_STATE_CONFIGURED) {
                        hhid->AltSetting = (uint8_t)(req->wValue);
                    } else {
                        USBD_CtlError(pdev, req);
                        ret = USBD_FAIL;
                    }
                    break;
                default:
                    USBD_CtlError(pdev, req);
                    ret = USBD_FAIL;
                    break;
            }
            break;

        default:
            USBD_CtlError(pdev, req);
            ret = USBD_FAIL;
            break;
    }
    return ret;
}

uint8_t USBD_CUSTOM_HID_SendReport(USBD_HandleTypeDef *pdev, uint8_t *report, uint16_t len) {
    USBD_CUSTOM_HID_HandleTypeDef *hhid;

    if (pdev == NULL) {
        return USBD_FAIL;
    }

    hhid = (USBD_CUSTOM_HID_HandleTypeDef *)pdev->pClassData;
    if (hhid == NULL) {
        return USBD_FAIL;
    }

    if (pdev->dev_state != USBD_STATE_CONFIGURED) {
        return USBD_BUSY;
    }

    if (len > CUSTOM_HID_EPIN_SIZE) {
        len = CUSTOM_HID_EPIN_SIZE;
    }

    /* The Cube HID class can leave BUSY set across host polls; clearing it here
       matches the gateway-like behavior observed in the working trace. */
    hhid->state = CUSTOM_HID_IDLE;

    if (USBD_LL_Transmit(pdev, CUSTOM_HID_EPIN_ADDR, report, len) != USBD_OK) {
        hhid->state = CUSTOM_HID_IDLE;
        return USBD_FAIL;
    }

    hhid->state = CUSTOM_HID_BUSY;
    return USBD_OK;
}

static uint8_t *USBD_CUSTOM_HID_GetFSCfgDesc(uint16_t *length) {
    *length = sizeof(USBD_CUSTOM_HID_CfgFSDesc);
    return USBD_CUSTOM_HID_CfgFSDesc;
}

static uint8_t *USBD_CUSTOM_HID_GetHSCfgDesc(uint16_t *length) {
    *length = sizeof(USBD_CUSTOM_HID_CfgHSDesc);
    return USBD_CUSTOM_HID_CfgHSDesc;
}

static uint8_t *USBD_CUSTOM_HID_GetOtherSpeedCfgDesc(uint16_t *length) {
    *length = sizeof(USBD_CUSTOM_HID_OtherSpeedCfgDesc);
    return USBD_CUSTOM_HID_OtherSpeedCfgDesc;
}

static uint8_t USBD_CUSTOM_HID_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum) {
    (void)epnum;
    ((USBD_CUSTOM_HID_HandleTypeDef *)pdev->pClassData)->state = CUSTOM_HID_IDLE;
    return USBD_OK;
}

static uint8_t USBD_CUSTOM_HID_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum) {
    USBD_CUSTOM_HID_HandleTypeDef *hhid = (USBD_CUSTOM_HID_HandleTypeDef *)pdev->pClassData;
    (void)epnum;
    ((USBD_CUSTOM_HID_ItfTypeDef *)pdev->pUserData)->OutEvent(hhid->Report_buf[0], hhid->Report_buf[1]);
    USBD_LL_PrepareReceive(pdev, CUSTOM_HID_EPOUT_ADDR, hhid->Report_buf, USBD_CUSTOMHID_OUTREPORT_BUF_SIZE);
    return USBD_OK;
}

static uint8_t USBD_CUSTOM_HID_EP0_RxReady(USBD_HandleTypeDef *pdev) {
    USBD_CUSTOM_HID_HandleTypeDef *hhid = (USBD_CUSTOM_HID_HandleTypeDef *)pdev->pClassData;
    if (hhid->IsReportAvailable == 1U) {
        ((USBD_CUSTOM_HID_ItfTypeDef *)pdev->pUserData)->OutEvent(hhid->Report_buf[0], hhid->Report_buf[1]);
        hhid->IsReportAvailable = 0U;
    }
    return USBD_OK;
}

static uint8_t *USBD_CUSTOM_HID_GetDeviceQualifierDesc(uint16_t *length) {
    *length = sizeof(USBD_CUSTOM_HID_DeviceQualifierDesc);
    return USBD_CUSTOM_HID_DeviceQualifierDesc;
}

uint8_t USBD_CUSTOM_HID_RegisterInterface(USBD_HandleTypeDef *pdev, USBD_CUSTOM_HID_ItfTypeDef *fops) {
    if (fops != NULL) {
        pdev->pUserData = fops;
        return USBD_OK;
    }
    return USBD_FAIL;
}
