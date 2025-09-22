// SPDX-License-Identifier: MIT
#define DT_DRV_COMPAT zmk_behavior_toggle_action

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zmk/behavior.h>
#include <zmk/event_manager.h>
#include <zmk/keymap.h>
#include <zmk/behavior_binding.h>
#include <zephyr/logging/log.h>

//LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct toggle_action_data {
    bool toggled;
};

static int behavior_toggle_action_binding_pressed(struct zmk_behavior_binding *binding,
                                                  struct zmk_behavior_binding_event event) {
    const struct device *dev = behavior_get_binding_behavior(binding->behavior_dev);
    struct toggle_action_data *data = dev->data;

    data->toggled = !data->toggled;

    struct zmk_behavior_binding_param *param =
        data->toggled ? &binding->param1 : &binding->param0;

    const struct device *target_behavior = behavior_get_binding_behavior(param->behavior_dev);
    if (!device_is_ready(target_behavior)) {
//        LOG_ERR("Target behavior device not ready");
        return -ENODEV;
    }

    return behavior_keymap_binding_pressed(target_behavior, event);
}

static int behavior_toggle_action_binding_released(struct zmk_behavior_binding *binding,
                                                   struct zmk_behavior_binding_event event) {
    // No release action needed
    return 0;
}

static const struct behavior_driver_api behavior_toggle_action_driver_api = {
    .binding_pressed = behavior_toggle_action_binding_pressed,
    .binding_released = behavior_toggle_action_binding_released,
};

#define TOGGLE_ACTION_INST(n)                                                                    \
    static struct toggle_action_data toggle_action_data_##n = {                                  \
        .toggled = false,                                                                         \
    };                                                                                            \
    DEVICE_DT_INST_DEFINE(n, NULL, NULL, &toggle_action_data_##n, NULL, POST_KERNEL,             \
                          CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &behavior_toggle_action_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TOGGLE_ACTION_INST)
