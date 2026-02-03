#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>

LOG_MODULE_REGISTER(analog_input, LOG_LEVEL_INF);

static const struct device *input_dev;

struct analog_channel_cfg {
    const struct device *adc_dev;
    uint8_t channel;
    int32_t mid_mv;
    int32_t range_mv;
    int32_t deadzone;
    int32_t mul;
    int32_t div;
    uint16_t input_code;
    bool invert;
};

static struct analog_channel_cfg channels[] = {
    {
        .adc_dev = DEVICE_DT_GET(DT_NODELABEL(adc)),
        .channel = 2,
        .mid_mv = 1630,
        .range_mv = 1600,
        .deadzone = 10,
        .mul = 3,
        .div = 4,
        .input_code = INPUT_REL_X,
        .invert = true,
    },
    {
        .adc_dev = DEVICE_DT_GET(DT_NODELABEL(adc)),
        .channel = 3,
        .mid_mv = 1630,
        .range_mv = 1600,
        .deadzone = 10,
        .mul = 3,
        .div = 4,
        .input_code = INPUT_REL_Y,
        .invert = true,
    },
};

static int read_mv(const struct device *adc_dev, uint8_t channel)
{
    struct adc_channel_cfg cfg = {
        .gain = ADC_GAIN_1,
        .reference = ADC_REF_INTERNAL,
        .acquisition_time = ADC_ACQ_TIME_DEFAULT,
        .channel_id = channel,
    };

    adc_channel_setup(adc_dev, &cfg);

    int16_t raw;
    struct adc_sequence seq = {
        .channels = BIT(channel),
        .buffer = &raw,
        .buffer_size = sizeof(raw),
        .resolution = 12,
    };

    if (adc_read(adc_dev, &seq) < 0) {
        return 0;
    }

    return raw * 3600 / 4096;
}

static void process_channel(struct analog_channel_cfg *cfg)
{
    int mv = read_mv(cfg->adc_dev, cfg->channel);
    int delta = mv - cfg->mid_mv;

    if (cfg->invert) {
        delta = -delta;
    }

    if (abs(delta) < cfg->deadzone) {
        return;
    }

    delta = (delta * cfg->mul) / cfg->div;

    input_report_rel(input_dev, cfg->input_code, delta, true, K_NO_WAIT);
}

static void analog_thread(void)
{
    LOG_INF("Analog input thread started");

    while (1) {
        for (int i = 0; i < ARRAY_SIZE(channels); i++) {
            process_channel(&channels[i]);
        }
        k_msleep(10);
    }
}

static int analog_init(void)
{
    input_dev = input_device_get_binding("ANALOG_IN");

    if (!input_dev) {
        LOG_ERR("Failed to get input device");
        return -ENODEV;
    }

    LOG_INF("Analog input device registered");
    return 0;
}

SYS_INIT(analog_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

K_THREAD_DEFINE(analog_input_thread, 1024, analog_thread, NULL, NULL, NULL, 5, 0, 0);
