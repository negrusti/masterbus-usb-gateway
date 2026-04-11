#include "can_bridge.h"

#include <string.h>

#include "board_leds.h"
#include "stm32f0xx_hal.h"
#include "stm32f0xx_hal_can.h"

#define MB_CAN_RX_PIN GPIO_PIN_8
#define MB_CAN_TX_PIN GPIO_PIN_9
#define MB_CAN_GPIO_PORT GPIOB
#define MB_CAN_PHY_CTRL2_PIN GPIO_PIN_13
#define MB_CAN_PHY_CTRL_GPIOC_PORT GPIOC

#define MB_CAN_RX_QUEUE_SIZE 64U
#define MB_CAN_COMBIMASTER_RESP_ID 0x061A026CU
#define MB_CAN_LEGACY_NAME_STRING_ID_HI 0xFCU
#define MB_CAN_LEGACY_NAME_STRING_ID_LO 0x00U
#define MB_CAN_LIVE_NAME_STRING_ID_HI 0x00U
#define MB_CAN_LIVE_NAME_STRING_ID_LO 0x01U
#define MB_CAN_HID_RESERVED_TAIL1 0x12U
#define MB_CAN_HID_RESERVED_TAIL2 0x80U
static CAN_HandleTypeDef g_hcan;
static gw_can_frame_t g_rx_queue[MB_CAN_RX_QUEUE_SIZE];
static uint8_t g_rx_head;
static uint8_t g_rx_tail;
static uint8_t g_rx_count;
static uint16_t g_can_rx_meta_seq;

static void mb_can_put_hid_slot(gw_hid_report_view_t *out, uint8_t slot, const gw_can_frame_t *frame) {
    if ((out == NULL) || (frame == NULL) || (slot >= GW_HID_MAX_PACKETS)) {
        return;
    }

    gw_can_to_packet14(frame, &out->slots[slot], &out->meta_low[slot]);
    out->slots[slot].bytes[1] |= 0x08U;
}

static void mb_can_fill_context_slots(gw_hid_report_view_t *out) {
    /* These companion frames were present in the original gateway's name
       response window. MasterAdjust advances only after seeing the same
       sideband/context shape, even when the requested name string is complete. */
    static const gw_can_frame_t context_frames[] = {
        {
            .ext_id = 0x021A026CU,
            .dlc = 6U,
            .data = {0x0FU, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U},
            .meta16 = 0x014CU,
        },
        {
            .ext_id = 0x0B9A026CU,
            .dlc = 6U,
            .data = {0x11U, 0x00U, 0x01U, 0x00U, 0x54U, 0x41U, 0x00U, 0x00U},
            .meta16 = 0x007AU,
        },
        {
            .ext_id = 0x061A026CU,
            .dlc = 4U,
            .data = {0x08U, 0x02U, 0x04U, 0x00U, 0x06U, 0x00U, 0x00U, 0x00U},
            .meta16 = 0x0069U,
        },
    };
    uint8_t slot;
    uint8_t i;

    if ((out == NULL) || (out->count == 0U)) {
        return;
    }

    slot = out->count;
    for (i = 0U; (i < (uint8_t)(sizeof(context_frames) / sizeof(context_frames[0]))) && (slot < GW_HID_MAX_PACKETS); ++i) {
        mb_can_put_hid_slot(out, slot, &context_frames[i]);
        ++slot;
    }
}

static void mb_can_set_original_tail(gw_hid_report_view_t *out) {
    if (out == NULL) {
        return;
    }

    out->reserved_tail[0] = 0U;
    out->reserved_tail[1] = MB_CAN_HID_RESERVED_TAIL1;
    out->reserved_tail[2] = MB_CAN_HID_RESERVED_TAIL2;
}

static uint8_t mb_can_is_name_id(const uint8_t *buf) {
    return (((buf[0] == MB_CAN_LEGACY_NAME_STRING_ID_HI) && (buf[1] == MB_CAN_LEGACY_NAME_STRING_ID_LO)) ||
            ((buf[0] == MB_CAN_LIVE_NAME_STRING_ID_HI) && (buf[1] == MB_CAN_LIVE_NAME_STRING_ID_LO))) ? 1U : 0U;
}

static void mb_can_apply_original_meta(gw_can_frame_t *frame) {
    if ((frame == NULL) || (frame->ext_id != MB_CAN_COMBIMASTER_RESP_ID) || (frame->dlc < 4U)) {
        return;
    }

    if ((frame->data[0] == 0x09U) && (frame->data[1] == 0x03U) &&
        (mb_can_is_name_id(&frame->data[2]) != 0U)) {
        /* Sideband value captured from the original gateway for the string-id
           discovery response. This is transport metadata, not CAN data. */
        frame->meta16 = 0x00C7U;
        return;
    }

    if ((frame->data[0] == 0x30U) &&
        (mb_can_is_name_id(&frame->data[1]) != 0U)) {
        /* The name chunks need the same low metadata sequence as the original
           gateway; without it MasterAdjust receives the text but does not
           proceed to the richer post-name query phase. */
        switch (frame->data[3]) {
            case 0x00U:
                frame->meta16 = 0x00ECU;
                break;
            case 0x01U:
                frame->meta16 = 0x00CFU;
                break;
            case 0x02U:
                frame->meta16 = 0x00D3U;
                break;
            case 0x03U:
                frame->meta16 = 0x00E5U;
                break;
            default:
                break;
        }
    }
}

static void mb_can_gpio_init(void) {
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* The visible red/blue LEDs are on PA0/PA1 on this CANable clone. Do not
       drive them here as PHY controls; forcing them low leaves both LEDs on. */
    HAL_GPIO_WritePin(MB_CAN_PHY_CTRL_GPIOC_PORT, MB_CAN_PHY_CTRL2_PIN, GPIO_PIN_RESET);
    gpio.Pin = MB_CAN_PHY_CTRL2_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(MB_CAN_PHY_CTRL_GPIOC_PORT, &gpio);

    gpio.Pin = MB_CAN_RX_PIN | MB_CAN_TX_PIN;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio.Alternate = GPIO_AF4_CAN;
    HAL_GPIO_Init(MB_CAN_GPIO_PORT, &gpio);
}

void HAL_CAN_MspInit(CAN_HandleTypeDef *hcan) {
    if (hcan->Instance == CAN) {
        __HAL_RCC_CAN1_CLK_ENABLE();
        mb_can_gpio_init();
    }
}

static void mb_can_queue_push(const gw_can_frame_t *frame) {
    if (g_rx_count >= MB_CAN_RX_QUEUE_SIZE) {
        return;
    }

    g_rx_queue[g_rx_head] = *frame;
    g_rx_head = (uint8_t)((g_rx_head + 1U) % MB_CAN_RX_QUEUE_SIZE);
    ++g_rx_count;
}

static bool mb_can_queue_pop(gw_can_frame_t *frame) {
    if (g_rx_count == 0U) {
        return false;
    }

    *frame = g_rx_queue[g_rx_tail];
    g_rx_tail = (uint8_t)((g_rx_tail + 1U) % MB_CAN_RX_QUEUE_SIZE);
    --g_rx_count;
    return true;
}

void can_bridge_init(void) {
    CAN_FilterTypeDef filter = {0};

    g_can_rx_meta_seq = 1U;

    memset(&g_hcan, 0, sizeof(g_hcan));
    g_hcan.Instance = CAN;
    g_hcan.Init.Prescaler = 12U;
    g_hcan.Init.Mode = CAN_MODE_NORMAL;
    g_hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
    g_hcan.Init.TimeSeg1 = CAN_BS1_13TQ;
    g_hcan.Init.TimeSeg2 = CAN_BS2_2TQ;
    g_hcan.Init.TimeTriggeredMode = DISABLE;
    g_hcan.Init.AutoBusOff = DISABLE;
    g_hcan.Init.AutoWakeUp = DISABLE;
    g_hcan.Init.AutoRetransmission = ENABLE;
    g_hcan.Init.ReceiveFifoLocked = DISABLE;
    g_hcan.Init.TransmitFifoPriority = DISABLE;

    g_rx_head = 0U;
    g_rx_tail = 0U;
    g_rx_count = 0U;

    if (HAL_CAN_Init(&g_hcan) != HAL_OK) {
        return;
    }

    filter.FilterBank = 0U;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    filter.FilterIdHigh = 0U;
    filter.FilterIdLow = 0U;
    filter.FilterMaskIdHigh = 0U;
    filter.FilterMaskIdLow = 0U;
    filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    filter.FilterActivation = ENABLE;
    if (HAL_CAN_ConfigFilter(&g_hcan, &filter) != HAL_OK) {
        return;
    }

    if (HAL_CAN_Start(&g_hcan) != HAL_OK) {
        return;
    }
}

bool can_bridge_send_frame(const gw_can_frame_t *frame) {
    CAN_TxHeaderTypeDef header = {0};
    uint32_t mailbox;

    if (frame == NULL) {
        return false;
    }

    header.IDE = CAN_ID_EXT;
    header.ExtId = frame->ext_id & 0x1FFFFFFFU;
    header.RTR = CAN_RTR_DATA;
    header.DLC = frame->dlc & 0x0FU;
    header.TransmitGlobalTime = DISABLE;

    if (HAL_CAN_AddTxMessage(&g_hcan, &header, (uint8_t *)frame->data, &mailbox) == HAL_OK) {
        return true;
    }

    return false;
}

void can_bridge_poll(void) {
    while (HAL_CAN_GetRxFifoFillLevel(&g_hcan, CAN_RX_FIFO0) > 0U) {
        CAN_RxHeaderTypeDef header = {0};
        gw_can_frame_t frame;

        memset(&frame, 0, sizeof(frame));
        if (HAL_CAN_GetRxMessage(&g_hcan, CAN_RX_FIFO0, &header, frame.data) != HAL_OK) {
            break;
        }
        if ((header.IDE != CAN_ID_EXT) || (header.RTR != CAN_RTR_DATA)) {
            continue;
        }

        frame.ext_id = header.ExtId & 0x1FFFFFFFU;
        frame.dlc = header.DLC & 0x0FU;
        frame.meta16 = (uint16_t)(g_can_rx_meta_seq++ & 0x00FFU);
        mb_can_apply_original_meta(&frame);
        mb_can_queue_push(&frame);
        board_leds_pulse_rx();
    }
}

bool can_bridge_pop_hid_report(gw_hid_report_view_t *out) {
    uint8_t i;

    if (out == NULL) {
        return false;
    }

    memset(out, 0, sizeof(*out));
    mb_can_set_original_tail(out);
    for (i = 0; i < GW_HID_MAX_PACKETS; ++i) {
        gw_can_frame_t frame;

        if (!mb_can_queue_pop(&frame)) {
            break;
        }
        mb_can_put_hid_slot(out, i, &frame);
        ++out->count;
    }

    mb_can_fill_context_slots(out);

    return out->count != 0U;
}

uint8_t can_bridge_pending_count(void) {
    return g_rx_count;
}
