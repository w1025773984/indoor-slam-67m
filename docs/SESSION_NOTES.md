# 会话笔记（决策背后的理由）

> 新 Agent 接手时，看 PROJECT_STATUS 看"是什么"，看本文件看"为什么"。

## 项目起源

用户要做室内 SLAM 项目，"先跑通 67 平米"，**兴趣 + 找工作兼顾**。

最初用户看了一份"豆包"AI 给的"高难度实时增量 SLAM 移动机器人项目说明"，询问是不是真的高难度。我们达成共识：

- 这份说明**不是学术意义上的高难度**（不是发论文级算法）
- 是**工程集成度极高的机器人项目**——把 5+ 个独立技术栈串成一个能跑的系统
- **真正的难点**：组合、调试、稳定、文档，不是算法创新

## 关键决策记录

### 决策 1：先做产品，不写 SLAM 算法

**用户原话**："我只是说我的想法，目前只是先做一个产品出来，而不是写一个slam或者魔改开源"

**理解**：用户明确"工程路线"而非"学术路线"。这意味着：
- 不需要改 slam_toolbox 源码
- 不需要读 slam_toolbox 论文
- 不需要复现 ICP / 图优化算法
- 重心在"组合 + 调通 + 稳定"

### 决策 2：3 个月路线图（¥3000 + 简历 + 商用级）

**用户原话**："预算3000吧，简历，有竞争力，我不要发论文。作出一个能够商用级别的产品最好，时间3个月可以"

**拆解**：
- 预算紧：¥2300 内（不上 Jetson Orin Nano）
- 简历优先：每阶段完成后要有简历信号
- 商用级：用户视角下"能 7×24 跑、能叫它去某个房间、能自主回充"
- 不发论文：纯工程路线

### 决策 3：硬件不上 Jetson

**理由**：
- Jetson Orin Nano Super ¥1900 占预算 60%
- 用笔记本开发 3 个月足够
- 等产品稳定后再考虑嵌入式

**风险**：Jetson 不直接 = 简历少一块。但 ROS2 + 多传感器融合 + Nav2 + 自主回充已足够 Tier 2。

### 决策 4：ESP32 框架用 ESP-IDF

**用户原话**：选"ESP-IDF（推荐）"

**理由**：
- micro-ROS 支持最成熟
- 实时性好（PID + 编码器 + IMU 并发）
- freeRTOS 集成

### 决策 5：回充方式选触点式

**理由**：
- 视觉对接 < 5cm 难做到
- 触点式简单可靠（金属触点 + 弹簧）
- 淘宝 ¥150-300 买成品

### 决策 6：底盘 vs 手持测试

**用户原话**："阶段1我没有机器人，我可以手持假装成机器人测试对吧，在笔记本上看效果对吧"

**理解**：阶段 1 先不接底盘，手持雷达或遥控扫图，把 SLAM 跑通。

## 关于"豆包高难度项目说明"的判断

我们当时拆解了这份说明，给用户的客观评估：

| 说明里的能力 | 真实难度 | 现成方案 |
|---------|---------|---------|
| 实时增量 SLAM | ★★☆☆☆ | slam_toolbox 已经是增量 |
| 多传感器融合（EKF）| ★★★☆☆ | robot_localization 现成 |
| 紧耦合 | ★★★★☆ | 真做要 GTSAM / iSAM |
| Frontier 自主探索 | ★★★☆☆ | exploration 现成包 |
| 动态物体过滤 | ★★★☆☆ | dynablox + 自定义 costmap |

**结论**：大部分是"工程量大"不是"算法创新"。

## 安装过程中的 3 个坑（已修）

1. **rplidar 包名错误**：原本写 `ros-humble-rplidar-ros2`，实际是 `ros-humble-rplidar-ros`（不带 2）
2. **libunwind-dev 依赖**：slam_toolbox 编译需要先装这个
3. **目录命名不匹配**：colcon 用下划线版本 `indoor_slam_67m`，需要软链接对齐

## 项目路径变化

- 最初：`/home/server/.proma/agent-workspaces/default/workspace-files/work/indoor-slam-67m`
- 用户要求移到 `/home/server/code/`
- 最终：`/home/server/code/indoor-slam-67m/`

## 系统环境

- OS: Ubuntu 22.04.5 LTS
- ROS2: Humble Hawksbill（285 个包）
- 内核：6.8.0-124-generic
- 用户：server（sudo 配置 NOPASSWD，方便 Agent 执行 apt）

## sudo NOPASSWD 配置

```bash
echo "server ALL=(ALL) NOPASSWD: ALL" | sudo tee /etc/sudoers.d/90-proma-nopasswd
sudo chmod 440 /etc/sudoers.d/90-proma-nopasswd
```

**只在 Proma Agent 上下文有意义**——这是给 Agent 用的，不是用户家需要的。

## 工作区结构

```
~/ros_ws/src/
└── indoor_slam_67m → /home/server/code/indoor-slam-67m/
```

这是 colcon 工作区，软链接指向实际工程。

## 何时需要更新本文件

每当你（Agent）做了重要决策、新增依赖、改方向、推翻之前计划时，**更新本文件**。

特别需要更新的事件类型：
- 更换 SLAM 算法
- 更换 IMU / 底盘型号
- 用户改预算或时间
- 阶段重新划分
- 用户改偏好（"先做产品" → "现在想做学术"等）
- 出现重大阻塞