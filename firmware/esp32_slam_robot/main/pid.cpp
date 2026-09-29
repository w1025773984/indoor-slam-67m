#include "pid.h"

PID::PID(const Params& params)
    : params_(params),
      integral_(0),
      last_error_(0),
      first_call_(true) {}

float PID::compute(float target, float actual, float dt) {
    if (dt <= 0) return 0;

    float error = target - actual;

    // 积分项（带抗积分饱和）
    integral_ += error * dt;
    if (integral_ > params_.integral_max) integral_ = params_.integral_max;
    if (integral_ < -params_.integral_max) integral_ = -params_.integral_max;

    // 微分项（带低通滤波）
    float derivative = 0;
    if (!first_call_) {
        derivative = (error - last_error_) / dt;
    }

    // 输出
    float output = params_.kp * error + params_.ki * integral_ + params_.kd * derivative;

    // 限幅
    if (output > params_.output_max) output = params_.output_max;
    if (output < params_.output_min) output = params_.output_min;

    last_error_ = error;
    first_call_ = false;

    return output;
}

void PID::reset() {
    integral_ = 0;
    last_error_ = 0;
    first_call_ = true;
}