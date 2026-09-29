#pragma once
/**
 * Safety 模块（v3 firmware 新增）
 *
 * 设计原则（来自 oomwoo ARCHITECTURE.md）：
 * - Safety 跑在 MCU，不依赖 Linux/ROS2
 * - 即便 ROS2 死掉，机器人也能急停
 *
 * 当前实现的 safety 输入：
 * - 紧急停止按钮（GPIO）
 * - 前 bumper 微动开关（GPIO）
 * - 后 bumper 微动开关（GPIO）
 * - 低电量检测（ADC 读电池电压）
 *
 * 触发任意 safety → 立即停电机 + 持续拒绝执行命令
 * 直到 safety 状态恢复 + 收到 reset 信号
 */

#include <driver/gpio.h>
#include <driver/adc.h>

class Safety {
public:
    struct Config {
        // 紧急停止按钮（拉低触发：按下 = 低电平）
        gpio_num_t pin_estop;

        // 前 bumper（左前 + 右前微动开关）
        gpio_num_t pin_bumper_front_left;
        gpio_num_t pin_bumper_front_right;

        // 后 bumper
        gpio_num_t pin_bumper_back_left;
        gpio_num_t pin_bumper_back_right;

        // 电池电压 ADC
        adc1_channel_t adc_battery;
        // 电压分压比（V_adc = V_bat * divider_ratio）
        float battery_divider_ratio;
        // 低电量阈值（V）
        float battery_low_threshold;
        // 急停阈值（V）—— 低于此电压整个系统停止
        float battery_critical_threshold;
    };

    enum class State {
        NORMAL,           // 正常运行
        ESTOP_ACTIVE,     // 急停按钮按下
        BUMPER_HIT,       // bumper 触发
        BATTERY_LOW,      // 低电量（警告但继续跑）
        BATTERY_CRITICAL  // 急停电压（强制停）
    };

    Safety(const Config& cfg);
    void init();

    /**
     * 检查所有 safety 条件，更新内部状态
     * @return 当前 safety state
     */
    State check();

    /**
     * 是否应该停电机
     */
    bool shouldStopMotors();

    /**
     * 清除 ESTOP_ACTIVE 状态（其他状态自动恢复）
     */
    void resetEstop();

    /**
     * 获取当前电压（V）
     */
    float batteryVoltage();

    /**
     * 获取人类可读的 status 字符串
     */
    const char* stateName();

private:
    Config cfg_;
    State state_;
    float last_voltage_;

    bool readEstop();
    bool readBumperFront();
    bool readBumperBack();
    float readBatteryVoltage();
};