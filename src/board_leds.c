#include "board_leds.h"

#include "stm32f0xx_hal.h"

#define MB_LED_RX_PIN GPIO_PIN_0
#define MB_LED_TX_PIN GPIO_PIN_1
#define MB_LED_PORT GPIOB

#define MB_LED_PULSE_MS 300U

typedef struct {
    uint8_t connected;
    uint32_t rx_until_ms;
    uint32_t tx_until_ms;
} mb_led_state_t;

static mb_led_state_t g_led_state;

static uint8_t mb_led_is_active(uint32_t now_ms, uint32_t until_ms) {
    return until_ms != 0U && (int32_t)(until_ms - now_ms) > 0;
}

static void mb_led_apply(uint32_t now_ms) {
    GPIO_PinState rx_state;
    GPIO_PinState tx_state;

    (void)g_led_state.connected;
    rx_state = mb_led_is_active(now_ms, g_led_state.rx_until_ms) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    tx_state = mb_led_is_active(now_ms, g_led_state.tx_until_ms) ? GPIO_PIN_SET : GPIO_PIN_RESET;

    HAL_GPIO_WritePin(MB_LED_PORT, MB_LED_RX_PIN, rx_state);
    HAL_GPIO_WritePin(MB_LED_PORT, MB_LED_TX_PIN, (tx_state == GPIO_PIN_SET) ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

void board_leds_init(void) {
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    gpio.Pin = MB_LED_RX_PIN | MB_LED_TX_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(MB_LED_PORT, &gpio);

    g_led_state.connected = 0U;
    g_led_state.rx_until_ms = 0U;
    g_led_state.tx_until_ms = 0U;
    mb_led_apply(0U);
}

void board_leds_poll(uint32_t now_ms) {
    mb_led_apply(now_ms);
}

void board_leds_set_connected(uint8_t connected) {
    g_led_state.connected = (connected != 0U) ? 1U : 0U;
}

void board_leds_pulse_rx(void) {
    g_led_state.rx_until_ms = HAL_GetTick() + MB_LED_PULSE_MS;
}

void board_leds_pulse_tx(void) {
    g_led_state.tx_until_ms = HAL_GetTick() + MB_LED_PULSE_MS;
}
