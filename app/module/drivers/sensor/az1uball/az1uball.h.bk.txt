#pragma once

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/mutex.h>

/* Bit Masks */
#define MSK_SWITCH_STATE    0b10000000

/* Mode definitions */
enum az1uball_mode {
    AZ1UBALL_MODE_MOUSE,
    AZ1UBALL_MODE_SCROLL
};

/* Polling modes */
enum az1uball_poll_mode {
    POLL_MODE_NOR = 0,   // 20ms
    POLL_MODE_BLE = 1,   // 1000ms
    POLL_MODE_JIG = 2    // JIG_WAIT_MS（4分）
};

struct az1uball_config {
    struct i2c_dt_spec i2c;
    const char *default_mode;
    const char *sensitivity;
};

struct az1uball_data {
    const struct device *dev;

    /* Work / Timer */
    struct k_work work;             // I2C 読み取り
    struct k_timer polling_timer;   // ポーリング制御
    struct k_work_delayable jiggler_work; // ★ ジグラー専用ワーク

    struct k_mutex data_lock;

    /* 状態管理 */
    bool sw_pressed;
    float pre_x;
    float pre_y;
    float scaling_factor;

    uint32_t last_activity_time;

    /* ★ 追加：ジグラー ON/OFF */
    bool jiggler_on;

    /* ★ 追加：ポーリングモード */
    uint8_t poll_mode;
};
