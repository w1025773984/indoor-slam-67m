#pragma once
/**
 * ros2_control 模式接口
 *
 * 让 ESP32 变成"透明硬件抽象"：
 * - 读编码器 → 发 /joint_states
 * - 收 /cmd_vel（Twist）→ 写 PWM
 * - **不跑 PID**（PID 完全在 ROS2 上的 DiffDriveController 跑）
 *
 * 优势：
 * - 标准化（ros2_control 是 ROS2 官方规范）
 * - 换底盘只换 hardware interface
 * - PID 调参可以在 RViz 实时调整
 */

#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <geometry_msgs/msg/twist.h>
#include <sensor_msgs/msg/joint_state.h>

class ROS2ControlInterface {
public:
    bool init();

    /**
     * 发送 joint_states
     * @param left_pos   左轮位置（弧度）
     * @param left_vel   左轮速度（弧度/秒）
     * @param right_pos  右轮位置（弧度）
     * @param right_vel  右轮速度（弧度/秒）
     */
    void publishJointState(float left_pos, float left_vel,
                           float right_pos, float right_vel);

    /**
     * 获取最新 /cmd_vel
     */
    float cmdLinearX();
    float cmdAngularZ();
    bool hasNewCmd();

    void spinOnce();

private:
    rcl_node_t rcl_node_;
    rcl_publisher_t joint_state_pub_;
    rcl_subscription_t cmd_vel_sub_;
    rclc_executor_t executor_;
    rclc_support_t support_;
    rcl_allocator_t allocator_;

    geometry_msgs__msg__Twist cmd_vel_msg_;
    sensor_msgs__msg__JointState joint_state_msg_;

    static void cmd_vel_callback(const void* msg, void* ctx);
};