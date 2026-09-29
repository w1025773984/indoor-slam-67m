#pragma once
/**
 * 编码器读取（带方向的方波计数）
 * 用 PCNT（Pulse Counter）硬件计数
 */

#include <driver/pulse_cnt.h>

class Encoder {
public:
    struct Config {
        gpio_num_t pin_a;
        gpio_num_t pin_b;
        pcnt_unit_t pcnt_unit;
        int pulses_per_rev;  // 编码器线数 × 倍频
    };

    Encoder(const Config& cfg);
    void init();

    /** 获取累计计数 */
    int64_t getCount();

    /** 清零 */
    void clear();

    /** 转换为 RPM（每分钟转速） */
    float getRPM(float dt_s);

private:
    Config cfg_;
    pcnt_channel_handle_t channel_;
    int64_t last_count_;
};