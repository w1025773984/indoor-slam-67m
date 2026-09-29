#pragma once
/**
 * 双直流电机驱动
 * PWM 控制速度，GPIO 控制方向
 * 用 LEDC（ESP32 硬件 PWM）
 */

#include <driver/ledc.h>

class Motor {
public:
    struct Config {
        gpio_num_t pin_pwm;
        gpio_num_t pin_dir1;
        gpio_num_t pin_dir2;
        ledc_channel_t ledc_channel;
        ledc_timer_t ledc_timer;
        int max_duty;  // 最大占空比（分辨率）
    };

    Motor(const Config& cfg);
    void init();

    /**
     * 设置电机速度
     * @param speed    -1.0 ~ 1.0
     *                 正数：正转
     *                 负数：反转
     *                 0：停
     */
    void setSpeed(float speed);

private:
    Config cfg_;

    void setDirection(bool forward);
    void setDuty(int duty);
};