#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>

#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/modifiers_state_changed.h>
#include <zmk/events/hid_indicators_changed.h>

#define LED1 11
#define LED2 23

static const struct device *strip = DEVICE_DT_GET(DT_NODELABEL(ws2812));

static struct led_rgb layer2_color = {0, 0, 255};     // 青
static struct led_rgb layer3_color = {0, 255, 0};     // 緑
static struct led_rgb ctrl_color   = {0, 255, 255};   // シアン
static struct led_rgb caps_color   = {255, 0, 0};     // 赤
static struct led_rgb off_color    = {0, 0, 0};

static uint8_t current_layer = 0;
static bool ctrl_active = false;
static bool caps_active = false;

static void update_led1() {
    struct led_rgb color;

    if (ctrl_active) {
        color = ctrl_color;
    } else if (current_layer == 2) {
        color = layer2_color;
    } else if (current_layer == 3) {
        color = layer3_color;
    }

    led_strip_set_pixel(strip, LED1, &color);
    led_strip_update(strip);
}

static void update_led2() {
    if (caps_active) {
        led_strip_set_pixel(strip, LED2, &caps_color);
    } else {
        led_strip_set_pixel(strip, LED2, &off_color);
    }
    led_strip_update(strip);
}

ZMK_LISTENER(custom_leds, custom_leds_listener);
ZMK_SUBSCRIPTION(custom_leds, zmk_layer_state_changed);
ZMK_SUBSCRIPTION(custom_leds, zmk_modifiers_state_changed);
ZMK_SUBSCRIPTION(custom_leds, zmk_hid_indicators_changed);

int custom_leds_listener(const zmk_event_t *eh) {

    // レイヤー変更
    if (as_zmk_layer_state_changed(eh)) {
        const struct zmk_layer_state_changed *ev = as_zmk_layer_state_changed(eh);
        current_layer = ev->state;
        update_led1();
    }

    // Ctrl 押下
    if (as_zmk_modifiers_state_changed(eh)) {
        const struct zmk_modifiers_state_changed *ev = as_zmk_modifiers_state_changed(eh);
        ctrl_active = (ev->state & MOD_LCTL) || (ev->state & MOD_RCTL);
        update_led1();
    }

    // CapsLock
    if (as_zmk_hid_indicators_changed(eh)) {
        const struct zmk_hid_indicators_changed *ev = as_zmk_hid_indicators_changed(eh);
        caps_active = (ev->indicators & HID_USAGE_LED_CAPS_LOCK);
        update_led2();
    }

    return 0;
}
