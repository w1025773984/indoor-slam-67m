#include "safety.h"
#include <esp_log.h>

static const char* TAG = "SAFETY";

Safety::Safety(const Config& cfg) : cfg_(cfg), state_(State::NORMAL), last_voltage_(0) {}

void Safety::init() {
    // 紧急停止按钮（输入模式 + 上拉）
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << cfg_.pin_estop);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;  // 上拉，按下 = 低电平
    gpio_config(&io_conf);

    // Bumper 4 个
    io_conf.pin_bit_mask = (1ULL << cfg_.pin_bumper_front_left) |
                           (1ULL << cfg_.pin_bumper_front_right) |
                           (1ULL << cfg_.pin_bumper_back_left) |
                           (1ULL << cfg_.pin_bumper_back_right);
    gpio_config(&io_conf);

    // ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(cfg_.adc_battery, ADC_ATTEN_DB_11);

    ESP_LOGI(TAG, "Safety initialized");
}

Safety::State Safety::check() {
    // 优先级：ESTOP > BATTERY_CRITICAL > BUMPER > BATTERY_LOW
    if (readEstop()) {
        state_ = State::ESTOP_ACTIVE;
        return state_;
    }

    last_voltage_ = readBatteryVoltage();
    if (last_voltage_ < cfg_.battery_critical_threshold) {
        state_ = State::BATTERY_CRITICAL;
        return state_;
    }

    if (readBumperFront() || readBumperBack()) {
        state_ = State::BUMPER_HIT;
        return state_;
    }

    if (last_voltage_ < cfg_.battery_low_threshold) {
        state_ = State::BATTERY_LOW;  // 警告但不强制停
    } else {
        state_ = State::NORMAL;
    }

    return state_;
}

bool Safety::shouldStopMotors() {
    return state_ == State::ESTOP_ACTIVE ||
           state_ == State::BUMPER_HIT ||
           state_ == State::BATTERY_CRITICAL;
}

void Safety::resetEstop() {
    if (state_ == State::ESTOP_ACTIVE) {
        // 只在按钮释放后才能 reset
        if (!readEstop()) {
            state_ = State::NORMAL;
        }
    }
}

float Safety::batteryVoltage() {
    return last_voltage_;
}

const char* Safety::stateName() {
    switch (state_) {
        case State::NORMAL:           return "NORMAL";
        case State::ESTOP_ACTIVE:     return "ESTOP_ACTIVE";
        case State::BUMPER_HIT:       return "BUMPER_HIT";
        case State::BATTERY_LOW:      return "BATTERY_LOW";
        case State::BATTERY_CRITICAL: return "BATTERY_CRITICAL";
        default: return "UNKNOWN";
    }
}

bool Safety::readEstop() {
    // 上拉 + 按下低电平
    return gpio_get_level(cfg_.pin_estop) == 0;
}

bool Safety::readBumperFront() {
    return gpio_get_level(cfg_.pin_bumper_front_left) == 0 ||
           gpio_get_level(cfg_.pin_bumper_front_right) == 0;
}

bool Safety::readBumperBack() {
    return gpio_get_level(cfg_.pin_bumper_back_left) == 0 ||
           gpio_get_level(cfg_.pin_bumper_back_right) == 0;
}

float Safety::readBatteryVoltage() {
    int raw = adc1_get_raw(cfg_.adc_battery);
    // 12-bit ADC: 0-4095 对应 0-3.3V
    float v_adc = (raw / 4095.0f) * 3.3f;
    // 反推电池电压（考虑分压比）
    return v_adc / cfg_.battery_divider_ratio;
}