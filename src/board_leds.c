#include "board_leds.h"

#include "stm32f0xx_hal.h"

#define MB_LED_RX_PORT GPIOA
#define MB_LED_RX_PIN GPIO_PIN_0
#define MB_LED_TX_PORT GPIOA
#define MB_LED_TX_PIN GPIO_PIN_1

#define MB_LED_PULSE_MS 60U
#define MB_LED_OFF_GAP_MS 60U
#define MB_LED_RX_ON GPIO_PIN_RESET
#define MB_LED_TX_ON GPIO_PIN_RESET

typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
    GPIO_PinState on_state;
    uint8_t blink_requested;
    uint32_t on_until_ms;
    uint32_t blocked_until_ms;
} mb_led_t;

typedef struct {
    mb_led_t rx;
    mb_led_t tx;
} mb_led_state_t;

static mb_led_state_t g_led_state;

static GPIO_PinState mb_led_off_state(GPIO_PinState on_state) {
    return (on_state == GPIO_PIN_SET) ? GPIO_PIN_RESET : GPIO_PIN_SET;
}

static uint8_t mb_led_time_passed(uint32_t now_ms, uint32_t target_ms) {
    return (int32_t)(now_ms - target_ms) >= 0;
}

static void mb_led_write(const mb_led_t *led, uint8_t active) {
    HAL_GPIO_WritePin(led->port, led->pin, (active != 0U) ? led->on_state : mb_led_off_state(led->on_state));
}

static void mb_led_update_one(mb_led_t *led, uint32_t now_ms) {
    if (led->blink_requested != 0U) {
        led->blink_requested = 0U;
        if (mb_led_time_passed(now_ms, led->blocked_until_ms) != 0U) {
            led->on_until_ms = now_ms + MB_LED_PULSE_MS;
            led->blocked_until_ms = led->on_until_ms + MB_LED_OFF_GAP_MS;
        }
    }

    mb_led_write(led, (mb_led_time_passed(now_ms, led->on_until_ms) == 0U) ? 1U : 0U);
}

void board_leds_init(void) {
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();

    gpio.Pin = MB_LED_RX_PIN | MB_LED_TX_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &gpio);

    /* AliExpress CANable clone: visible red/blue activity LEDs are PA0/PA1,
       both active-low. These are not the upstream CANable PB0/PB1 LEDs. */
    g_led_state.rx.port = MB_LED_RX_PORT;
    g_led_state.rx.pin = MB_LED_RX_PIN;
    g_led_state.rx.on_state = MB_LED_RX_ON;
    g_led_state.rx.blink_requested = 0U;
    g_led_state.rx.on_until_ms = 0U;
    g_led_state.rx.blocked_until_ms = 0U;
    g_led_state.tx.port = MB_LED_TX_PORT;
    g_led_state.tx.pin = MB_LED_TX_PIN;
    g_led_state.tx.on_state = MB_LED_TX_ON;
    g_led_state.tx.blink_requested = 0U;
    g_led_state.tx.on_until_ms = 0U;
    g_led_state.tx.blocked_until_ms = 0U;

    mb_led_write(&g_led_state.rx, 0U);
    mb_led_write(&g_led_state.tx, 0U);
}

void board_leds_poll(uint32_t now_ms) {
    mb_led_update_one(&g_led_state.rx, now_ms);
    mb_led_update_one(&g_led_state.tx, now_ms);
}

void board_leds_set_connected(uint8_t connected) {
    (void)connected;
}

void board_leds_pulse_rx(void) {
    g_led_state.rx.blink_requested = 1U;
}

void board_leds_pulse_tx(void) {
    g_led_state.tx.blink_requested = 1U;
}
