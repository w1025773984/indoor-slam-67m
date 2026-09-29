#pragma once

#include <memory>
#include <string>
#include <vector>

#include "hardware_interface/handle.hpp"
#include "hardware_interface/hardware_info.hpp"
#include "hardware_interface/system_interface.hpp"
#include "hardware_interface/types/hardware_interface_return_values.hpp"

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/state.hpp"
#include "sensor_msgs/msg/joint_state.hpp"
#include "geometry_msgs/msg/twist.hpp"

namespace esp32_hardware
{

/**
 * ESP32 Hardware Interface for ros2_control
 *
 * 数据流：
 * - 输入：/joint_states（ESP32 v2 firmware 发）
 * - 输出：/diff_drive_controller/cmd_vel（ESP32 v2 firmware 收）
 *
 * DiffDriveController 通过本接口：
 * - read()  拿到关节位置 + 速度
 * - write() 设置关节速度命令
 * - 这里 read/write 转换为 ROS topic 通信
 */
class ESP32HardwareInterface : public hardware_interface::SystemInterface
{
public:
  RCLCPP_SHARED_PTR_DEFINITIONS(ESP32HardwareInterface)

  CallbackReturn on_init(const hardware_interface::HardwareInfo & info) override;

  CallbackReturn on_configure(const rclcpp_lifecycle::State & previous_state) override;

  CallbackReturn on_activate(const rclcpp_lifecycle::State & previous_state) override;

  CallbackReturn on_deactivate(const rclcpp_lifecycle::State & previous_state) override;

  std::vector<hardware_interface::StateInterface> export_state_interfaces() override;

  std::vector<hardware_interface::CommandInterface> export_command_interfaces() override;

  hardware_interface::return_type read(
    const rclcpp::Time & time,
    const rclcpp::Duration & period) override;

  hardware_interface::return_type write(
    const rclcpp::Time & time,
    const rclcpp::Duration & period) override;

private:
  // 关节参数（从 hardware_info 解析）
  std::vector<std::string> joint_names_;
  double wheel_radius_{0.06};   // 米
  double wheel_separation_{0.30}; // 米

  // 状态接口（位置 + 速度）
  std::vector<double> hw_pos_states_;
  std::vector<double> hw_vel_states_;

  // 命令接口（速度）
  std::vector<double> hw_vel_commands_;

  // ROS2 节点
  rclcpp::Node::SharedPtr node_;

  // 订阅 /joint_states
  rclcpp::Subscription<sensor_msgs::msg::JointState>::SharedPtr joint_state_sub_;

  // 发布 /diff_drive_controller/cmd_vel
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_pub_;

  // 最新接收的关节状态（topic 异步，read 时拿最新值）
  sensor_msgs::msg::JointState latest_joint_state_;
  bool has_joint_state_{false};

  // 关节名称到 latest_joint_state 索引的映射
  std::map<std::string, int> joint_name_to_index_;
};

}  // namespace esp32_hardware