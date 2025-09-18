#include <drivers/behavior.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zmk/behavior.h>
#include <zmk/events/sensor_event.h>
#include <zmk/hid.h>
#include <zmk/keymap.h> // ← これが必要

#ifndef MOD_LCTRL
#define MOD_LCTRL (1 << 0) // ZMKでは通常このビットが左Ctrlに対応
#endif

struct sensor_rev_cfg {
    int direction;
};

static int sensor_rev_if_ctrl_handler(const struct device *dev,
                                      const struct zmk_behavior_binding *binding,
                                      const struct zmk_sensor_event *event,
                                      struct zmk_sensor_binding_data *data) {
    const struct sensor_rev_cfg *cfg = dev->config;
    int direction = cfg->direction;

    bool ctrl_active = zmk_hid_get_explicit_mods() & MOD_LCTRL;
    int final_direction = ctrl_active ? -direction : direction;

    zmk_hid_mouse_scroll_set(0,final_direction);
    return 0;
}

static const struct behavior_driver_api sensor_rev_if_ctrl_driver_api = {
    .sensor_binding_process = sensor_rev_if_ctrl_handler,
};

#define DT_DRV_COMPAT zmk_behavior_sensor_reverse_if_ctrl

#define SENSOR_REV_INST(n)                                             \
    static struct sensor_rev_cfg sensor_rev_cfg_##n = {               \
        .direction = DT_INST_PROP(n, direction),                      \
    };                                                                 \
    DEVICE_DT_INST_DEFINE(n, NULL, NULL, NULL,                        \
                          &sensor_rev_cfg_##n,                        \
                          POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, \
                          &sensor_rev_if_ctrl_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SENSOR_REV_INST)
