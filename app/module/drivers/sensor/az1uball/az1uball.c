#define DT_DRV_COMPAT zmk_az1uball

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zmk/usb.h>
#include <zmk/hid.h>

#define JIGGLE_DELTA_X 5
#define JIGGLE_INTERVAL_MS 10000

struct az1uball_data {
    const struct device *dev;
    struct k_work work;
    struct k_timer polling_timer;
    int64_t last_jiggle_time;
};

static struct az1uball_data az1uball_driver_data;

/* ---------------------------------------------------------
 * マウスジグラー本体（ZMK HID API 使用）
 * --------------------------------------------------------- */
static void az1uball_jiggle_work(struct k_work *work)
{
    struct az1uball_data *data =
        CONTAINER_OF(work, struct az1uball_data, work);

    if (!zmk_usb_is_powered()) {
        return;
    }

    int64_t now = k_uptime_get();

    if (now - data->last_jiggle_time >= JIGGLE_INTERVAL_MS) {
        data->last_jiggle_time = now;

        /* ZMK HID API を使用して確実にマウス移動を送る */
        zmk_hid_mouse_movement_update(JIGGLE_DELTA_X, 0);
        k_sleep(K_MSEC(500));
        zmk_hid_mouse_movement_update(-JIGGLE_DELTA_X, 0);
    }
}

/* ---------------------------------------------------------
 * 1 秒周期ポーリング
 * --------------------------------------------------------- */
static void az1uball_polling(struct k_timer *timer)
{
    struct az1uball_data *data =
        CONTAINER_OF(timer, struct az1uball_data, polling_timer);

    /* USB が powered のときだけ jiggle を実行 */
    if (zmk_usb_is_powered()) {
        k_work_submit(&data->work);
    } else {
        k_timer_stop(&data->polling_timer);
    }
}

/* ---------------------------------------------------------
 * 初期化
 * --------------------------------------------------------- */
static int az1uball_init(const struct device *dev)
{
    struct az1uball_data *data = dev->data;

    data->dev = dev;
    data->last_jiggle_time = k_uptime_get();

    k_work_init(&data->work, az1uball_jiggle_work);
    k_timer_init(&data->polling_timer, az1uball_polling, NULL);

    /* USB イベントが廃止されたため、常時タイマーを開始 */
    k_timer_start(&data->polling_timer, K_MSEC(1000), K_MSEC(1000));

    return 0;
}

DEVICE_DEFINE(az1uball, "az1uball",
              az1uball_init, NULL,
              &az1uball_driver_data, NULL,
              APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
              NULL);

