#define DT_DRV_COMPAT zmk_behavior_sensor_reverse_if_ctrl

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zmk/event_manager.h>
#include <zmk/events/sensor_event.h>
#include <zmk/keymap.h>
#include <zmk/hid.h>
#include <zmk/behavior.h>
#include <zmk/endpoints.h>
#include <zmk/keys.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static int behavior_sensor_reverse_if_ctrl_binding_pressed(struct zmk_behavior_binding *binding,
                                                           struct zmk_behavior_binding_event event) {
    int direction = event.param1; // CW = 1, CCW = -1（センサー定義に依存）

    bool ctrl_active = zmk_keymap_is_active(ZMK_HID_USAGE_KEY_LEFT_CONTROL);

    int effective_direction = ctrl_active ? -direction : direction;

    uint16_t usage = effective_direction > 0
                         ? HID_USAGE_CONSUMER_VOLUME_INCREMENT
                         : HID_USAGE_CONSUMER_VOLUME_DECREMENT;

    LOG_DBG("Sensor direction: %d, Ctrl active: %d, Effective: %d, Usage: 0x%04X",
            direction, ctrl_active, effective_direction, usage);

    zmk_hid_consumer_press(usage);
    return ZMK_EV_EVENT_HANDLED;
}

static int behavior_sensor_reverse_if_ctrl_binding_released(struct zmk_behavior_binding *binding,
                                                            struct zmk_behavior_binding_event event) {
    int direction = event.param1;

    bool ctrl_active = zmk_keymap_is_active(ZMK_HID_USAGE_KEY_LEFT_CONTROL);

    int effective_direction = ctrl_active ? -direction : direction;

    uint16_t usage = effective_direction > 0
                         ? HID_USAGE_CONSUMER_VOLUME_INCREMENT
                         : HID_USAGE_CONSUMER_VOLUME_DECREMENT;

    zmk_hid_consumer_release(usage);
    return ZMK_EV_EVENT_HANDLED;
}

static const struct behavior_driver_api behavior_sensor_reverse_if_ctrl_driver = {
    .binding_pressed = behavior_sensor_reverse_if_ctrl_binding_pressed,
    .binding_released = behavior_sensor_reverse_if_ctrl_binding_released,
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL,
                      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                      &behavior_sensor_reverse_if_ctrl_driver);
