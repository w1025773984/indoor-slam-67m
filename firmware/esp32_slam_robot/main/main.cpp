/**
 * ESP32 SLAM Robot 主程序
 *
 * 频率：
 * - PID 控制：100 Hz（每 10 ms）
 * - 里程计发布：50 Hz（每 20 ms）
 * - IMU 发布：50 Hz（每 20 ms）
 *
 * 流程：
 * 1. 初始化 WiFi → micro-ROS agent
 * 2. 初始化电机、编码器、IMU
 * 3. 启动 FreeRTOS 任务：motor_control + ros2_publish
 */

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <micro_ros_rosidl_types_c/int32.h>
#include <micro_ros_rosidl_types_c/msg/header.h>

#include "motor.h"
#include "encoder.h"
#include "pid.h"
#include "imu.h"
#include "odometry.h"
#include "ros2_node.h"

static const char* TAG = "MAIN";

// ---------------- 引脚定义（按需调整） ----------------
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

// ---------------- 全局对象 ----------------
Motor* motor_left;
Motor* motor_right;
Encoder* enc_left;
Encoder* enc_right;
PID* pid_left;
PID* pid_right;
IMU* imu;
Odometry* odom;
ROS2Node* ros2;

int64_t last_pid_time_us = 0;
int64_t last_pub_time_us = 0;

float target_v_left = 0;
float target_v_right = 0;

/**
 * 把 ROS2 /cmd_vel 的 (linear_x, angular_z) 转换为左右轮目标速度
 * linear_x: m/s
 * angular_z: rad/s
 */
void computeTwist_diff(float linear_x, float angular_z,
                       float wheel_base, float& v_left, float& v_right) {
    v_left = linear_x - angular_z * wheel_base / 2.0f;
    v_right = linear_x + angular_z * wheel_base / 2.0f;
}

/**
 * 控制任务：100 Hz
 * 读编码器 → PID 计算 → 写电机
 */
void motor_control_task(void* arg) {
    const float WHEEL_BASE = 0.30f;   // 30 cm
    const float WHEEL_RADIUS = 0.06f; // 6 cm
    const int PULSES_PER_REV = 1320;   // 660 线 × 4 倍频
    const float PID_DT = 0.01f;         // 10 ms

    odom = new Odometry({WHEEL_RADIUS, WHEEL_BASE, PULSES_PER_REV});

    while (1) {
        int64_t now = esp_timer_get_time();
        float dt = (now - last_pid_time_us) / 1e6f;
        last_pid_time_us = now;

        // 读编码器
        int64_t count_l = enc_left->getCount();
        int64_t count_r = enc_right->getCount();

        // 计算实际转速（RPM）
        static int64_t last_count_l = 0, last_count_r = 0;
        int delta_l = count_l - last_count_l;
        int delta_r = count_r - last_count_r;
        last_count_l = count_l;
        last_count_r = count_r;

        // 计算实际速度（m/s）
        float dist_per_pulse = (2.0f * M_PI * WHEEL_RADIUS) / PULSES_PER_REV;
        float v_actual_l = (delta_l * dist_per_pulse) / dt;
        float v_actual_r = (delta_r * dist_per_pulse) / dt;

        // 取最新 /cmd_vel 转换目标速度
        TwistCommand cmd = ros2->getCmdVel();
        computeTwist_diff(cmd.linear_x, cmd.angular_z, WHEEL_BASE,
                          target_v_left, target_v_right);

        // PID 计算
        float pwm_l = pid_left->compute(target_v_left, v_actual_l, PID_DT);
        float pwm_r = pid_right->compute(target_v_right, v_actual_r, PID_DT);

        // 写电机（PID 输出 → 占空比 -1.0 ~ 1.0）
        motor_left->setSpeed(pwm_l);
        motor_right->setSpeed(pwm_r);

        // 更新里程计
        odom->update(delta_l, delta_r, dt);

        vTaskDelay(pdMS_TO_TICKS(10));  // 100 Hz
    }
}

/**
 * ROS2 发布任务：50 Hz
 * 发布 /odom + /imu/data
 */
void ros2_publish_task(void* arg) {
    while (1) {
        IMUData imu_data;
        if (imu->read(imu_data)) {
            ros2->publishIMU(
                imu_data.accel_x, imu_data.accel_y, imu_data.accel_z,
                imu_data.gyro_x, imu_data.gyro_y, imu_data.gyro_z
            );
        }

        ros2->publishOdometry(
            odom->x(), odom->y(), odom->theta(),
            odom->linear_vel(), odom->angular_vel(),
            0, 0  // 左右轮单独速度，可扩展
        );

        ros2->spinOnce();

        vTaskDelay(pdMS_TO_TICKS(20));  // 50 Hz
    }
}

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "ESP32 SLAM Robot 启动中...");

    // 初始化电机
    motor_left = new Motor({
        .pin_pwm = PIN_MOTOR_L_PWM,
        .pin_dir1 = PIN_MOTOR_L_DIR1,
        .pin_dir2 = PIN_MOTOR_L_DIR2,
        .ledc_channel = LEDC_CHANNEL_0,
        .ledc_timer = LEDC_TIMER_0,
        .max_duty = 8191,  // 13-bit resolution
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

    // 初始化编码器
    enc_left = new Encoder({
        .pin_a = PIN_ENC_L_A,
        .pin_b = PIN_ENC_L_B,
        .pcnt_unit = PCNT_UNIT_0,
        .pulses_per_rev = 1320,
    });
    enc_right = new Encoder({
        .pin_a = PIN_ENC_R_A,
        .pin_b = PIN_ENC_R_B,
        .pcnt_unit = PCNT_UNIT_1,
        .pulses_per_rev = 1320,
    });
    enc_left->init();
    enc_right->init();

    // 初始化 PID（需要现场调！）
    pid_left = new PID({
        .kp = 1.5f, .ki = 5.0f, .kd = 0.05f,
        .integral_max = 1.0f,
        .output_min = -1.0f,
        .output_max = 1.0f,
    });
    pid_right = new PID({
        .kp = 1.5f, .ki = 5.0f, .kd = 0.05f,
        .integral_max = 1.0f,
        .output_min = -1.0f,
        .output_max = 1.0f,
    });

    // 初始化 IMU
    imu = new IMU({
        .i2c_port = I2C_NUM_0,
        .pin_sda = PIN_IMU_SDA,
        .pin_scl = PIN_IMU_SCL,
        .clk_speed = 400000,
    });
    if (!imu->init()) {
        ESP_LOGE(TAG, "BMI088 初始化失败！");
    }

    // 初始化 ROS2（micro-ROS 在 transport.h 中启动）
    ros2 = new ROS2Node();
    ros2->init();

    // 启动任务
    xTaskCreate(motor_control_task, "motor_ctrl", 4096, NULL, 5, NULL);
    xTaskCreate(ros2_publish_task, "ros2_pub", 8192, NULL, 4, NULL);

    ESP_LOGI(TAG, "ESP32 SLAM Robot 启动完成");
}