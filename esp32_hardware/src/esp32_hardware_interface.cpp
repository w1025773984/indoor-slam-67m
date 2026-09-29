#include "esp32_hardware/esp32_hardware_interface.hpp"

#include <chrono>
#include <cstring>

#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "pluginlib/class_list_macros.hpp"
#include "rclcpp/rclcpp.hpp"

namespace esp32_hardware
{

CallbackReturn ESP32HardwareInterface::on_init(const hardware_interface::HardwareInfo & info)
{
  if (hardware_interface::SystemInterface::on_init(info) != CallbackReturn::SUCCESS) {
    return CallbackReturn::ERROR;
  }

  // 解析 joint 名
  for (const auto & joint : info.joints) {
    joint_names_.push_back(joint.name);
    RCLCPP_INFO(rclcpp::get_logger("ESP32HardwareInterface"),
                "Joint: %s", joint.name.c_str());
  }

  // 解析 parameters（wheel_radius / wheel_separation）
  for (const auto & param : info.hardware_parameters) {
    if (param.first == "wheel_radius") {
      wheel_radius_ = std::stod(param.second);
    } else if (param.first == "wheel_separation") {
      wheel_separation_ = std::stod(param.second);
    }
  }

  RCLCPP_INFO(rclcpp::get_logger("ESP32HardwareInterface"),
              "wheel_radius=%.3f m, wheel_separation=%.3f m",
              wheel_radius_, wheel_separation_);

  // 状态/命令大小
  hw_pos_states_.resize(joint_names_.size(), 0.0);
  hw_vel_states_.resize(joint_names_.size(), 0.0);
  hw_vel_commands_.resize(joint_names_.size(), 0.0);

  return CallbackReturn::SUCCESS;
}

CallbackReturn ESP32HardwareInterface::on_configure(const rclcpp_lifecycle::State & /*previous_state*/)
{
  RCLCPP_INFO(rclcpp::get_logger("ESP32HardwareInterface"), "Configuring...");

  // 创建节点（不作为 LifecycleNode，由 controller_manager 管理）
  node_ = std::make_shared<rclcpp::Node>("esp32_hardware_interface");

  joint_state_sub_ = node_->create_subscription<sensor_msgs::msg::JointState>(
    "/joint_states", rclcpp::SensorDataQoS(),
    [this](const sensor_msgs::msg::JointState::SharedPtr msg) {
      latest_joint_state_ = *msg;
      has_joint_state_ = true;

      // 建立 joint_name -> index 映射（首次）
      if (joint_name_to_index_.empty()) {
        for (size_t i = 0; i < msg->name.size(); i++) {
          joint_name_to_index_[msg->name[i]] = i;
        }
        RCLCPP_INFO(rclcpp::get_logger("ESP32HardwareInterface"),
                    "Joint name mapping established, %zu joints",
                    msg->name.size());
      }
    });

  cmd_vel_pub_ = node_->create_publisher<geometry_msgs::msg::Twist>(
    "/diff_drive_controller/cmd_vel", 10);

  return CallbackReturn::SUCCESS;
}

CallbackReturn ESP32HardwareInterface::on_activate(const rclcpp_lifecycle::State & /*previous_state*/)
{
  RCLCPP_INFO(rclcpp::get_logger("ESP32HardwareInterface"),
              "Activating... waiting for first /joint_states message");
  return CallbackReturn::SUCCESS;
}

CallbackReturn ESP32HardwareInterface::on_deactivate(const rclcpp_lifecycle::State & /*previous_state*/)
{
  RCLCPP_INFO(rclcpp::get_logger("ESP32HardwareInterface"), "Deactivating...");
  return CallbackReturn::SUCCESS;
}

std::vector<hardware_interface::StateInterface> ESP32HardwareInterface::export_state_interfaces()
{
  std::vector<hardware_interface::StateInterface> states;

  for (size_t i = 0; i < joint_names_.size(); i++) {
    states.emplace_back(joint_names_[i],
                        hardware_interface::HW_IF_POSITION,
                        &hw_pos_states_[i]);
    states.emplace_back(joint_names_[i],
                        hardware_interface::HW_IF_VELOCITY,
                        &hw_vel_states_[i]);
  }

  return states;
}

std::vector<hardware_interface::CommandInterface> ESP32HardwareInterface::export_command_interfaces()
{
  std::vector<hardware_interface::CommandInterface> commands;

  for (size_t i = 0; i < joint_names_.size(); i++) {
    commands.emplace_back(joint_names_[i],
                          hardware_interface::HW_IF_VELOCITY,
                          &hw_vel_commands_[i]);
  }

  return commands;
}

hardware_interface::return_type ESP32HardwareInterface::read(
  const rclcpp::Time & /*time*/,
  const rclcpp::Duration & /*period*/)
{
  if (!has_joint_state_) {
    // 还没收到数据，返回 OK 让 controller 知道状态
    return hardware_interface::return_type::OK;
  }

  // 把 latest_joint_state_ 复制到 hw_pos_states_ / hw_vel_states_
  for (size_t i = 0; i < joint_names_.size(); i++) {
    auto it = joint_name_to_index_.find(joint_names_[i]);
    if (it == joint_name_to_index_.end()) {
      RCLCPP_WARN_THROTTLE(rclcpp::get_logger("ESP32HardwareInterface"),
                            *node_->get_clock(), 1000,
                            "Joint %s not in /joint_states",
                            joint_names_[i].c_str());
      continue;
    }

    size_t idx = it->second;
    if (idx < latest_joint_state_.position.size()) {
      hw_pos_states_[i] = latest_joint_state_.position[idx];
    }
    if (idx < latest_joint_state_.velocity.size()) {
      hw_vel_states_[i] = latest_joint_state_.velocity[idx];
    }
  }

  return hardware_interface::return_type::OK;
}

hardware_interface::return_type ESP32HardwareInterface::write(
  const rclcpp::Time & /*time*/,
  const rclcpp::Duration & /*period*/)
{
  // 差速底盘逆运动学
  // joint 顺序：[left_wheel, right_wheel]
  if (joint_names_.size() != 2) {
    RCLCPP_ERROR_THROTTLE(rclcpp::get_logger("ESP32HardwareInterface"),
                          *node_->get_clock(), 1000,
                          "Expected 2 joints, got %zu",
                          joint_names_.size());
    return hardware_interface::return_type::ERROR;
  }

  double v_left = hw_vel_commands_[0] * wheel_radius_;
  double v_right = hw_vel_commands_[1] * wheel_radius_;

  // 差速运动学正解（wheel velocity -> twist）
  double linear_x = (v_left + v_right) / 2.0;
  double angular_z = (v_right - v_left) / wheel_separation_;

  auto twist = geometry_msgs::msg::Twist();
  twist.linear.x = linear_x;
  twist.angular.z = angular_z;

  cmd_vel_pub_->publish(twist);

  return hardware_interface::return_type::OK;
}

}  // namespace esp32_hardware

PLUGINLIB_EXPORT_CLASS(esp32_hardware::ESP32HardwareInterface, hardware_interface::SystemInterface)