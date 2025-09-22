#define DT_DRV_COMPAT zmk_az1uball

#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <math.h>
#include <stdlib.h>
#include <zmk/ble.h> // 追加
#include "az1uball.h"

//追加
#include <zmk/event_manager.h>
#include <zmk/behavior.h>
#include <zmk/keymap.h>

//define
#define NORMAL_POLL_INTERVAL K_MSEC(10)   // 通常時: 10ms (100Hz)
#define LOW_POWER_POLL_INTERVAL K_MSEC(500) // 省電力時: 500ms (2Hz)
#define LOW_POWER_TIMEOUT_MS 5000    // 5秒間入力がないと省電力モードへ

#define JIGGLE_INTERVAL_MS 10*1000         // 10sごとに動かす
#define JIGGLE_DELTA_X 100                   // X方向にnピクセル分動かす


//global
volatile uint8_t AZ1UBALL_MOUSE_MAX_SPEED = 25;
volatile uint8_t AZ1UBALL_MOUSE_MAX_TIME = 5;
volatile float AZ1UBALL_MOUSE_SMOOTHING_FACTOR = 1.3f;
volatile uint8_t AZ1UBALL_SCROLL_MAX_SPEED = 1;
volatile uint8_t AZ1UBALL_SCROLL_MAX_TIME = 1;
volatile float AZ1UBALL_SCROLL_SMOOTHING_FACTOR = 0.5f;
static int previous_x = 0;
static int previous_y = 0;
//static enum az1uball_mode current_mode = AZ1UBALL_MODE_MOUSE;//default:mouse

//struct
const struct zmk_behavior_binding binding = {
    .behavior_dev = "key_press",
    .param1 = 0x0D,  //HID_USAGE_KEY_J
    .param2 = 0,
};


//prototype
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
    struct az1uball_data *data = dev->data;
    const struct az1uball_config *config = dev->config;
    int ret;
    uint8_t cmd = 0x91;
    data->dev = dev;
    data->sw_pressed_prev = false;
    data->last_jiggle_time = k_uptime_get();

    ret = device_is_ready(config->i2c.bus);
    ret = i2c_write_dt(&config->i2c, &cmd, sizeof(cmd));
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
        data->is_low_power_mode = true;
        k_timer_stop(&data->polling_timer);
        k_timer_start(&data->polling_timer, LOW_POWER_POLL_INTERVAL, LOW_POWER_POLL_INTERVAL);
    }
}

static void az1uball_process_movement(struct az1uball_data *data, int delta_x, int delta_y, uint32_t time_between_interrupts, int max_speed, int max_time, float smoothing_factor) {
    const struct az1uball_config *config = data->dev->config;
    float sensitivity = parse_sensitivity(config->sensitivity);
    float scaling_factor = sensitivity;
    if (time_between_interrupts < max_time) {
        float exponent = -3.0f * (float)time_between_interrupts / max_time;
        scaling_factor *= 1.0f + (max_speed - 1.0f) * expf(exponent);
    }

    atomic_add(&data->x_buffer, delta_x);
    atomic_add(&data->y_buffer, delta_y);

    int scaled_x_movement = (int)(delta_x * scaling_factor);
    int scaled_y_movement = (int)(delta_y * scaling_factor);

    //XとY感度を変更(Yを抑えめに)
    data->smoothed_x = (int)(smoothing_factor * scaled_x_movement + (1.0f - smoothing_factor) * previous_x);
    data->smoothed_y = (int)(smoothing_factor * scaled_y_movement + (1.0f - smoothing_factor) * previous_y * 9 / 16);

    data->previous_x = data->smoothed_x;
    data->previous_y = data->smoothed_y;
    if (delta_x != 0 || delta_y != 0) {
        data->last_activity_time = k_uptime_get();
        if (data->is_low_power_mode) {
            data->is_low_power_mode = false;
            k_timer_stop(&data->polling_timer);
            k_timer_start(&data->polling_timer, NORMAL_POLL_INTERVAL, NORMAL_POLL_INTERVAL);
        }
    }
}

void az1uball_read_data_work(struct k_work *work)
{
    struct az1uball_data *data = CONTAINER_OF(work, struct az1uball_data, work);
    const struct az1uball_config *config = data->dev->config;
    uint8_t buf[5];
    int ret;
    uint32_t time_between_interrupts;

    ret = i2c_read_dt(&config->i2c, buf, sizeof(buf));
    k_mutex_lock(&data->data_lock, K_FOREVER);
    time_between_interrupts = data->last_interrupt_time - data->previous_interrupt_time;
    k_mutex_unlock(&data->data_lock);

    int16_t delta_x = (int16_t)buf[1] - (int16_t)buf[0]; // RIGHT - LEFT
    int16_t delta_y = (int16_t)buf[3] - (int16_t)buf[2]; // DOWN - UP

    if (delta_x != 0 || delta_y != 0) {
            az1uball_process_movement(data, delta_x, delta_y, time_between_interrupts, AZ1UBALL_MOUSE_MAX_SPEED, AZ1UBALL_MOUSE_MAX_TIME, AZ1UBALL_MOUSE_SMOOTHING_FACTOR);
            if (delta_x != 0) {
                ret = input_report_rel(data->dev, INPUT_REL_X, data->smoothed_x, true, K_NO_WAIT);
            }
            if (delta_y != 0) {
                ret = input_report_rel(data->dev, INPUT_REL_Y, data->smoothed_y, true, K_NO_WAIT);
            }
    }
    data->sw_pressed = (buf[4] & MSK_SWITCH_STATE) != 0;
    if (data->sw_pressed != data->sw_pressed_prev) {
        struct zmk_behavior_binding_event event = {
            .position = 0,
            .timestamp = k_uptime_get(),
            .layer = 0,
        };
        zmk_behavior_invoke_binding(&binding, event, data->sw_pressed);
        data->sw_pressed_prev = data->sw_pressed;
    }

    // 定期的なマウスカーソル移動処理
    if (k_uptime_get() - data->last_jiggle_time >= JIGGLE_INTERVAL_MS) {
        data->last_jiggle_time = k_uptime_get();
        //ジグラー操作は、省電力切替に無関係。az1uball_process_movementは起動しない。//az1uball_process_movement(data, (int)JIGGLE_DELTA_X, 0, time_between_interrupts, AZ1UBALL_MOUSE_MAX_SPEED, AZ1UBALL_MOUSE_MAX_TIME, AZ1UBALL_MOUSE_SMOOTHING_FACTOR);
        input_report_rel(data->dev, INPUT_REL_X, (int)JIGGLE_DELTA_X, true, K_NO_WAIT);
        while(k_uptime_get() - data->last_jiggle_time > 2000){};
        input_report_rel(data->dev, INPUT_REL_X, (int)-1*JIGGLE_DELTA_X, true, K_NO_WAIT);
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
