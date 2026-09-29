# 项目状态

> **这是项目最权威的状态文件**。新 Agent 或新会话**先读本文件**，再读 README 和其他 docs。

## 项目一句话

**¥3000 预算 + 3 个月时间 + 67 平米真实家庭环境，做商用级家用全栈机器人**（简历项目，不要发论文）。

## 阶段（3 个月 12 周）

| 月份 | 周 | 工作 | 状态 |
| --- | --- | --- | --- |
| **M1** | W1 | 硬件到货 + 装配 | ⏳ 等待硬件 |
| **M1** | W2 | slam_toolbox 67 平米真机建图 | ⏳ 等待硬件 |
| **M1** | W3 | 切定位 + 调参 + 复测 3 次 | ⏳ 等待硬件 |
| **M1** | W4 | robot_localization 加 IMU/编码器融合 | ⏳ 等待硬件 |
| **M2** | W1 | 底盘驱动 + ESP32 micro-ROS 固件 | 📝 已写骨架 |
| **M2** | W2 | Nav2 全栈 | ⏳ |
| **M2** | W3 | 紧耦合 + 故障注入 | ⏳ |
| **M2** | W4 | 端到端导航 | ⏳ |
| **M3** | W1 | 自主回充桩 | ⏳ |
| **M3** | W2 | 状态机 + 异常恢复 | ⏳ |
| **M3** | W3 | 72h 稳定性 | ⏳ |
| **M3** | W4 | 文档 + 简历 + 视频 | ⏳ |

状态符号：✅ 完成 · 📝 部分完成 · ⏳ 未开始 · 🚧 进行中 · 🚫 阻塞

## 已完成

- [x] ROS2 Humble 桌面版 + slam_toolbox + rplidar_ros + nav2_map_server 全部装好
- [x] `~/ros_ws/src` 工作区建立，软链接到本工程
- [x] colcon build 通过
- [x] fake_scan_publisher + fake_full_mapping.launch.py pipeline 跑通
- [x] 保存 demo 地图 `maps/demo/fake_room_map.{pgm,yaml}`
- [x] 工程移到 `/home/server/code/indoor-slam-67m/`，git init 完成
- [x] ESP32 micro-ROS 固件骨架（ESP-IDF 框架，17 个文件）
- [x] 机器人 URDF（xacro）
- [x] 67 平米测试场景模板

## 待完成（按优先级）

### 阻塞等硬件

- **硬件到货 + 装配**：RPLIDAR A2M8 + 差速底盘 + BMI088 IMU + ESP32 + 12V 电池 + DC-DC
- 用户需要：下单（按 `docs/HARDWARE.md` 清单）

### 用户必做（Agent 帮不了）

- 量 67 平米户型（按 `maps/home_layout/HOME_LAYOUT_TEMPLATE.md`）
- 画户型图
- 选充电桩位置
- 测试 WiFi 覆盖

### 工程上还要做

- PID 参数调优（ESP32 固件里的 kp/ki/kd，现场调）
- IMU 偏移校准
- IMU / 编码器 / 雷达 时间同步
- Nav2 参数（路径规划 + costmap + 行为树）
- 回充桩触点对接
- 异常恢复机制
- 72h 稳定性测试

## 关键技术决策（已经定下来）

| 决策 | 选择 | 理由 |
| --- | --- | --- |
| ROS2 版本 | Humble | Ubuntu 22.04 兼容最好，社区最稳 |
| SLAM 算法 | slam_toolbox | Karto 现代实现，文档全，apt 一键装 |
| 主板 | 笔记本（暂时）| Jetson ¥1900 占用预算 60%，先用笔记本 |
| ESP32 框架 | ESP-IDF | 实时性好，micro-ROS 支持成熟 |
| IMU | BMI088 | 高精度 6 轴，比 MPU6050 好一档 |
| 回充方式 | 触点式 | 简单可靠，¥150-300 成品 |
| 项目路径 | `/home/server/code/indoor-slam-67m/` | 用户指定 |
| 临时禁用 Jetson | 在阶段 1-2 不买 | 预算紧+服务不可用 |

## 用户核心偏好（不能忘）

1. **不要写 SLAM 算法或魔改开源**——用户明说"先做产品，不要写 SLAM"
2. **不要发论文路线**——商用产品优先
4. **简历级项目质量**——3 个月项目要为找工作服务
5. **不要拍脑袋规划**——以工程指标验收（精度 < 10cm、72h 稳定）

## 简历信号（3 个月后要交付的）

```
项目：家用全栈机器人感知与导航系统
- 67 平米真实家庭环境完成端到端 SLAM + 自主导航
- 激光 + IMU + 编码器多传感器紧耦合定位（EKF）
- Nav2 全栈：路径规划 + 动态避障 + 行为树
- ESP32 micro-ROS 自研实时控制固件
- 自主回充桩对接（< 5cm 精度）
- 72 小时连续运行稳定性测试
- 完整开源仓库 + 文档 + 视频
```

## Agent 接手指南

如果你是新 Agent，**按这个顺序读**：

1. `README.md` —— 项目是什么
2. `docs/PROJECT_STATUS.md` —— 当前状态（本文件）
3. `docs/SESSION_NOTES.md` —— 决策背后的理由
4. `docs/HARDWARE.md` —— 硬件清单
5. `docs/WEEK1.md` —— 阶段 1 操作清单
6. `git log --oneline` —— 变更历史
8. 按 PROJECT_STATUS 里的"待完成"继续

**绝对不要**：
- ✗ 改 ROS2 平台（已经在用 Humble）
- ✗ 改 SLAM 算法（已经在用 slam_toolbox）
- ✗ 改 ESP32 框架（已经在用 ESP-IDF）
- ✗ 上 Jetson（用户预算紧，阶段 1-2 不买）