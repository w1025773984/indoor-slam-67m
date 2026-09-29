/**
 * ESP32 SLAM Robot v3 主程序（ros2_control + Safety 模式）
 *
 * 与 v2 的差异：
 * - v2：透明硬件 + DiffDriveController 跑 PID
 * - v3：v2 + Safety 模块（不依赖 ROS2 的安全保护）
 *
 * Safety 逻辑（来自 oomwoo ARCHITECTURE.md 借鉴）：
 * - 急停按钮、bumper、低电量检测在 ESP32 上
 * - 即便 ROS2 死掉，机器人也能安全停机
 */

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_timer.h>

#include "motor.h"
#include "encoder.h"
#include "imu.h"
#include "safety.h"
#include "ros2_control_interface.h"

static const char* TAG = "MAIN_V3";

// ---------------- 引脚定义 ----------------
#define PIN_MOTOR_L_PWM    GPIO_NUM_32
#define PIN_MOTOR_L_DIR1    GPIO_NUM_25
#define PIN_MOTOR_L_DIR2    GPIO_NUM_26
#define PIN_MOTOR_R_PWM    GPIO_NUM_33
#define PIN_MOTOR_R_DIR1    GPIO_NUM_27
#define PIN_MOTOR_R_DIR2    GPIO_NUM_14
#define PIN_ENC_L_A         GPIO_NUM_34
#define PIN_ENC_L_B         GPIO_NUM_35
#define PIN_ENC_R_A         GPIO_NUM_36
#define PIN_ENC_R_B         GPIO_NUM_39
#define PIN_IMU_SDA         GPIO_NUM_21
#define PIN_IMU_SCL         GPIO_NUM_22

// Safety pins
#define PIN_ESTOP              GPIO_NUM_4    // 急停按钮
#define PIN_BUMPER_FL          GPIO_NUM_5    // 前左 bumper
#define PIN_BUMPER_FR          GPIO_NUM_6    // 前右 bumper
#define PIN_BUMPER_BL          GPIO_NUM_7    // 后左 bumper
#define PIN_BUMPER_BR          GPIO_NUM_8    // 后右 bumper

#define PIN_BATTERY_ADC        ADC1_CHANNEL_6  // GPIO34（注意：34 同时是编码器，需要重新分配）

// ---------------- 物理参数 ----------------
const float WHEEL_RADIUS = 0.06f;
const float WHEEL_BASE = 0.30f;
const int PULSES_PER_REV = 1320;
const float RAD_PER_PULSE = (2.0f * 3.14159265f) / PULSES_PER_REV;

// ---------------- 全局对象 ----------------
Motor* motor_left;
Motor* motor_right;
Encoder* enc_left;
Encoder* enc_right;
IMU* imu;
Safety* safety;
ROS2ControlInterface* ros2_control;

int64_t last_time_us = 0;
float left_pos_rad = 0;
float right_pos_rad = 0;

/**
 * 50 Hz 任务：硬件抽象 + Safety
 */
void control_task(void* arg) {
    const float MAX_VEL = 0.5f;

    while (1) {
        int64_t now = esp_timer_get_time();
        float dt = (now - last_time_us) / 1e6f;
        last_time_us = now;

        // Safety 检查（先于一切）
        Safety::State safety_state = safety->check();

        // 读编码器
        int64_t count_l = enc_left->getCount();
        int64_t count_r = enc_right->getCount();

        static int64_t last_count_l = 0, last_count_r = 0;
        int delta_l = count_l - last_count_l;
        int delta_r = count_r - last_count_r;
        last_count_l = count_l;
        last_count_r = count_r;

        left_pos_rad += delta_l * RAD_PER_PULSE;
        right_pos_rad += delta_r * RAD_PER_PULSE;

        float left_vel = (delta_l * RAD_PER_PULSE) / dt;
        float right_vel = (delta_r * RAD_PER_PULSE) / dt;

        // 发关节状态
        ros2_control->publishJointState(left_pos_rad, left_vel,
                                          right_pos_rad, right_vel);

        // 收 cmd_vel
        ros2_control->spinOnce();

        // 决定给电机的指令
        float pwm_l = 0, pwm_r = 0;

        if (safety->shouldStopMotors()) {
            // Safety 触发：立即停电机
            pwm_l = 0;
            pwm_r = 0;
        } else {
            // Safety 正常：执行 cmd_vel
            float vx = ros2_control->cmdLinearX();
            float wz = ros2_control->cmdAngularZ();

            float v_left = vx - wz * WHEEL_BASE / 2.0f;
            float v_right = vx + wz * WHEEL_BASE / 2.0f;

            pwm_l = v_left / MAX_VEL;
            pwm_r = v_right / MAX_VEL;

            if (pwm_l > 1.0f) pwm_l = 1.0f;
            if (pwm_l < -1.0f) pwm_l = -1.0f;
            if (pwm_r > 1.0f) pwm_r = 1.0f;
            if (pwm_r < -1.0f) pwm_r = -1.0f;
        }

        // 写电机
        motor_left->setSpeed(pwm_l);
        motor_right->setSpeed(pwm_r);

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

/**
 * 1 Hz 任务：打印 safety 状态
 */
void safety_log_task(void* arg) {
    while (1) {
        Safety::State state = safety->check();
        float voltage = safety->batteryVoltage();
        ESP_LOGI(TAG, "Safety: %s | Battery: %.2f V",
                 safety->stateName(), voltage);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "ESP32 SLAM Robot v3 (ros2_control + Safety) 启动中...");

    // 电机
    motor_left = new Motor({
        .pin_pwm = PIN_MOTOR_L_PWM,
        .pin_dir1 = PIN_MOTOR_L_DIR1,
        .pin_dir2 = PIN_MOTOR_L_DIR2,
        .ledc_channel = LEDC_CHANNEL_0,
        .ledc_timer = LEDC_TIMER_0,
        .max_duty = 8191,
    });
    motor_right = new Motor({
        .pin_pwm = PIN_MOTOR_R_PWM,
        .pin_dir1 = PIN_MOTOR_R_DIR1,
        .pin_dir2 = PIN_MOTOR_R_DIR2,
        .ledc_channel = LEDC_CHANNEL_1,
        .ledc_timer = LEDC_TIMER_1,
        .max_duty = 8191,
    });
    motor_left->init();
    motor_right->init();

    // 编码器
    enc_left = new Encoder({
        .pin_a = PIN_ENC_L_A, .pin_b = PIN_ENC_L_B,
        .pcnt_unit = PCNT_UNIT_0, .pulses_per_rev = PULSES_PER_REV,
    });
    enc_right = new Encoder({
        .pin_a = PIN_ENC_R_A, .pin_b = PIN_ENC_R_B,
        .pcnt_unit = PCNT_UNIT_1, .pulses_per_rev = PULSES_PER_REV,
    });
    enc_left->init();
    enc_right->init();

    // IMU
    imu = new IMU({
        .i2c_port = I2C_NUM_0,
        .pin_sda = PIN_IMU_SDA, .pin_scl = PIN_IMU_SCL,
        .clk_speed = 400000,
    });
    imu->init();

    // Safety（v3 新增）
    safety = new Safety({
        .pin_estop = PIN_ESTOP,
        .pin_bumper_front_left = PIN_BUMPER_FL,
        .pin_bumper_front_right = PIN_BUMPER_FR,
        .pin_bumper_back_left = PIN_BUMPER_BL,
        .pin_bumper_back_right = PIN_BUMPER_BR,
        .adc_battery = PIN_BATTERY_ADC,
        .battery_divider_ratio = 4.0f,    // 12V 电池 + 4:1 分压
        .battery_low_threshold = 10.5f,   // 12V 锂离子 < 10.5V = 低电量
        .battery_critical_threshold = 9.0f, // < 9.0V 急停
    });
    safety->init();

    // ROS2 接口
    ros2_control = new ROS2ControlInterface();
    ros2_control->init();

    // 任务
    xTaskCreate(control_task, "control", 4096, NULL, 5, NULL);
    xTaskCreate(safety_log_task, "safety_log", 2048, NULL, 3, NULL);

    ESP_LOGI(TAG, "ESP32 SLAM Robot v3 启动完成（ros2_control + Safety）");
}