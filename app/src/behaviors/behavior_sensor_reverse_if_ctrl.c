#define DT_DRV_COMPAT zmk_behavior_sensor_reverse_if_ctrl

#include <zephyr/device.h>
#include <drivers/behavior.h>
#include <zephyr/drivers/sensor.h>
#include <zmk/behavior.h>
#include <zmk/events/sensor_event.h>
#include <zmk/hid.h>

#ifndef MOD_LCTRL
#define MOD_LCTRL (1 << 0)
#endif

struct behavior_sensor_rev_if_ctrl_config {
    int scale;
};

static int sensor_rev_if_ctrl_process(struct zmk_behavior_binding *binding,
                                      struct zmk_behavior_binding_event event,
                                      enum behavior_sensor_binding_process_mode mode) {
    const struct zmk_sensor_event *sensor_event = event.sensor_event;
    const struct device *dev = device_get_binding(binding->behavior_dev);
    const struct behavior_sensor_rev_if_ctrl_config *cfg = dev->config;

    const struct zmk_hid_keyboard_report *report = zmk_hid_get_keyboard_report();
    bool ctrl_active = (report->body.modifiers & MOD_LCTRL) != 0;

    struct sensor_value val = sensor_event->channel_data[1].value;
    double accel_y = sensor_value_to_double(val);
    int scroll_amount = (int)(accel_y * cfg->scale);
    int final_direction = ctrl_active ? -scroll_amount : scroll_amount;

    if (final_direction != 0) {
        zmk_hid_mouse_scroll_set(0, final_direction);
    }

    return 0;
}

static const struct behavior_driver_api behavior_sensor_rev_if_ctrl_driver_api = {
    .sensor_binding_process = sensor_rev_if_ctrl_process,
};

#define SENSOR_REV_INST(n) \
    static struct behavior_sensor_rev_if_ctrl_config behavior_sensor_rev_if_ctrl_config_##n = { \
        .scale = DT_INST_PROP_OR(n, scale, 10), \
    }; \
    BEHAVIOR_DT_INST_DEFINE( \
        n, NULL, NULL, NULL, &behavior_sensor_rev_if_ctrl_config_##n, \
        POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, \
        &behavior_sensor_rev_if_ctrl_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SENSOR_REV_INST)
