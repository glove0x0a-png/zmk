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

struct az1uball_config {
    struct i2c_dt_spec i2c;
    const char *default_mode;
    const char *sensitivity;
};

struct az1uball_data {
    const struct device *dev;      //初期化
    struct k_work work;            //polling時、読み取り関数登録 //az1uball_read_data_work
    struct k_timer polling_timer;  //polling時、間隔制御         //az1uball_polling
    struct k_mutex data_lock;      //polling時、時間制御

    bool sw_pressed;               //前回、押下状態
    float scaling_factor;          //感度。

    uint32_t last_activity_time;    // 最後の入力があった時間
};
