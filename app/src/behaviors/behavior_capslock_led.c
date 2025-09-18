#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zmk/event_manager.h>
#include <zmk/events/hid_indicators_changed.h>

#define CAPS_LED_NODE DT_ALIAS(led_red)

static const struct gpio_dt_spec caps_led = GPIO_DT_SPEC_GET(CAPS_LED_NODE, gpios);

static void capslock_handler(const struct zmk_hid_indicators_changed *ev) {
    gpio_pin_set_dt(&caps_led, ev->indicators.caps_lock ? 0 : 1);
}

ZMK_EVENT_SUBSCRIBE(zmk_hid_indicators_changed, capslock_handler);
