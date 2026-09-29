# Architecture Brief

> 借鉴 [OOMWOO](https://github.com/makerspet/oomwoo) 的架构思路。
> 定义系统边界，让模块可以独立开发和替换。

## 1. 设计原则

- **CPU + MCU 分离**：决策层（ROS2 / slam_toolbox / Nav2）跑在笔记本；实时控制层（PID / 编码器 / 安全）跑在 ESP32
- **Safety 不依赖 ROS2**：急停 / bumper / 低电量在 ESP32 上实现，ROS2 挂了机器人也能急停
- **模块化**：每个模块有清晰接口，可以独立替换实现
- **Simulation-first**：算法先用 fake 数据验证，再上硬件

## 2. 系统框图

```
┌─────────────────────────────────────────────────────┐
│ 笔记本（决策层）                                       │
│                                                      │
│  ┌──────────────────────────────────────────────┐   │
│  │ ROS2 Humble + Nav2 + slam_toolbox            │   │
│  ├──────────────────────────────────────────────┤   │
│  │  • slam_toolbox（建图 + 定位）               │   │
│  │  • robot_localization（多源 EKF 融合）       │   │
│  │  • Nav2（路径规划 + 避障 + 行为树）          │   │
│  │  • ros2_control（DiffDriveController + PID） │   │
│  │  • 状态机 + 异常恢复                         │   │
│  └──────────────────────────────────────────────┘   │
│                       ↓                                           │
│  ┌──────────────────────────────────────────────┐   │
│  │ ESP32HardwareInterface (ros2_control)       │   │
│  │  • 订阅 /joint_states                         │   │
│  │  • 发布 /diff_drive_controller/cmd_vel       │   │
│  └──────────────────────────────────────────────┘   │
└─────────────────┬─────────────────────────────────────┘
                  │ WiFi UDP（micro-ROS）
                  ↓
┌─────────────────────────────────────────────────────┐
│ ESP32（实时控制层）                                       │
│                                                      │
│  ┌──────────────────────────────────────────────┐   │
│  │ micro-ROS client                              │   │
│  ├──────────────────────────────────────────────┤   │
│  │  • 电机 PWM（LEDC）+ 方向 GPIO               │   │
│  │  • 编码器读取（PCNT 4 倍频）                  │   │
│  │  • IMU 读取（BMI088 I2C）                     │   │
│  │  • Safety：急停 / bumper / 低电量（独立模块）│   │
│  │  • 透明硬件抽象（PID 跑在 ROS2）             │   │
│  └──────────────────────────────────────────────┘   │
└─────────────────┬─────────────────────────────────────┘
                  │ 12V 锂电池
                  ↓
┌─────────────────────────────────────────────────────┐
│ 电机 + 编码器 + 雷达 + IMU + bumper + 电池             │
└─────────────────────────────────────────────────────┘
```

## 3. 模块清单（借鉴 Oomwoo contributions/）

我们把功能拆成模块，每个模块有：
- 文档：模块做什么、接口是什么
- 代码：在哪里
- 测试：怎么验证

| 模块 | 路径 | 状态 | 阶段 |
| --- | --- | --- | --- |
| **硬件抽象** | `urdf/` | ✅ | M1 |
| **SLAM 建图** | `config/slam_toolbox_mapping.yaml` | ✅ | M1 |
| **SLAM 定位** | `config/slam_toolbox_localization.yaml` | ✅ | M1 |
| **ros2_control 集成** | `esp32_hardware/` + `launch/ros2_control.launch.py` | ✅ | M2 |
| **底盘驱动（ESP32 v1）** | `firmware/main/motor.cpp` 等 | ✅ | M2 |
| **透明硬件（ESP32 v2/v3）** | `firmware/main/ros2_control_interface.cpp` 等 | ✅ | M2 |
| **Safety (ESP32 v3)** | `firmware/main/safety.cpp` | ✅ | M2 |
| **多传感器融合** | `config/`（待加 ekf.yaml） | ⏳ | M1-W4 |
| **Nav2 集成** | `launch/`（待加） | ⏳ | M2-W2 |
| **自主回充** | 待开发 | ⏳ | M3-W1 |
| **状态机 + 异常恢复** | 待开发 | ⏳ | M3-W2 |
| **72h 稳定性测试** | 待开发 | ⏳ | M3-W3 |

## 4. 安全模型

借鉴 Oomwoo：safety 跑在 MCU，不依赖 ROS2。

| 触发条件 | 检测位置 | 反应 |
| --- | --- | --- |
| 急停按钮按下 | ESP32 GPIO | 立即停电机 |
| Bumper 碰撞 | ESP32 GPIO | 立即停电机 |
| 电池 < 10.5V | ESP32 ADC | 警告（继续跑） |
| 电池 < 9.0V | ESP32 ADC | 强制停 |
| ROS2 失联 | ESP32 WiFi timeout | 持续拒绝执行新命令 |

## 5. 数据流

```
LiDAR → /scan
     │
     ▼
slam_toolbox ─→ /map + /pose
     │
     ▼
robot_localization ← /imu/data (ESP32) + /wheel_odom (ESP32) + /pose (slam_toolbox)
     │              ↓
     └──────→ /odom_filtered
                │
                ▼
Nav2 (global + local costmap + planner + controller)
     │
     ▼
/cmd_vel_nav
     │
     ▼
DiffDriveController (PID 跑在 ROS2)
     │
     ▼
/diff_drive_controller/cmd_vel
     │
     ▼
ESP32 → 电机 PWM
```

## 6. 借鉴来源

- [OOMWOO](https://github.com/makerspet/oomwoo) —— 11k stars 开源扫地机
- [oomwoo-one](https://github.com/makerspet/oomwoo-one) —— ROS2 robot description 模板
- [Nav2](https://docs.nav2.org/) —— ROS2 官方导航栈
- [ros2_control](https://github.com/ros-controls/ros2_control) —— ROS2 硬件抽象标准
- [robot_localization](https://github.com/cra-ros-pkg/robot_localization) —— 多源融合 EKF

## 7. 设计上的差异（与 Oomwoo 对比）

| 维度 | Oomwoo | 我们 |
| --- | --- | --- |
| 主板 | Raspberry Pi CM4/CM5 | 笔记本（暂） |
| 下位机 | STM32G473 | ESP32 + micro-ROS |
| 通信 | Serial（自定协议） | WiFi UDP + micro-ROS |
| 仿真 | Gazebo（完整） | fake_scan_publisher |
| Home Assistant | ✅ | ❌（暂不做） |
| 3D 打印 | ✅ 完整 chassis | ❌（预算紧） |
| 时间 | 长期社区开发 | 3 个月 |

我们聚焦在**算法 + 集成**，不做工业级优化。