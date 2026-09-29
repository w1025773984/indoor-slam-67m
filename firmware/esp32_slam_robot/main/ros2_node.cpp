#include "ros2_node.h"
#include <rclc/publisher.h>
#include <rclc/subscription.h>
#include <stdio.h>

ROS2Node::ROS2Node() : last_cmd_{0.0f, 0.0f} {}

bool ROS2Node::init() {
    allocator_ = rcl_get_default_allocator();
    rclc_support_init(&support_, 0, NULL, &allocator_);

    rclc_node_init_default(&rcl_node_, "esp32_robot", "", &support_);

    // 发布器
    rclc_publisher_init_default(
        &odom_pub_,
        &rcl_node_,
        ROSIDL_GET_MSG_TYPE_SUPPORT(nav_msgs, msg, Odometry),
        "/odom"
    );
    rclc_publisher_init_default(
        &imu_pub_,
        &rcl_node_,
        ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, Imu),
        "/imu/data"
    );

    // 订阅器
    rclc_subscription_init_default(
        &cmd_vel_sub_,
        &rcl_node_,
        ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Twist),
        "/cmd_vel"
    );

    // 执行器
    rclc_executor_init(&executor_, &support_.context, 1, &allocator_);
    rclc_executor_add_subscription(
        &executor_,
        &cmd_vel_sub_,
        &cmd_vel_msg_,
        &ROS2Node::cmd_vel_callback,
        ON_NEW_DATA
    );

    last_cmd_ = {0.0f, 0.0f};
    return true;
}

void ROS2Node::spinOnce() {
    rclc_executor_spin_some(&executor_, RCL_MS_TO_NS(10));
}

void ROS2Node::publishOdometry(float x, float y, float theta,
                                float v, float omega,
                                float v_left, float v_right) {
    nav_msgs__msg__Odometry msg = {};

    // Header
    msg.header.stamp.sec = 0;
    msg.header.stamp.nanosec = 0;
    msg.header.frame_id.data = (char*)"odom";
    msg.header.frame_id.size = 4;

    // 子坐标系
    msg.child_frame_id.data = (char*)"base_link";
    msg.child_frame_id.size = 9;

    // 位置
    msg.pose.pose.position.x = x;
    msg.pose.pose.position.y = y;
    msg.pose.pose.position.z = 0;

    // 朝向（四元数，从 theta 转）
    msg.pose.pose.orientation.w = cos(theta / 2);
    msg.pose.pose.orientation.x = 0;
    msg.pose.pose.orientation.y = 0;
    msg.pose.pose.orientation.z = sin(theta / 2);

    // 速度
    msg.twist.twist.linear.x = v;
    msg.twist.twist.angular.z = omega;

    rcl_publish(&odom_pub_, &msg, NULL);
}

void ROS2Node::publishIMU(float ax, float ay, float az,
                           float gx, float gy, float gz) {
    sensor_msgs__msg__Imu msg = {};

    msg.header.frame_id.data = (char*)"imu_link";
    msg.header.frame_id.size = 8;

    msg.linear_acceleration.x = ax;
    msg.linear_acceleration.y = ay;
    msg.linear_acceleration.z = az;

    msg.angular_velocity.x = gx;
    msg.angular_velocity.y = gy;
    msg.angular_velocity.z = gz;

    msg.orientation.w = 1.0;  // 0 不合法
    msg.orientation_covariance[0] = -1;  // 标 -1 表示无朝向估计

    rcl_publish(&imu_pub_, &msg, NULL);
}

TwistCommand ROS2Node::getCmdVel() {
    return last_cmd_;
}

void ROS2Node::cmd_vel_callback(const void* msg, void* ctx) {
    const geometry_msgs__msg__Twist* twist = (const geometry_msgs__msg__Twist*)msg;
    ROS2Node* self = (ROS2Node*)ctx;
    self->last_cmd_.linear_x = twist->linear.x;
    self->last_cmd_.angular_z = twist->angular.z;
}