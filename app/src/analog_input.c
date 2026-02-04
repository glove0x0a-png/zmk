#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/logging/log.h>
#include <zmk/sensors.h>

LOG_MODULE_REGISTER(analog_input, LOG_LEVEL_INF);

struct analog_input_config {
    uint8_t channel_id;
};

struct analog_input_data {
    const struct device *adc_dev;
    struct adc_channel_cfg channel_cfg;
    int16_t sample_buffer;
};

static int analog_input_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
    struct analog_input_data *data = dev->data;

    struct adc_sequence sequence = {
        .channels = BIT(data->channel_cfg.channel_id),
        .buffer = &data->sample_buffer,
        .buffer_size = sizeof(data->sample_buffer),
        .resolution = 12,
    };

    int ret = adc_read(data->adc_dev, &sequence);
    if (ret < 0) {
        LOG_ERR("ADC read failed (%d)", ret);
        return ret;
    }

    return 0;
}

static int analog_input_channel_get(const struct device *dev,
                                    enum sensor_channel chan,
                                    struct sensor_value *val)
{
    struct analog_input_data *data = dev->data;

    val->val1 = data->sample_buffer;  // raw ADC value
    val->val2 = 0;

    return 0;
}

static int analog_input_init(const struct device *dev)
{
    struct analog_input_data *data = dev->data;
    const struct analog_input_config *cfg = dev->config;

    data->adc_dev = DEVICE_DT_GET(DT_PARENT(DT_DRV_INST(0)));
    if (!device_is_ready(data->adc_dev)) {
        LOG_ERR("ADC device not ready");
        return -ENODEV;
    }

    data->channel_cfg.channel_id = cfg->channel_id;
    data->channel_cfg.differential = 0;
    data->channel_cfg.gain = ADC_GAIN_1;
    data->channel_cfg.reference = ADC_REF_INTERNAL;
    data->channel_cfg.acquisition_time = ADC_ACQ_TIME_DEFAULT;

    int ret = adc_channel_setup(data->adc_dev, &data->channel_cfg);
    if (ret < 0) {
        LOG_ERR("ADC channel setup failed (%d)", ret);
        return ret;
    }

    LOG_INF("Analog input sensor initialized on channel %d", cfg->channel_id);
    return 0;
}

static const struct sensor_driver_api analog_input_api = {
    .sample_fetch = analog_input_sample_fetch,
    .channel_get = analog_input_channel_get,
};

#define ANALOG_INPUT_DEFINE(inst)                                              \
    static struct analog_input_data analog_input_data_##inst;                  \
                                                                               \
    static const struct analog_input_config analog_input_config_##inst = {     \
        .channel_id = DT_INST_IO_CHANNELS_CELL(inst, channel),                 \
    };                                                                         \
                                                                               \
    DEVICE_DT_INST_DEFINE(inst, analog_input_init, NULL,                       \
                          &analog_input_data_##inst,                           \
                          &analog_input_config_##inst,                         \
                          POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,            \
                          &analog_input_api);

DT_INST_FOREACH_STATUS_OKAY(ANALOG_INPUT_DEFINE)
