#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/led_strip.h>

#include <zmk/event_manager.h>
#include <zmk/events/layer_state_changed.h>
#include <zmk/events/modifiers_state_changed.h>
#include <zmk/events/hid_indicators_changed.h>

#define LED1 11
#define LED2 23
#define LED_COUNT 47   // WS2812 の総数に合わせる

static const struct device *strip = DEVICE_DT_GET(DT_NODELABEL(led_strip));

static struct led_rgb layer2_color = {0, 0, 255};     // 青
static struct led_rgb layer3_color = {0, 255, 0};     // 緑
static struct led_rgb ctrl_color   = {0, 255, 255};   // シアン
static struct led_rgb caps_color   = {255, 0, 0};     // 赤
static struct led_rgb off_color    = {0, 0, 0};

static uint8_t current_layer = 0;
static bool ctrl_active = false;
static bool caps_active = false;

static void update_leds() {
    struct led_rgb buf[LED_COUNT];

    // 全 LED を消灯
    for (int i = 0; i < LED_COUNT; i++) {
        buf[i] = off_color;
    }

    // LED1 の色
    if (ctrl_active) {
        buf[LED1] = ctrl_color;
    } else if (current_layer == 2) {
        buf[LED1] = layer2_color;
    } else if (current_layer == 3) {
        buf[LED1] = layer3_color;
    }

    // LED2 の色（CapsLock）
    if (caps_active) {
        buf[LED2] = caps_color;
    }

    // 一括更新
    led_strip_update_rgb(strip, buf, LED_COUNT);
}

int custom_leds_listener(const zmk_event_t *eh) {

    // レイヤー変更
    if (as_zmk_layer_state_changed(eh)) {
        const struct zmk_layer_state_changed *ev = as_zmk_layer_state_changed(eh);
        current_layer = ev->state;
        update_leds();
    }

    // Ctrl 押下
    if (as_zmk_modifiers_state_changed(eh)) {
        const struct zmk_modifiers_state_changed *ev = as_zmk_modifiers_state_changed(eh);
        ctrl_active = (ev->state & MOD_LCTL) || (ev->state & MOD_RCTL);
        update_leds();
    }

    // CapsLock
    if (as_zmk_hid_indicators_changed(eh)) {
        const struct zmk_hid_indicators_changed *ev = as_zmk_hid_indicators_changed(eh);
        caps_active = (ev->indicators & HID_USAGE_LED_CAPS_LOCK);
        update_leds();
    }

    return 0;
}

ZMK_LISTENER(custom_leds, custom_leds_listener);
ZMK_SUBSCRIPTION(custom_leds, zmk_layer_state_changed);
ZMK_SUBSCRIPTION(custom_leds, zmk_modifiers_state_changed);
ZMK_SUBSCRIPTION(custom_leds, zmk_hid_indicators_changed);
