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

struct behavior_sensor_reverse_if_ctrl_config {
    int direction;
};

static int behavior_sensor_reverse_if_ctrl_binding_pressed(struct zmk_behavior_binding *binding,
                                                           struct zmk_behavior_binding_event event) {
    const struct device *dev = device_get_binding(binding->behavior_dev);
    const struct behavior_sensor_reverse_if_ctrl_config *cfg = dev->config;

    int sensor_direction = event.param1;
    bool ctrl_active = zmk_keymap_is_active(ZMK_HID_USAGE_KEY_LEFT_CONTROL);

    int effective_direction = ctrl_active ? -cfg->direction : cfg->direction;
    int final_direction = effective_direction * sensor_direction;

    LOG_DBG("Sensor: %d, Config: %d, Ctrl: %d → Final: %d",
            sensor_direction, cfg->direction, ctrl_active, final_direction);

    // 垂直スクロール（Y方向）
    zmk_hid_mouse_scroll(final_direction, 0);
    return ZMK_EV_EVENT_HANDLED;
}

static int behavior_sensor_reverse_if_ctrl_binding_released(struct zmk_behavior_binding *binding,
                                                            struct zmk_behavior_binding_event event) {
    // スクロールは press/release で分離しないため、release は何もしない
    return ZMK_EV_EVENT_HANDLED;
}

static const struct behavior_driver_api behavior_sensor_reverse_if_ctrl_driver = {
    .binding_pressed = behavior_sensor_reverse_if_ctrl_binding_pressed,
    .binding_released = behavior_sensor_reverse_if_ctrl_binding_released,
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL,
                      &(struct behavior_sensor_reverse_if_ctrl_config){
                          .direction = DT_INST_PROP(0, direction),
                      },
                      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
                      &behavior_sensor_reverse_if_ctrl_driver);
