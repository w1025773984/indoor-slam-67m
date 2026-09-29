#include "odometry.h"
#include <cmath>

Odometry::Odometry(const Params& params)
    : params_(params),
      x_(0), y_(0), theta_(0),
      v_(0), omega_(0) {}

void Odometry::update(int delta_left, int delta_right, float dt) {
    if (dt <= 0) return;

    // 编码器脉冲 → 距离
    float dist_per_pulse = (2.0f * M_PI * params_.wheel_radius) / params_.pulses_per_rev;
    float dist_left = delta_left * dist_per_pulse;
    float dist_right = delta_right * dist_per_pulse;

    // 差速运动学
    float dist_center = (dist_left + dist_right) / 2.0f;
    float delta_theta = (dist_right - dist_left) / params_.wheel_base;

    // 更新位姿
    float theta_mid = theta_ + delta_theta / 2.0f;
    x_ += dist_center * cos(theta_mid);
    y_ += dist_center * sin(theta_mid);
    theta_ += delta_theta;

    // 角度归一化
    if (theta_ > M_PI) theta_ -= 2 * M_PI;
    if (theta_ < -M_PI) theta_ += 2 * M_PI;

    // 速度
    v_ = dist_center / dt;
    omega_ = delta_theta / dt;
}

void Odometry::reset() {
    x_ = y_ = theta_ = 0;
    v_ = omega_ = 0;
}