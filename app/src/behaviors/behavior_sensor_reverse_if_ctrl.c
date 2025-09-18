#include <drivers/behavior.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zmk/events/sensor_event.h>
#include <zmk/hid.h>

LOG_MODULE_REGISTER(sensor_rev_if_ctrl, LOG_LEVEL_DBG);

#ifndef MOD_LCTRL
#define MOD_LCTRL (1 << 0)
#endif

static int sensor_rev_if_ctrl_handler(const struct device *dev,
                                      const struct zmk_behavior_binding *binding,
                                      const struct zmk_sensor_event *event,
                                      struct zmk_sensor_binding_data *data) {
    // HIDレポートから修飾キー状態を取得
    const struct zmk_hid_keyboard_report *report = zmk_hid_get_keyboard_report();
    bool ctrl_active = (report->modifiers & MOD_LCTRL) != 0;

    // センサーのY軸加速度（仮定）からスクロール量を算出
    int16_t accel_y = event->channel_data[1].value;
    int scroll_amount = accel_y / 100;

    // Ctrlが押されていればスクロール方向を反転
    int final_direction = ctrl_active ? -scroll_amount : scroll_amount;

    // スクロール送信（Y方向）
    if (final_direction != 0) {
        zmk_hid_mouse_scroll_set(0, final_direction);
        LOG_DBG("Scroll sent: %d (ctrl=%d)", final_direction, ctrl_active);
    } else {
        LOG_DBG("No scroll: accel_y=%d", accel_y);
    }

    return 0;
}

static const struct behavior_driver_api sensor_rev_if_ctrl_driver_api = {
    .sensor_binding_process = sensor_rev_if_ctrl_handler,
};

#define DT_DRV_COMPAT zmk_behavior_sensor_reverse_if_ctrl

#define SENSOR_REV_INST(n)                                             \
    DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL, NULL,                   \
                          POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, \
                          &sensor_rev_if_ctrl_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SENSOR_REV_INST)
