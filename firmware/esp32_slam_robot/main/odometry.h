#pragma once
/**
 * 差速里程计计算
 * 输入：左右轮编码器增量
 * 输出：机器人在 odom 坐标系下的 (x, y, theta)
 */

class Odometry {
public:
    struct Params {
        float wheel_radius;   // 米
        float wheel_base;     // 轮距，米
        int   pulses_per_rev; // 编码器总脉冲/圈
    };

    Odometry(const Params& params);

    /**
     * 更新里程计
     * @param delta_left  左编码器增量（脉冲）
     * @param delta_right 右编码器增量（脉冲）
     * @param dt          距离上次（分钟）
     */
    void update(int delta_left, int delta_right, float dt);

    float x() const { return x_; }
    float y() const { return y_; }
    float theta() const { return theta_; }

    /** 速度估计（m/s, rad/s） */
    float linear_vel() const { return v_; }
    float angular_vel() const { return omega_; }

    void reset();

private:
    Params params_;
    float x_, y_, theta_;
    float v_, omega_;
};