#include "encoder.h"
#include <esp_timer.h>

Encoder::Encoder(const Config& cfg) : cfg_(cfg), last_count_(0) {}

void Encoder::init() {
    // PCNT 配置：上升沿 + 下降沿计数（4 倍频）
    pcnt_unit_config_t unit_config = {
        .low_limit = -100000,
        .high_limit = 100000,
        .intr_priority = 0,
        .flags = {}
    };
    pcnt_new_unit(&unit_config, &channel_);

    pcnt_chan_config_t chan_config = {
        .edge_gpio_num = cfg_.pin_a,
        .level_gpio_num = cfg_.pin_b,
        .flags = {}
    };
    pcnt_channel_handle_t chan;
    pcnt_new_channel(channel_, &chan_config, &chan);

    // 上升沿计数
    pcnt_channel_set_edge_action(chan, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE);
    // 高电平 + 上升沿 = 增加
    pcnt_channel_set_level_action(chan, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);

    pcnt_unit_enable(channel_);
    pcnt_unit_clear_count(channel_);
}

int64_t Encoder::getCount() {
    int count = 0;
    pcnt_unit_get_count(channel_, &count);
    return count;
}

void Encoder::clear() {
    pcnt_unit_clear_count(channel_);
    last_count_ = 0;
}

float Encoder::getRPM(float dt_s) {
    if (dt_s <= 0) return 0;

    int64_t current = getCount();
    int64_t delta = current - last_count_;
    last_count_ = current;

    // delta 个脉冲 → delta / pulses_per_rev 转 → × 60 转/分
    float rev_per_sec = (float)delta / cfg_.pulses_per_rev / dt_s;
    return rev_per_sec * 60.0f;
}