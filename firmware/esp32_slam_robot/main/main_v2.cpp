/**
 * ESP32 SLAM Robot v2 主程序（ros2_control 模式）
 *
 * 模式说明：
 * - ESP32 是"透明硬件抽象"：只做硬件 I/O
 * - PID 完全在 ROS2 上的 DiffDriveController 跑
 * - 数据流：
 *   - ROS2 DiffDriveController → /cmd_vel (Twist)
 *   - ESP32 收到 cmd_vel → 解析成左右轮速度 → 写 PWM
 *   - ESP32 读编码器 → 计算关节状态 → 发 /joint_states
 *
 * 与 v1 的对比：
 * - v1：ESP32 跑 PID，发 /odom，收 /cmd_vel
 * - v2：ESP32 只做硬件抽象，发 /joint_states，收 /cmd_vel
 */

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_timer.h>

#include "motor.h"
#include "encoder.h"
#include "imu.h"
#include "ros2_control_interface.h"

static const char* TAG = "MAIN_V2";

// ---------------- 引脚定义 ----------------
#define PIN_MOTOR_L_PWM   GPIO_NUM_32
#define PIN_MOTOR_L_DIR1   GPIO_NUM_25
#define PIN_MOTOR_L_DIR2   GPIO_NUM_26
#define PIN_MOTOR_R_PWM   GPIO_NUM_33
#define PIN_MOTOR_R_DIR1   GPIO_NUM_27
#define PIN_MOTOR_R_DIR2   GPIO_NUM_14
#define PIN_ENC_L_A        GPIO_NUM_34
#define PIN_ENC_L_B        GPIO_NUM_35
#define PIN_ENC_R_A        GPIO_NUM_36
#define PIN_ENC_R_B        GPIO_NUM_39
#define PIN_IMU_SDA        GPIO_NUM_21
#define PIN_IMU_SCL        GPIO_NUM_22

// ---------------- 物理参数 ----------------
const float WHEEL_RADIUS = 0.06f;       // m
const float WHEEL_BASE = 0.30f;          // m
const int PULSES_PER_REV = 1320;         // 编码器分辨率
const float RAD_PER_PULSE = (2.0f * 3.14159265f) / PULSES_PER_REV;

// ---------------- 全局对象 ----------------
Motor* motor_left;
Motor* motor_right;
Encoder* enc_left;
Encoder* enc_right;
IMU* imu;
ROS2ControlInterface* ros2_control;

int64_t last_time_us = 0;

// 累积位置（用于发布关节状态）
float left_pos_rad = 0;
float right_pos_rad = 0;

/**
 * 50 Hz 任务：
 * - 读编码器
 * - 更新位置
 * - 发 joint_states
 * - 收 cmd_vel
 * - 写 PWM（不做 PID）
 */
void control_task(void* arg) {
    const float cmd_timeout_s = 0.5f;  // 500ms 没新命令就停

    while (1) {
        int64_t now = esp_timer_get_time();
        float dt = (now - last_time_us) / 1e6f;
        last_time_us = now;

        // 读编码器
        int64_t count_l = enc_left->getCount();
        int64_t count_r = enc_right->getCount();

        // 关节位置（累积弧度）
        static int64_t last_count_l = 0, last_count_r = 0;
        int delta_l = count_l - last_count_l;
        int delta_r = count_r - last_count_r;
        last_count_l = count_l;
        last_count_r = count_r;

        left_pos_rad += delta_l * RAD_PER_PULSE;
        right_pos_rad += delta_r * RAD_PER_PULSE;

        // 关节速度（弧度/秒）
        float left_vel = (delta_l * RAD_PER_PULSE) / dt;
        float right_vel = (delta_r * RAD_PER_PULSE) / dt;

        // 发布关节状态
        ros2_control->publishJointState(left_pos_rad, left_vel,
                                          right_pos_rad, right_vel);

        // 收 cmd_vel
        ros2_control->spinOnce();

        // 解析 cmd_vel 到左右轮目标速度（m/s）
        float vx = ros2_control->cmdLinearX();
        float wz = ros2_control->cmdAngularZ();

        float v_left = vx - wz * WHEEL_BASE / 2.0f;
        float v_right = vx + wz * WHEEL_BASE / 2.0f;

        // 转 PWM 占空比（v / max_v），不做 PID
        const float MAX_VEL = 0.5f;  // m/s 最大速度
        float pwm_l = v_left / MAX_VEL;
        float pwm_r = v_right / MAX_VEL;

        // 限制范围
        if (pwm_l > 1.0f) pwm_l = 1.0f;
        if (pwm_l < -1.0f) pwm_l = -1.0f;
        if (pwm_r > 1.0f) pwm_r = 1.0f;
        if (pwm_r < -1.0f) pwm_r = -1.0f;

        // 写电机
        motor_left->setSpeed(pwm_l);
        motor_right->setSpeed(pwm_r);

        // 50 Hz = 20 ms
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "ESP32 SLAM Robot v2 (ros2_control 模式) 启动中...");

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

    // IMU（ros2_control 模式下 IMU 数据由 robot_localization 用，所以仍然采集）
    imu = new IMU({
        .i2c_port = I2C_NUM_0,
        .pin_sda = PIN_IMU_SDA, .pin_scl = PIN_IMU_SCL,
        .clk_speed = 400000,
    });
    imu->init();

    // ros2_control 接口
    ros2_control = new ROS2ControlInterface();
    ros2_control->init();

    // 单任务：50 Hz
    xTaskCreate(control_task, "control", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "ESP32 SLAM Robot v2 启动完成（ros2_control 透明模式）");
}