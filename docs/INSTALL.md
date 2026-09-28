# Ubuntu 22.04 + ROS2 Humble 安装 SOP

## 系统要求

- **OS**: Ubuntu 22.04 LTS（Jammy Jellyfish，桌面或服务器都行）
- **CPU**: 4 核起步，slam_toolbox 是 CPU 任务，CPU 越强越好
- **RAM**: 8 GB 起步，16 GB 推荐（RViz 占用不少）
- **磁盘**: 至少 20 GB 空闲
- **USB**: 至少 1 个空闲 USB 口（雷达用）

> ⚠️ 不要装 Ubuntu 24.04 / 20.04。ROS2 Humble 的目标是 22.04，其他版本要源码编译大量包，浪费时间。

## 步骤 1：装 Ubuntu 22.04

如果笔记本已经有别的系统，建议：

- **选项 A**：双系统（推荐）。最稳，性能不打折。
- **选项 B**：用 VMware / VirtualBox 虚拟机。性能损失 15–30%，67 平米扫一圈时间从 5 分钟变 7 分钟，问题不大，但 USB 直通配置会折腾。
- **选项 C**：双系统装在移动 SSD，必要时插别的电脑用。最灵活。

## 步骤 2：一键安装 ROS2 Humble

打开终端：

```bash
# 1. 配 locale
sudo apt update && sudo apt install -y locales
sudo locale-gen en_US en_US.UTF-8
sudo update-locale LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8
export LANG=en_US.UTF-8

# 2. 加 ROS2 apt 源（使用清华镜像，下载快）
sudo apt install -y software-properties-common
sudo add-apt-repository -y universe
sudo apt update && sudo apt install -y curl gnupg lsb-release

sudo curl -sSL https://mirrors.tuna.tsinghua.edu.cn/rosdistro/ros.key \
  -o /usr/share/keyrings/ros-archive-keyring.gpg
echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/ros-archive-keyring.gpg] \
  https://mirrors.tuna.tsinghua.edu.cn/ros2/ubuntu jammy main" | \
  sudo tee /etc/apt/sources.list.d/ros2.list > /dev/null

# 3. 装 ROS2 Humble 桌面版（带 RViz2）
sudo apt update
sudo apt install -y ros-humble-desktop ros-humble-ros-base

# 4. 装开发工具（colcon、rosdep）
sudo apt install -y python3-colcon-common-extensions python3-rosdep
sudo rosdep init
rosdep update

# 5. 加环境变量到 ~/.bashrc
echo "source /opt/ros/humble/setup.bash" >> ~/.bashrc
source ~/.bashrc
```

验证：

```bash
ros2 --version
# 应该输出：ros2 cli version: 0.18.x（humble）

ros2 run demo_nodes_cpp talker
# 新开终端
ros2 run demo_nodes_cpp listener
# 应该看到消息收发
```

## 步骤 3：装 SLAM 必备包

```bash
# slam_toolbox（建图 + 定位核心）
sudo apt install -y ros-humble-slam-toolbox

# RPLIDAR 驱动（思岚官方）
sudo apt install -y ros-humble-rplidar-ros2
# 备选：从源码编译（如果 apt 找不到）
# mkdir -p ~/ros_ws/src && cd ~/ros_ws/src
# git clone https://github.com/Slamtec/rplidar_ros2.git
# cd ~/ros_ws && rosdep install --from-paths src --ignore-src -r -y
# colcon build --symlink-install

# 键盘遥控
sudo apt install -y ros-humble-teleop-twist-keyboard

# 地图保存工具（map_server）
sudo apt install -y ros-humble-nav2-map-server

# tf2 工具（看 tf 树）
sudo apt install -y ros-humble-tf2-tools ros-humble-tf-transformations
```

## 步骤 4：建 colcon 工作区

```bash
mkdir -p ~/ros_ws/src
cd ~/ros_ws/src

# 把本项目作为工作区下的子目录软链接进来
ln -s /home/server/code/indoor-slam-67m indoor-slam-67m

# 或拷贝（如果你想脱离 git 管理）
# cp -r /path/to/indoor-slam-67m .

cd ~/ros_ws
rosdep install --from-paths src --ignore-src -r -y
colcon build --symlink-install

echo "source ~/ros_ws/install/setup.bash" >> ~/.bashrc
source ~/.bashrc
```

## 步骤 5：配 udev 规则

```bash
cd ~/ros_ws/src/indoor-slam-67m
chmod +x scripts/setup_udev.sh
./scripts/setup_udev.sh

# 拔掉 USB 再插一次
ls -la /dev/rplidar
# 应该看到 /dev/rplidar -> ttyUSB0
```

## 步骤 6：验证

```bash
# 雷达是否被识别？
ls -la /dev/rplidar

# 启动雷达
ros2 launch indoor-slam-67m rplidar.launch.py

# 另开终端，看 /scan topic
ros2 topic list
# 应该看到 /scan

ros2 topic hz /scan
# 应该看到 5–10 Hz

ros2 topic echo /scan --once
# 应该看到 sensor_msgs/LaserScan
```

看到 `/scan` 在 RViz2 显示激光线，说明硬件 + 软件栈通了，可以进入建图阶段。

## 阶段 1 不需要的包

阶段 1 不需要装：

- ros-humble-navigation2（阶段 2 才用）
- ros-humble-cartographer（和 slam_toolbox 二选一，阶段 1 用 slam_toolbox）
- ros-humble-gmapping（过时了，不推荐）
- 任何深度相机驱动（阶段 1 不用）

## 常见安装错误

| 错误 | 解决 |
| --- | --- |
| `rosdep: command not found` | `sudo apt install -y python3-rosdep` |
| `locale` 警告 | 重新执行步骤 2-1 |
| 下载超时 | 已用清华镜像，应该不会；如果是，再换一个或挂代理 |
| slam_toolbox 找不到 | `apt-cache search slam-toolbox` 确认包名，可能要从源码装 |
| rplidar 启动报 `cannot open port` | udev 没配好，重启脚本后再插 USB |