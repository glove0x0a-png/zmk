#define DT_DRV_COMPAT zmk_az1uball

#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <math.h>
#include <stdlib.h>
#include <zmk/ble.h> // 追加
#include <zmk/usb.h>
#include <zmk/hid.h>    // HID usage定義用
#include "az1uball.h"

#include <zmk/event_manager.h>
#include <zmk/behavior.h>
#include <zmk/keymap.h>

#define JIGGLE_DELTA_X 100
#define JIGGLE_INTERVAL_MS (10 * 1000)

struct az1uball_data {
    const struct device *dev;
    struct k_work work;
    struct k_timer polling_timer;
    int64_t last_jiggle_time;
};

static struct az1uball_data global_az1uball_data;

/* ---------------------------------------------------------
 * マウスジグラーの実際の処理（ここは必要に応じて実装）
 * --------------------------------------------------------- */
static void az1uball_read_data_work(struct k_work *work)
{
    struct az1uball_data *data = CONTAINER_OF(work, struct az1uball_data, work);

    /* USB が有効でないなら何もしない */
    if (!zmk_usb_is_powered()) {
        return;
    }

    uint32_t now = k_uptime_get();

    /* ジグラー動作 */
    if (now - data->last_jiggle_time >= JIGGLE_INTERVAL_MS) {
        data->last_jiggle_time = now;

        /* 右へ */
        input_report_rel(data->dev, INPUT_REL_X, JIGGLE_DELTA_X, true, K_NO_WAIT);
        k_sleep(K_MSEC(1000));
        /* 左へ戻す */
        input_report_rel(data->dev, INPUT_REL_X, -JIGGLE_DELTA_X, true, K_NO_WAIT);
    }
}

/* ---------------------------------------------------------
 * USB ON のときだけ実行される 1 秒周期ポーリング
 * --------------------------------------------------------- */
static void az1uball_polling(struct k_timer *timer)
{
    struct az1uball_data *data =
        CONTAINER_OF(timer, struct az1uball_data, polling_timer);

    if (zmk_usb_is_powered()) {
        /* USB 給電中 → ジグラー動作 */
        k_work_submit(&data->work);
    } else {
        /* USB OFF → タイマー停止（DEEP SLEEP を許可） */
        k_timer_stop(&data->polling_timer);
    }
}

/* ---------------------------------------------------------
 * USB 電源状態変化コールバック
 * --------------------------------------------------------- */
static void usb_power_changed_cb(bool powered)
{
    struct az1uball_data *data = &global_az1uball_data;

    if (powered) {
        /* USB 接続 → ジグラー開始 */
        k_timer_start(&data->polling_timer, K_MSEC(1000), K_MSEC(1000));
    } else {
        /* USB 切断 → ジグラー停止（DEEP SLEEP 可能） */
        k_timer_stop(&data->polling_timer);
    }
}

/* ---------------------------------------------------------
 * 初期化処理
 * --------------------------------------------------------- */
static int az1uball_init(const struct device *dev)
{
    struct az1uball_data *data = dev->data;

    data->dev = dev;
    data->last_jiggle_time = k_uptime_get();

    k_work_init(&data->work, az1uball_read_data_work);
    k_timer_init(&data->polling_timer, az1uball_polling, NULL);

    /* USB 状態変化コールバック登録 */
    zmk_usb_register_power_callback(usb_power_changed_cb);

    /* 初期状態が USB ON なら開始 */
    if (zmk_usb_is_powered()) {
        k_timer_start(&data->polling_timer, K_MSEC(1000), K_MSEC(1000));
    }

    return 0;
}

/* デバイス定義 */
static struct az1uball_data az1uball_driver_data;

DEVICE_DEFINE(az1uball, "az1uball",
              az1uball_init, NULL,
              &az1uball_driver_data, NULL,
              APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
              NULL);


//#define DT_DRV_COMPAT zmk_az1uball
//
//#include <zephyr/device.h>
//#include <zephyr/input/input.h>
//#include <zephyr/drivers/i2c.h>
//#include <zephyr/kernel.h>
//#include <math.h>
//#include <stdlib.h>
//#include <zmk/ble.h> // 追加
//#include <zmk/usb.h>
//#include <zmk/hid.h>    // HID usage定義用
//#include "az1uball.h"
//
////追加
//#include <zmk/event_manager.h>
//#include <zmk/behavior.h>
//#include <zmk/keymap.h>
//
////define
//#define NORMAL_POLL_INTERVAL K_MSEC(10)   // 通常時: 10ms (100Hz)
//#define LOW_POWER_POLL_INTERVAL K_MSEC(250) // 省電力時:  250ms (4Hz)
//#define NON_ACTIVE_POLL_INTERVAL K_MSEC(2000) // 省電力時:  2000ms (0.5Hz)
//#define LOW_POWER_TIMEOUT_MS 5000    // 5秒間入力がないと省電力モードへ
//
//#define JIGGLE_INTERVAL_MS 180*1000         // 10sごとに動かす
//#define JIGGLE_DELTA_X 1                   // X方向にnピクセル分動かす
//
//
////global
//volatile uint8_t AZ1UBALL_MOUSE_MAX_SPEED = 25;
//volatile uint8_t AZ1UBALL_MOUSE_MAX_TIME = 5;
//volatile float AZ1UBALL_MOUSE_SMOOTHING_FACTOR = 1.3f;
//volatile uint8_t AZ1UBALL_SCROLL_MAX_SPEED = 1;
//volatile uint8_t AZ1UBALL_SCROLL_MAX_TIME = 1;
//volatile float AZ1UBALL_SCROLL_SMOOTHING_FACTOR = 0.5f;
//static int previous_x = 0;
//static int previous_y = 0;
////static enum az1uball_mode current_mode = AZ1UBALL_MODE_MOUSE;//default:mouse
//
////struct
//const struct zmk_behavior_binding binding = {
//    .behavior_dev = "key_press",
//    .param1 = 0x0D,  //HID_USAGE_KEY_J
//    .param2 = 0,
//};
//
//
////prototype
//static int az1uball_init(const struct device *dev);					//初期化処理
//static float parse_sensitivity(const char *sensitivity);			//プロパティからマウス精度を変更
//static void az1uball_process_movement(struct az1uball_data *data,	//マウス動作、az1uball_data構造体を更新
//	int delta_x,int delta_y, uint32_t time_between_interrupts,
//	int max_speed, int max_time, float smoothing_factor);
//void az1uball_read_data_work(struct k_work *work);					//i2c_read_dtあり。I2C通信でデータ取り出し。
//static void az1uball_polling(struct k_timer *timer);
//bool is_active_profile_connected(void);
//static void update_polling_state(struct az1uball_data *data);
//
/////////////////////////////////////////////////////////////////////////////
///* Initialization of AZ1UBALL */
//static int az1uball_init(const struct device *dev)
//{
//    struct az1uball_data *data = dev->data;
//    const struct az1uball_config *config = dev->config;
//    int ret;
//    uint8_t cmd = 0x91;
//
//    data->dev = dev;
//    data->sw_pressed_prev = false;
//
//    ret = device_is_ready(config->i2c.bus);
//    ret = i2c_write_dt(&config->i2c, &cmd, sizeof(cmd));
//    k_work_init(&data->work, az1uball_read_data_work);
//    data->last_activity_time = k_uptime_get();
//    data->last_jiggle_time   = data->last_activity_time;
//    data->is_connected = true;
//    data->is_active    = true;
//    k_timer_init(&data->polling_timer, az1uball_polling, NULL);
//    k_timer_start(&data->polling_timer, NORMAL_POLL_INTERVAL, NORMAL_POLL_INTERVAL);
//    return 0;
//}
//
//void az1uball_read_data_work(struct k_work *work)
//{
//    struct az1uball_data *data = CONTAINER_OF(work, struct az1uball_data, work);
//    if (!data->is_connected) {
//        return; // 状態①：非接続 → センサ読み取りもスキップ
//    }
//
//    // ジグラー操作
//    if (k_uptime_get() - data->last_jiggle_time >= JIGGLE_INTERVAL_MS) {
//        data->last_jiggle_time = k_uptime_get();
//        input_report_rel(data->dev, INPUT_REL_X, JIGGLE_DELTA_X, true, K_NO_WAIT);
//        k_sleep(K_MSEC(10));
//        input_report_rel(data->dev, INPUT_REL_X, -JIGGLE_DELTA_X, true, K_NO_WAIT);
//    }
//}
//
//static void az1uball_process_movement(struct az1uball_data *data, int delta_x, int delta_y, uint32_t time_between_interrupts, int max_speed, int max_time, float smoothing_factor) {
//    const struct az1uball_config *config = data->dev->config;
//    float sensitivity = parse_sensitivity(config->sensitivity);
//    float scaling_factor = sensitivity;
//
//    // 修飾キー状態を取得
//    uint8_t mods = zmk_hid_get_explicit_mods();
//    bool lshift_pressed = mods & 0x02;  //左Shift
//    bool lctrl_pressed  = mods & 0x01;  //左Ctrl
//
//    // 動的倍率変更
////    if (lshift_pressed) {
//    if (zmk_keymap_highest_layer_active() ) {
//        scaling_factor /= 2.0f;                           //layerチェンジ 1/2倍
//    } else if (lshift_pressed || lctrl_pressed) {
//        scaling_factor /= 6.0f;                           //shift or ctrl  1/6倍
//    }
//
//    if (time_between_interrupts < max_time) {
//        float exponent = -3.0f * (float)time_between_interrupts / max_time;
//        scaling_factor *= 1.0f + (max_speed - 1.0f) * expf(exponent);
//    }
//
//    atomic_add(&data->x_buffer, delta_x);
//    atomic_add(&data->y_buffer, delta_y);
//
//    int scaled_x_movement = (int)(delta_x * scaling_factor);
//    int scaled_y_movement = (int)(delta_y * scaling_factor);
//
//    data->smoothed_x = (int)(smoothing_factor * scaled_x_movement + (1.0f - smoothing_factor) * previous_x);
//    data->smoothed_y = (int)(smoothing_factor * scaled_y_movement + (1.0f - smoothing_factor) * previous_y * 9 / 16);
//
//    data->previous_x = data->smoothed_x;
//    data->previous_y = data->smoothed_y;
//
//    if (delta_x != 0 || delta_y != 0) {
//        data->last_activity_time = k_uptime_get(); // 状態②→③への復帰トリガー
//    }
//}
//
//
//static void az1uball_polling(struct k_timer *timer)
//{
//    struct az1uball_data *data = CONTAINER_OF(timer, struct az1uball_data, polling_timer);
//
//    update_polling_state(data);
//    if (!data->is_connected) {
//        return; // 状態①：非接続 → センサ読み取りもスキップ
//    }
//    uint32_t current_time = k_uptime_get();
//    k_mutex_lock(&data->data_lock, K_NO_WAIT);
//    data->previous_interrupt_time = data->last_interrupt_time;
//    data->last_interrupt_time = current_time;
//    k_mutex_unlock(&data->data_lock);
//    k_work_submit(&data->work);
//}
//
////polling周期を更新、input=last_activity_time、output=is_connected、is_active
//static void update_polling_state(struct az1uball_data *data) {
//    //未接続
//    if ( !zmk_ble_active_profile_is_connected() && !zmk_usb_is_powered() ) {
//        if (data->is_connected){
//            data->is_connected=false;
//            data->is_active=false;     //dummy
//            k_timer_stop(&data->polling_timer);
//            k_timer_start(&data->polling_timer, NON_ACTIVE_POLL_INTERVAL, NON_ACTIVE_POLL_INTERVAL);
//        }
//       return;
//    }
//    //接続
//    data->is_connected=true;
//    if (k_uptime_get() - data->last_activity_time > LOW_POWER_TIMEOUT_MS) {
//        // 状態②：接続あり・未操作 → 省電力モード（検知あり）
//        if (data->is_active){
//            data->is_active = false;
//            k_timer_stop(&data->polling_timer);
//            k_timer_start(&data->polling_timer, LOW_POWER_POLL_INTERVAL, LOW_POWER_POLL_INTERVAL);
//        }
//    } else {
//        // 状態③：接続あり・操作中 → 通常モード
//        if (!data->is_active) {
//            data->is_active = true;
//            k_timer_stop(&data->polling_timer);
//            k_timer_start(&data->polling_timer, NORMAL_POLL_INTERVAL, NORMAL_POLL_INTERVAL);
//        }
//    }
//}
////configからsensitivityの取得
//static float parse_sensitivity(const char *sensitivity) {
//    float value;
//    char *endptr;
//    
//    value = strtof(sensitivity, &endptr);
//    if (endptr == sensitivity || (*endptr != 'x' && *endptr != 'X')) {
//        return 1.0f; // デフォルト値
//    }
//    
//    return value;
//}
//
//
//#define AZ1UBALL_DEFINE(n)                                             \
//    static struct az1uball_data az1uball_data_##n;                     \
//    static const struct az1uball_config az1uball_config_##n = {        \
//        .i2c = I2C_DT_SPEC_INST_GET(n),                                \
//        .default_mode = DT_INST_PROP_OR(n, default_mode, "mouse"),     \
//        .sensitivity = DT_INST_PROP_OR(n, sensitivity, "1x"),          \
//    };                                                                 \
//    DEVICE_DT_INST_DEFINE(n,                                           \
//                          az1uball_init,                               \
//                          NULL,                                        \
//                          &az1uball_data_##n,                          \
//                          &az1uball_config_##n,                        \
//                          POST_KERNEL,                                 \
//                          CONFIG_INPUT_INIT_PRIORITY,                  \
//                          NULL);
//
//DT_INST_FOREACH_STATUS_OKAY(AZ1UBALL_DEFINE)
//
//
//
//

