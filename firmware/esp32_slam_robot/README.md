# ESP32 SLAM Robot 固件

ESP32 + BMI088 IMU + 双直流电机 + 编码器 + micro-ROS 的家用机器人固件骨架。

## 角色

ESP32 = 实时控制层（PID + 编码器 + IMU + 通信）
ROS2 笔记本 = 决策层（SLAM / 路径规划 / 状态机）

## 系统架构

```
ESP32 (本固件)
├── 电机驱动 (PWM + 方向 GPIO)
├── 编码器读取 (中断 + 计数)
├── IMU 读取 (I2C BMI088)
├── 里程计计算 (差速运动模型)
└── micro-ROS 客户端
    ├── 发布 /odom (nav_msgs/Odometry)
    ├── 发布 /imu/data (sensor_msgs/Imu)
    └── 订阅 /cmd_vel (geometry_msgs/Twist)
            ↓ WiFi UDP
ROS2 笔记本
└── micro-ros-agent (作为 ROS2 节点桥接)
```

## 编译

```bash
# 1. 装 ESP-IDF（一次性）
# 见 https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/get-started/

# 2. 装 micro_ros_espidf_component（一次性）
cd ~/esp
git clone -b humble https://github.com/micro-ROS/micro_ros_espidf_component.git components/micro_ros_espidf_component

# 3. 编译
cd firmware/esp32_slam_robot
idf.py set-target esp32
idf.py menuconfig  # 配置 WiFi SSID/密码 和 micro-ROS agent IP
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

## 桌面端 agent

```bash
# 在笔记本上跑（让 ESP32 能通过 WiFi 发送消息）
ros2 run micro_ros_agent micro_ros_agent udp4 --port 8888
```

## 接线

```
ESP32 DevKitC V4
├── GPIO 25 → L298N IN1（左电机方向）
├── GPIO 26 → L298N IN2（左电机方向）
├── GPIO 32 → L298N ENA（左电机 PWM）
├── GPIO 27 → L298N IN3（右电机方向）
├── GPIO 14 → L298N IN4（右电机方向）
├── GPIO 33 → L298N ENB（右电机 PWM）
├── GPIO 34 → 编码器 A 相（左电机）
├── GPIO 35 → 编码器 B 相（左电机）
├── GPIO 36 → 编码器 A 相（右电机）
├── GPIO 39 → 编码器 B 相（右电机）
├── GPIO 21 → I2C SDA（BMI088）
├── GPIO 22 → I2C SCL（BMI088）
└── VIN    → 12V 降压 5V
```

## 调试

```bash
# 查看 micro-ROS 是否通信成功
ros2 topic list
ros2 topic hz /odom  # 应该看到 ~50 Hz
ros2 topic echo /odom --once
```

## 文件结构

```
esp32_slam_robot/
├── CMakeLists.txt
├── sdkconfig.defaults
├── main/
│   ├── CMakeLists.txt
│   ├── main.cpp           # 入口
│   ├── motor.h/cc         # 电机 PWM 驱动
│   ├── encoder.h/cc       # 编码器读取
│   ├── pid.h/cc           # PID 控制器
│   ├── imu.h/cc           # BMI088 I2C
│   ├── odometry.h/cc      # 差速里程计
│   └── ros2_node.h/cc     # micro-ROS 接口
└── README.md
```

## 已知 TODO

- IMU 偏移校准（开机时静止 1-2 秒取平均）
- 编码器方向反向补偿
- PID 参数（kp / ki / kd）调优
- ESP32 主板是笔电（用 npm/yarn 需要已知 Wi-Fi）—— 你需要 menuconfig 配置 Wi-Fi SSID/密码