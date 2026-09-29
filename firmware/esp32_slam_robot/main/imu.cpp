#include "imu.h"
#include <esp_log.h>
#include <cmath>

#define BMI088_ACC_ADDR 0x19
#define BMI088_GYR_ADDR 0x21

// 加速度寄存器
#define REG_ACC_CONF     0x40
#define REG_ACC_RANGE    0x41
#define REG_ACC_DATA     0x12  // 6 bytes

// 陀螺仪寄存器
#define REG_GYR_RANGE    0x0F
#define REG_GYR_BANDWIDTH 0x10
#define REG_GYR_DATA     0x02  // 6 bytes

// 加速度配置：ODR 100 Hz, OSR4
#define ACC_CONF_VAL     0xA9
// ±3g 范围
#define ACC_RANGE_VAL    0x00
// 陀螺仪：±1000 dps
#define GYR_RANGE_VAL    0x01
// 陀螺仪带宽：ODR 100 Hz
#define GYR_BANDWIDTH_VAL 0x02

#define ACC_SENSITIVITY  (3.0f * 9.8f / 32768.0f)  // ±3g → g → m/s²
#define GYR_SENSITIVITY  (1000.0f / 32768.0f * M_PI / 180.0f)  // ±1000 dps → rad/s

static const char* TAG = "IMU";

IMU::IMU(const Config& cfg) : cfg_(cfg) {}

bool IMU::init() {
    // I2C 初始化
    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = cfg_.pin_sda;
    conf.scl_io_num = cfg_.pin_scl;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = cfg_.clk_speed;
    i2c_param_config(cfg_.i2c_port, &conf);
    i2c_driver_install(cfg_.i2c_port, conf.mode, 0, 0, 0);

    // 软复位 + 配置
    if (!writeReg(BMI088_ACC_ADDR, 0x7E, 0xB6)) return false;
    vTaskDelay(pdMS_TO_TICKS(100));
    if (!writeReg(BMI088_GYR_ADDR, 0x14, 0xB6)) return false;
    vTaskDelay(pdMS_TO_TICKS(100));

    // 配置加速度
    if (!writeReg(BMI088_ACC_ADDR, REG_ACC_CONF, ACC_CONF_VAL)) return false;
    if (!writeReg(BMI088_ACC_ADDR, REG_ACC_RANGE, ACC_RANGE_VAL)) return false;

    // 配置陀螺仪
    if (!writeReg(BMI088_GYR_ADDR, REG_GYR_RANGE, GYR_RANGE_VAL)) return false;
    if (!writeReg(BMI088_GYR_ADDR, REG_GYR_BANDWIDTH, GYR_BANDWIDTH_VAL)) return false;

    ESP_LOGI(TAG, "BMI088 初始化完成");
    return true;
}

bool IMU::read(IMUData& data) {
    BYTE acc_buf[6];
    if (!readMulti(BMI088_ACC_ADDR, REG_ACC_DATA, acc_buf, 6)) return false;

    BYTE gyr_buf[6];
    if (!readMulti(BMI088_GYR_ADDR, REG_GYR_DATA, gyr_buf, 6)) return false;

    int16_t ax = (int16_t)((acc_buf[1] << 8) | acc_buf[0]);
    int16_t ay = (int16_t)((acc_buf[3] << 8) | acc_buf[2]);
    int16_t az = (int16_t)((acc_buf[5] << 8) | acc_buf[4]);

    int16_t gx = (int16_t)((gyr_buf[1] << 8) | gyr_buf[0]);
    int16_t gy = (int16_t)((gyr_buf[3] << 8) | gyr_buf[2]);
    int16_t gz = (int16_t)((gyr_buf[5] << 8) | gyr_buf[4]);

    data.accel_x = ax * ACC_SENSITIVITY;
    data.accel_y = ay * ACC_SENSITIVITY;
    data.accel_z = az * ACC_SENSITIVITY;
    data.gyro_x = gx * GYR_SENSITIVITY;
    data.gyro_y = gy * GYR_SENSITIVITY;
    data.gyro_z = gz * GYR_SENSITIVITY;

    data.timestamp_us = esp_timer_get_time();
    return true;
}

bool IMU::writeReg(BYTE addr, BYTE reg, BYTE val) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | 0, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, val, true);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(cfg_.i2c_port, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return err == ESP_OK;
}

bool IMU::readReg(BYTE addr, BYTE reg, BYTE* val) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | 0, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | 1, true);
    i2c_master_read_byte(cmd, val, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(cfg_.i2c_port, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return err == ESP_OK;
}

bool IMU::readMulti(BYTE addr, BYTE start_reg, BYTE* buf, int len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | 0, true);
    i2c_master_write_byte(cmd, start_reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (addr << 1) | 1, true);
    i2c_master_read(cmd, buf, len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    esp_err_t err = i2c_master_cmd_begin(cfg_.i2c_port, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return err == ESP_OK;
}