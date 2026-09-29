#pragma once
/**
 * BMI088 IMU 驱动
 * I2C 接口
 * 6 轴：加速度 + 角速度
 */

#include <driver/i2c.h>

struct IMUData {
    float accel_x, accel_y, accel_z;   // m/s²
    float gyro_x, gyro_y, gyro_z;        // rad/s
    float temperature;                   // °C
    uint64_t timestamp_us;               // 微秒时间戳
};

class IMU {
public:
    struct Config {
        i2c_port_t i2c_port;
        gpio_num_t pin_sda;
        gpio_num_t pin_scl;
        uint32_t clk_speed;
    };

    IMU(const Config& cfg);
    bool init();
    bool read(IMUData& data);

private:
    Config cfg_;

    bool writeReg(BYTE reg, BYTE val);
    bool readReg(BYTE reg, BYTE* val);
    bool readMulti(BYTE start_reg, BYTE* buf, int len);
};