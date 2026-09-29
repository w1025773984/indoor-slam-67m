#pragma once
/**
 * PID 控制器
 * 输出 = kp * err + ki * ∫err + kd * derr/dt
 */

class PID {
public:
    struct Params {
        float kp;
        float ki;
        float kd;
        float integral_max;  // 抗积分饱和
        float output_min;
        float output_max;
    };

    PID(const Params& params);

    /**
     * 计算输出
     * @param target    目标值
     * @param actual    实际值
     * @param dt        距离上次（分钟）
     */
    float compute(float target, float actual, float dt);

    void reset();

private:
    Params params_;
    float integral_;
    float last_error_;
    bool first_call_;
};