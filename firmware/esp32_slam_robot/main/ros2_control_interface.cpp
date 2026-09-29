#include "ros2_control_interface.h"
#include <stdio.h>

bool ROS2ControlInterface::init() {
    allocator_ = rcl_get_default_allocator();
    rclc_support_init(&support_, 0, NULL, &allocator_);
    rclc_node_init_default(&rcl_node_, "esp32_hardware", "", &support_);

    // 发布 joint_states
    rclc_publisher_init_default(
        &joint_state_pub_,
        &rcl_node_,
        ROSIDL_GET_MSG_TYPE_SUPPORT(sensor_msgs, msg, JointState),
        "/joint_states"
    );

    // 订阅 cmd_vel
    rclc_subscription_init_default(
        &cmd_vel_sub_,
        &rcl_node_,
        ROSIDL_GET_MSG_TYPE_SUPPORT(geometry_msgs, msg, Twist),
        "/diff_drive_controller/cmd_vel"  // DiffDriveController 的输出
    );

    rclc_executor_init(&executor_, &support_.context, 1, &allocator_);
    rclc_executor_add_subscription(
        &executor_,
        &cmd_vel_sub_,
        &cmd_vel_msg_,
        &ROS2ControlInterface::cmd_vel_callback,
        ON_NEW_DATA
    );

    return true;
}

void ROS2ControlInterface::publishJointState(float left_pos, float left_vel,
                                              float right_pos, float right_vel) {
    // 构造 JointState 消息
    joint_state_msg_.header.frame_id.data = (char*)"base_link";
    joint_state_msg_.header.frame_id.size = 9;
    joint_state_msg_.name.size = 2;
    joint_state_msg_.name.data = (char**)malloc(2 * sizeof(char*));
    joint_state_msg_.name.data[0] = (char*)"wheel_left_joint";
    joint_state_msg_.name.data[1] = (char*)"wheel_right_joint";
    joint_state_msg_.name.size = 2;

    joint_state_msg_.position.size = 2;
    joint_state_msg_.position.data = (double*)malloc(2 * sizeof(double));
    joint_state_msg_.position.data[0] = left_pos;
    joint_state_msg_.position.data[1] = right_pos;

    joint_state_msg_.velocity.size = 2;
    joint_state_msg_.velocity.data = (double*)malloc(2 * sizeof(double));
    joint_state_msg_.velocity.data[0] = left_vel;
    joint_state_msg_.velocity.data[1] = right_vel;

    rcl_publish(&joint_state_pub_, &joint_state_msg_, NULL);

    // 释放动态分配的内存
    free(joint_state_msg_.name.data);
    free(joint_state_msg_.position.data);
    free(joint_state_msg_.velocity.data);
}

float ROS2ControlInterface::cmdLinearX() {
    return cmd_vel_msg_.linear.x;
}

float ROS2ControlInterface::cmdAngularZ() {
    return cmd_vel_msg_.angular.z;
}

bool ROS2ControlInterface::hasNewCmd() {
    return true;  // 简化处理
}

void ROS2ControlInterface::spinOnce() {
    rclc_executor_spin_some(&executor_, RCL_MS_TO_NS(10));
}

void ROS2ControlInterface::cmd_vel_callback(const void* msg, void* ctx) {
    // 消息内容已经写入 cmd_vel_msg_
    // 不需要额外处理
}