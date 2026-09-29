#pragma once
/**
 * micro-ROS 节点封装
 * - 发布 /odom (nav_msgs/msg/Odometry)
 * - 发布 /imu/data (sensor_msgs/msg/Imu)
 * - 订阅 /cmd_vel (geometry_msgs/msg/Twist)
 */

#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <geometry_msgs/msg/twist.h>
#include <nav_msgs/msg/odometry.h>
#include <sensor_msgs/msg/imu.h>

struct TwistCommand {
    float linear_x;   // m/s
    float angular_z;  // rad/s
};

class ROS2Node {
public:
    bool init();
    void spinOnce();

    // 发布接口
    void publishOdometry(float x, float y, float theta,
                         float v, float omega,
                         float v_left, float v_right);
    void publishIMU(float ax, float ay, float az,
                    float gx, float gy, float gz);

    // 获取最新命令
    TwistCommand getCmdVel();

private:
    rcl_node_t rcl_node_;
    rcl_publisher_t odom_pub_;
    rcl_publisher_t imu_pub_;
    rcl_subscription_t cmd_vel_sub_;

    rclc_executor_t executor_;
    rcl_allocator_t allocator_;
    rclc_support_t support_;

    TwistCommand last_cmd_;
    geometry_msgs__msg__Twist cmd_vel_msg_;

    static void cmd_vel_callback(const void* msg, void* ctx);
};