# Indoor SLAM @ 67m²

室内 67 平米家用 SLAM 项目，从零跑通建图 + 实时定位。

## 目标

在 67 平米家用场景跑通：
1. **建图**：扫一圈产出 2D 占用栅格地图（PGM + YAML）
2. **定位**：基于保存的地图实时输出机器人在地图中的位姿
3. **后续**：接入 Nav2 做自主导航（阶段 2）

## 阶段规划（3 个月商用级，¥2300 总预算）

| 月份 | 内容 | 预算 | 状态 |
| --- | --- | --- | --- |
| **1. 基础 + 多传感器** | 真机建图 + 定位 + IMU/编码器融合 | ¥2300 | 未开始 |
| **2. 自主机器人** | 底盘驱动 + Nav2 + 端到端导航 | 0 | 未开始 |
| **3. 商用级产品化** | 回充 + 异常恢复 + 72h 测试 + 文档 | 0 | 未开始 |

详细周规划见 TODO 表 / 各文档。

## 技术栈

- **OS**: Ubuntu 22.04 LTS
- **ROS**: ROS2 Humble（不要用 Foxy / Rolling）
- **SLAM**: slam_toolbox（基于 Karto / 图优化，mapping + localization 双模式）
- **雷达驱动**: rplidar_ros2（思岚官方）
- **可视化**: RViz2
- **遥控**: teleop_twist_keyboard
- **下位机（备用）**: ESP32 DevKit（阶段 1 暂不接底盘）

## 目录结构

```
indoor-slam-67m/
├── README.md               # 本文件
├── docs/                   # 文档
│   ├── HARDWARE.md         # 硬件清单 + 接线
│   ├── INSTALL.md          # Ubuntu + ROS2 一键安装 SOP
│   ├── WEEK1.md            # 第一周操作清单
│   └── PITFALLS.md         # 67 平米家用踩坑
├── launch/                 # ROS2 launch 文件
│   ├── rplidar.launch.py
│   ├── slam_mapping.launch.py
│   ├── slam_localization.launch.py
│   ├── view_rviz.launch.py
│   ├── full_mapping.launch.py
│   └── full_localization.launch.py
├── config/                 # 参数文件
│   ├── rplidar.yaml
│   ├── slam_toolbox_mapping.yaml
│   └── slam_toolbox_localization.yaml
├── maps/                   # 建图保存位置
├── scripts/                # 运维脚本
│   ├── setup_udev.sh
│   └── save_map.sh
├── hardware/               # 接线文档 + 实物图占位
└── logs/                   # 运行日志
```

## 快速开始（TL;DR）

```bash
# 1. 系统准备
./scripts/setup_udev.sh

# 2. 一键启动建图
ros2 launch full_mapping.launch.py

# 3. 扫一圈后保存地图
./scripts/save_map.sh home_67m
```

详细步骤见 `docs/WEEK1.md`。

## 当前状态

- [x] 硬件选型确认
- [x] 项目骨架建立
- [ ] 硬件到货
- [ ] Ubuntu + ROS2 装好
- [ ] RPLIDAR A2 在 RViz 看到 /scan
- [ ] slam_toolbox mapping 模式跑通
- [ ] 67 平米地图保存
- [ ] slam_toolbox localization 模式跑通