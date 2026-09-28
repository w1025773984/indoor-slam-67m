#!/usr/bin/env bash
# ============================================================
# 保存 slam_toolbox 当前地图到 maps/ 目录
# 用法：./scripts/save_map.sh [map_name]
# 例：./scripts/save_map.sh home_67m
# ============================================================

set -e

# 默认地图名
MAP_NAME="${1:-home_67m}"
MAPS_DIR="$(cd "$(dirname "$0")/.." && pwd)/maps"

# 建目录
mkdir -p "$MAPS_DIR"

MAP_PATH="${MAPS_DIR}/${MAP_NAME}"

echo "============================================================"
echo " 保存地图"
echo "============================================================"
echo "  目标文件：${MAP_PATH}.pgm / ${MAP_PATH}.yaml"
echo ""

# nav2_map_server 提供 map_saver_cli
if ! command -v map_saver_cli &> /dev/null; then
    echo "⚠️  map_saver_cli 未安装，执行："
    echo "   sudo apt install -y ros-humble-nav2-map-server"
    exit 1
fi

# 调用 map_saver_cli
map_saver_cli \
    -f "$MAP_PATH" \
    --ros-args \
    -p use_sim_time:=false

echo ""
echo "============================================================"
echo " 地图已保存"
echo "============================================================"
echo ""
echo "  PGM: ${MAP_PATH}.pgm"
echo "  YAML: ${MAP_PATH}.yaml"
echo ""
echo "现在可以："
echo "  1. 关闭建图 launch"
echo "  2. 启动定位模式：ros2 launch indoor_slam_67m full_localization.launch.py"
echo "  3. 在 RViz2 里用 2D Pose Estimate 给机器人一个初始位姿"