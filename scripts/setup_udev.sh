#!/usr/bin/env bash
# ============================================================
# 一键配置 udev 规则
# 让 RPLIDAR 永远叫 /dev/rplidar，ESP32 永远叫 /dev/esp32
# 避免 /dev/ttyUSB* 编号漂移
# ============================================================

set -e

echo "============================================================"
echo " udev 规则配置：RPLIDAR + ESP32"
echo "============================================================"

# ------------------------------------------------------------
# 1. RPLIDAR udev 规则
# ------------------------------------------------------------
RPLIDAR_RULES=/etc/udev/rules.d/99-rplidar.rules
echo "→ 写入 RPLIDAR udev 规则到 $RPLIDAR_RULES"

sudo tee "$RPLIDAR_RULES" > /dev/null <<'EOF'
# RPLIDAR A2 / A3 / S1
# 思岚科技 USB 串口，Vendor ID 通常是 10c4:ea60 (CP2102) 或 1a86:7523 (CH340)
SUBSYSTEM=="tty", ATTRS{idVendor}=="10c4", ATTRS{idProduct}=="ea60", SYMLINK+="rplidar"
SUBSYSTEM=="tty", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="7523", SYMLINK+="rplidar"
EOF

# ------------------------------------------------------------
# 2. ESP32 udev 规则
# ------------------------------------------------------------
ESP32_RULES=/etc/udev/rules.d/99-esp32.rules
echo "→ 写入 ESP32 udev 规则到 $ESP32_RULES"

sudo tee "$ESP32_RULES" > /dev/null <<'EOF'
# ESP32 DevKit / NodeMCU
# 大多数 ESP32 开发板用 CP2102 (10c4:ea60) 或 CH340 (1a86:7523)
# 注意：如果 ESP32 和 RPLIDAR 用同一芯片（CP2102），只能通过物理端口区分
# 这种情况下用端口号 ATTRS{port_num} 区分
SUBSYSTEM=="tty", ATTRS{idVendor}=="10c4", ATTRS{idProduct}=="ea60", ENV{ID_MM_PORT}=="devops" SYMLINK+="esp32"
SUBSYSTEM=="tty", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="7523", SYMLINK+="esp32"
EOF

# ------------------------------------------------------------
# 3. 触发 udev 重新加载
# ------------------------------------------------------------
echo "→ 触发 udev 重新加载"
sudo udevadm control --reload-rules
sudo udevadm trigger

echo ""
echo "============================================================"
echo " 配置完成"
echo "============================================================"
echo ""
echo "下一步："
echo "  1. 拔掉 RPLIDAR 和 ESP32 的 USB"
echo "  2. 重新插上"
echo "  3. 验证："
echo "       ls -la /dev/rplidar /dev/esp32"
echo ""
echo "如果没有看到设备，可能需要："
echo "  - 确认用户组：sudo usermod -aG dialout \$USER，然后重新登录"
echo "  - 检查 dmesg：dmesg | tail -20"