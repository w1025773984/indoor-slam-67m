#include "motor.h"

Motor::Motor(const Config& cfg) : cfg_(cfg) {}

void Motor::init() {
    // GPIO
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << cfg_.pin_dir1) | (1ULL << cfg_.pin_dir2);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    // LEDC 通道
    ledc_channel_config_t ledc_ch = {};
    ledc_ch.gpio_num = cfg_.pin_pwm;
    ledc_ch.speed_mode = LEDC_LOW_SPEED_MODE;
    ledc_ch.channel = cfg_.ledc_channel;
    ledc_ch.timer_sel = cfg_.ledc_timer;
    ledc_ch.duty = 0;
    ledc_ch.hpoint = 0;
    ledc_channel_config(&ledc_ch);

    // 默认停
    setSpeed(0);
}

void Motor::setSpeed(float speed) {
    if (speed > 1.0f) speed = 1.0f;
    if (speed < -1.0f) speed = -1.0f;

    bool forward = (speed >= 0);
    float magnitude = (forward ? speed : -speed);

    setDirection(forward);

    // 死区补偿（电机在低 PWM 下不动）
    // 最小启动 PWM 约 15-20%
    const float DEAD_ZONE = 0.15f;
    if (magnitude > 0 && magnitude < DEAD_ZONE) {
        magnitude = DEAD_ZONE;
    }

    int duty = (int)(magnitude * cfg_.max_duty);
    setDuty(duty);
}

void Motor::setDirection(bool forward) {
    if (forward) {
        gpio_set_level(cfg_.pin_dir1, 1);
        gpio_set_level(cfg_.pin_dir2, 0);
    } else {
        gpio_set_level(cfg_.pin_dir1, 0);
        gpio_set_level(cfg_.pin_dir2, 1);
    }
}

void Motor::setDuty(int duty) {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, cfg_.ledc_channel, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, cfg_.ledc_channel);
}