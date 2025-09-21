#define DT_DRV_COMPAT zmk_az1uball


#include <zephyr/device.h>
//debug
#include <zephyr/drivers/gpio.h>
//debug
#include <zmk/hid.h>
#include <dt-bindings/zmk/keys.h>
#include <zephyr/input/input.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <math.h>
#include <stdlib.h>
#include "az1uball.h"


volatile uint8_t AZ1UBALL_MOUSE_MAX_SPEED = 25;
volatile uint8_t AZ1UBALL_MOUSE_MAX_TIME = 5;
volatile float AZ1UBALL_MOUSE_SMOOTHING_FACTOR = 1.3f;
volatile uint8_t AZ1UBALL_SCROLL_MAX_SPEED = 1;
volatile uint8_t AZ1UBALL_SCROLL_MAX_TIME = 1;
volatile float AZ1UBALL_SCROLL_SMOOTHING_FACTOR = 0.5f;

#define NORMAL_POLL_INTERVAL K_MSEC(10)   // 通常時: 10ms (100Hz)
#define LOW_POWER_POLL_INTERVAL K_MSEC(100) // 省電力時: 100ms (10Hz)
#define LOW_POWER_TIMEOUT_MS 5000    // 5秒間入力がないと省電力モードへ

///////
//#define LED_R_NODE DT_ALIAS(led_red)
//#define LED_G_NODE DT_ALIAS(led_green)
//#define LED_B DT_ALIAS(led_blue)
//static const struct gpio_dt_spec my_red = GPIO_DT_SPEC_GET(LED_R_NODE, gpios);
//static const struct gpio_dt_spec my_green = GPIO_DT_SPEC_GET(LED_G_NODE, gpios);
//static const struct gpio_dt_spec my_blue = GPIO_DT_SPEC_GET(LED_B, gpios);
//uint32_t start_time;
//gpio_pin_set_dt(&my_red, 1);start_time=k_uptime_get();while(k_uptime_get()-start_time < 500){};
//gpio_pin_set_dt(&my_red, 0);start_time=k_uptime_get();while(k_uptime_get()-start_time < 500){};
//gpio_pin_set_dt(&my_green, 1);start_time=k_uptime_get();while(k_uptime_get()-start_time < 2000){};
//gpio_pin_set_dt(&my_green, 0);start_time=k_uptime_get();while(k_uptime_get()-start_time < 2000){};
//gpio_pin_set_dt(&my_blue, 1);start_time=k_uptime_get();while(k_uptime_get()-start_time < 500){};
//gpio_pin_set_dt(&my_blue, 0);start_time=k_uptime_get();while(k_uptime_get()-start_time < 500){};
//global
static int previous_x = 0;
static int previous_y = 0;
static enum az1uball_mode current_mode = AZ1UBALL_MODE_MOUSE;//default:mouse

static int az1uball_init(const struct device *dev);					//初期化処理
static float parse_sensitivity(const char *sensitivity);			//プロパティからマウス精度を変更
static void check_power_mode(         struct az1uball_data *data);	//LOW_POWER_TIMEOUT_MSを参照し、省電力モードへ
static void az1uball_process_movement(struct az1uball_data *data,	//マウス動作、az1uball_data構造体を更新
	int delta_x,int delta_y, uint32_t time_between_interrupts,
	int max_speed, int max_time, float smoothing_factor);
void az1uball_read_data_work(struct k_work *work);					//i2c_read_dtあり。I2C通信でデータ取り出し。
static void az1uball_polling(struct k_timer *timer);

///////////////////////////////////////////////////////////////////////////
/* Initialization of AZ1UBALL */
static int az1uball_init(const struct device *dev)
{
    //●●起動確認-OK from 332
    struct az1uball_data *data = dev->data;
    const struct az1uball_config *config = dev->config;
    int ret;
    data->dev = dev;
    data->sw_pressed_prev = false;

    /* Check if the I2C device is ready */
    if (!device_is_ready(config->i2c.bus)) {
        //●●エラーになっていないOK from 332
        return -ENODEV;
    }

    /* Set turbo mode */
    uint8_t cmd = 0x91;
    ret = i2c_write_dt(&config->i2c, &cmd, sizeof(cmd));

    if (ret) {
        return ret;
    }

    k_work_init(&data->work, az1uball_read_data_work);

    data->last_activity_time = k_uptime_get();
    data->is_low_power_mode = false;

    k_timer_init(&data->polling_timer, az1uball_polling, NULL);
    k_timer_start(&data->polling_timer, NORMAL_POLL_INTERVAL, NORMAL_POLL_INTERVAL);

    return 0;
}

static float parse_sensitivity(const char *sensitivity) {
    float value;
    char *endptr;
    
    value = strtof(sensitivity, &endptr);
    if (endptr == sensitivity || (*endptr != 'x' && *endptr != 'X')) {
        return 1.0f; // デフォルト値
    }
    
    return value;
}

static void check_power_mode(struct az1uball_data *data) {
    uint32_t current_time = k_uptime_get();
    uint32_t idle_time = current_time - data->last_activity_time;

    if (!data->is_low_power_mode && idle_time > LOW_POWER_TIMEOUT_MS) {
        // 省電力モードに切り替え
        data->is_low_power_mode = true;
        k_timer_stop(&data->polling_timer);
        k_timer_start(&data->polling_timer, LOW_POWER_POLL_INTERVAL, LOW_POWER_POLL_INTERVAL);
    }
}

static void az1uball_process_movement(struct az1uball_data *data, int delta_x, int delta_y, uint32_t time_between_interrupts, int max_speed, int max_time, float smoothing_factor) {
    const struct az1uball_config *config = data->dev->config;
    float sensitivity = parse_sensitivity(config->sensitivity);
    float scaling_factor = sensitivity;  // 基本のスケーリングファクターを感度に設定


    //●●定期的に起動確認-OK from 332

    if (time_between_interrupts < max_time) {
        // 既存の計算にsensitivityを掛ける
        float exponent = -3.0f * (float)time_between_interrupts / max_time;
        scaling_factor *= 1.0f + (max_speed - 1.0f) * expf(exponent);
    }

    // Apply scaling based on mode
    if (current_mode == AZ1UBALL_MODE_SCROLL) {
        scaling_factor *= 2.5f; // Example: Increase scaling for scroll mode
    }

    /* Accumulate deltas atomically */
    atomic_add(&data->x_buffer, delta_x);
    atomic_add(&data->y_buffer, delta_y);

    int scaled_x_movement = (int)(delta_x * scaling_factor);
    int scaled_y_movement = (int)(delta_y * scaling_factor);

    // Apply smoothing
    //XとY感度を変更(Yを抑えめに)
    data->smoothed_x = (int)(smoothing_factor * scaled_x_movement + (1.0f - smoothing_factor) * previous_x);
    data->smoothed_y = (int)(smoothing_factor * scaled_y_movement + (1.0f - smoothing_factor) * previous_y * 9 / 16);

    data->previous_x = data->smoothed_x;
    data->previous_y = data->smoothed_y;
    //●●339 data->smoothed_x != 0    OK。計算も正しそう。
    if (delta_x != 0 || delta_y != 0) {
        //●ボールタッチ時に起動OK from 336
        data->last_activity_time = k_uptime_get();
        
        if (data->is_low_power_mode) {
            // 通常モードに戻す
            data->is_low_power_mode = false;
            k_timer_stop(&data->polling_timer);
            k_timer_start(&data->polling_timer, NORMAL_POLL_INTERVAL, NORMAL_POLL_INTERVAL);
        }
    }
}

/* Execution functions for asynchronous work */
void az1uball_read_data_work(struct k_work *work)
{
    struct az1uball_data *data = CONTAINER_OF(work, struct az1uball_data, work);
    const struct az1uball_config *config = data->dev->config;
    uint8_t buf[5];
    int ret;

    // Read data from I2C
    ret = i2c_read_dt(&config->i2c, buf, sizeof(buf));

    if (ret) {
        return;
    }

    uint32_t time_between_interrupts;

    k_mutex_lock(&data->data_lock, K_FOREVER);
    time_between_interrupts = data->last_interrupt_time - data->previous_interrupt_time;
    k_mutex_unlock(&data->data_lock);

    /* Calculate deltas */
    int16_t delta_x = (int16_t)buf[1] - (int16_t)buf[0]; // RIGHT - LEFT
    int16_t delta_y = (int16_t)buf[3] - (int16_t)buf[2]; // DOWN - UP

    //●●通過OK from 335
    /* Report movement immediately if non-zero */
    if (delta_x != 0 || delta_y != 0) {
//        if (current_mode == AZ1UBALL_MODE_MOUSE) {
            az1uball_process_movement(data, delta_x, delta_y, time_between_interrupts, AZ1UBALL_MOUSE_MAX_SPEED, AZ1UBALL_MOUSE_MAX_TIME, AZ1UBALL_MOUSE_SMOOTHING_FACTOR);
            //●●起動確認-OK from 332
            /* Report relative X movement */
            if (delta_x != 0) {
                //●●通過確認-OK from 336
                ret = input_report_rel(data->dev, INPUT_REL_X, data->smoothed_x, true, K_NO_WAIT);
                //●●エラーではない from 335
            }

            /* Report relative Y movement */
            if (delta_y != 0) {
                ret = input_report_rel(data->dev, INPUT_REL_Y, data->smoothed_y, true, K_NO_WAIT);
            }
    }

    /* Update switch state */
    data->sw_pressed = (buf[4] & MSK_SWITCH_STATE) != 0;

    /* Report switch state if it changed */
    if (data->sw_pressed != data->sw_pressed_prev) {
        //ret = input_report_key(data->dev, INPUT_BTN_2, data->sw_pressed ? 1 : 0, true, K_NO_WAIT);
        //●●エラーではない from 335
        if (data->sw_pressed) {
            zmk_hid_keyboard_press(J);
        } else {
            zmk_hid_keyboard_release(J);
        }
        data->sw_pressed_prev = data->sw_pressed;
    }
}

static void az1uball_polling(struct k_timer *timer)
{
    struct az1uball_data *data = CONTAINER_OF(timer, struct az1uball_data, polling_timer);
    
    check_power_mode(data);

    uint32_t current_time = k_uptime_get();

    k_mutex_lock(&data->data_lock, K_NO_WAIT);
    data->previous_interrupt_time = data->last_interrupt_time;
    data->last_interrupt_time = current_time;
    k_mutex_unlock(&data->data_lock);

    /* Schedule the work item to handle the interrupt in thread context */
    k_work_submit(&data->work);
}


#define AZ1UBALL_DEFINE(n)                                             \
    static struct az1uball_data az1uball_data_##n;                     \
    static const struct az1uball_config az1uball_config_##n = {        \
        .i2c = I2C_DT_SPEC_INST_GET(n),                                \
        .default_mode = DT_INST_PROP_OR(n, default_mode, "mouse"),     \
        .sensitivity = DT_INST_PROP_OR(n, sensitivity, "1x"),          \
    };                                                                 \
    DEVICE_DT_INST_DEFINE(n,                                           \
                          az1uball_init,                               \
                          NULL,                                        \
                          &az1uball_data_##n,                          \
                          &az1uball_config_##n,                        \
                          POST_KERNEL,                                 \
                          CONFIG_INPUT_INIT_PRIORITY,                  \
                          NULL);

DT_INST_FOREACH_STATUS_OKAY(AZ1UBALL_DEFINE)
