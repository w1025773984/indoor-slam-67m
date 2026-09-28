# 第一周操作清单（Day 1–7）

按天执行，每天下班/周末 2–3 小时。

---

## Day 1：Ubuntu 22.04 + ROS2 Humble 装好

### 动作
1. 备份笔记本数据，装 Ubuntu 22.04 LTS 双系统
2. 按 `docs/INSTALL.md` 步骤 2–3 装 ROS2 Humble + SLAM 必备包
3. 验证：`ros2 run demo_nodes_cpp talker` + listener 跑通

### 验收
- [ ] `ros2 --version` 输出 humble
- [ ] demo_nodes_cpp talker / listener 收发正常
- [ ] `ros2 run rviz2 rviz2` 能打开 RViz2

### 踩坑预期
- 装系统卡住 → 多半是显卡驱动问题，先选"安全图形模式"启动再装 NVIDIA 驱动
- apt 装不上 → 切国内镜像（已配清华源）

---

## Day 2：硬件到位 + 雷达接入

### 动作
1. 硬件到货：RPLIDAR A2 + ESP32
2. 插 USB，执行 `scripts/setup_udev.sh`
3. 启动雷达：`ros2 launch indoor-slam-67m rplidar.launch.py`
4. RViz2 里添加 LaserScan（topic = `/scan`，fixed frame = `laser`），看到实时激光线

### 验收
- [ ] `ls -la /dev/rplidar` 看到设备
- [ ] `/scan` topic 频率 5–10 Hz
- [ ] RViz2 中能看到激光线随雷达旋转

### 踩坑预期
- 雷达只扫一个点 → `rplidar_ros2` 节点没启动成功，看日志 `ros2 run rplidar_ros2 rplidar_node --ros-args -p serial_port:=/dev/rplidar`
- 雷达转但不输出 → 串口权限问题，确认 udev 已生效
- RViz 不显示激光线 → fixed frame 改成 `laser` 而不是 `world`

---

## Day 3–4：slam_toolbox 建图跑通

### 动作
1. 启动建图：`ros2 launch indoor-slam-67m full_mapping.launch.py`
2. 用键盘遥控（或手持雷达慢慢走）：
   ```bash
   ros2 run teleop_twist_keyboard teleop_twist_keyboard \
     --ros-args -r /cmd_vel:=/cmd_vel
   ```
3. 把 67 平米每个房间 + 走廊都走一遍，回到起点
4. RViz2 中能看到 `/map` 实时增长
5. 走完后执行：`./scripts/save_map.sh home_67m`

### 验收
- [ ] `/map` topic 实时输出
- [ ] RViz2 中能看到地图逐渐成型
- [ ] 走回起点后，回环成功（地图不漂移）
- [ ] 保存的 `maps/home_67m.pgm` + `home_67m.yaml` 存在

### 关键动作要点
- **速度要慢**：`/cmd_vel` linear.x ≤ 0.2 m/s，angular.z ≤ 0.3 rad/s
- **贴墙走**：先沿墙根走一圈建立外轮廓，再补内部
- **多走几次**：每个房间来回走 2–3 遍，slam_toolbox 用 Karto 建图，重复扫描能强化回环
- **避免抖动**：手持时身体稳一点，平放在小车/三脚架上更稳

### 踩坑预期
- 地图扭曲 / 错位 → 回环失败，多走几遍
- 玻璃墙处地图穿过去 → 关门或贴磨砂膜
- 黑色家具附近地图缺一块 → 多走两遍
- 保存地图时报错 → `map_server` 没装（已在 INSTALL.md 步骤 3 装好）

---

## Day 5：切到 localization 模式

### 动作
1. 关闭建图 launch
2. 把 `home_67m.pgm` + `home_67m.yaml` 放到 `maps/` 目录
3. 启动定位：`ros2 launch indoor-slam-67m full_localization.launch.py`
4. 在 RViz2 里手动给定初始位姿（2D Pose Estimate 按钮），让机器人"开机即知道我在哪"
5. 缓慢走动，看 `/pose` 实时更新

### 验收
- [ ] 启动后地图立刻出现
- [ ] 给定初始位姿后，机器人位置实时跟踪
- [ ] 走回起点附近能识别到（位置不漂）

### 踩坑预期
- 给定初始位姿后机器人位置不更新 → slam_toolbox 配置 `mode: localization`，确认参数没切到 mapping
- 初始位姿差太多 → 慢慢走近起点（2 m 内）再手动定位，雷达对环境的初次匹配范围有限

---

## Day 6：复测 + 调参

### 动作
1. 关掉家里所有的门、各房间单独再扫一次（验完整性）
2. 看 `config/slam_toolbox_mapping.yaml` 参数（最小角度、阈值等），67 平米的家用参数已经调好
3. 跑 3 遍完整流程，看地图是否一致

### 验收
- [ ] 3 遍扫出的地图重合度 > 95%
- [ ] 地图边缘清晰（墙、门、家具大致轮廓可见）

---

## Day 7：阶段总结 + 评估下一步

### 动作
1. 把 `maps/home_67m.*` 备份到 `maps/` 的版本目录（如 `maps/v1/`）
2. 写一段笔记记录：地图质量、回环效果、参数调整过程
3. 决定：要不要进入阶段 2（嵌入式主板 + 底盘 + Nav2）

### 验收
- [ ] `maps/` 下有清晰的最终地图
- [ ] 笔记里写了参数最佳值
- [ ] 下一阶段决策（继续 SLAM 优化 / 直接上 Nav2 / 加深度相机）明确

---

## 关键交付物（第一周末）

```
work/indoor-slam-67m/
├── maps/
│   └── home_67m.{pgm,yaml}        # 67 平米最终地图
├── logs/
│   └── week1.md                   # 本周笔记 + 参数记录
└── config/
    └── slam_toolbox_mapping.yaml  # 调好的参数
```

能用这 3 个文件把第一周"先跑通"的目标完整交付。