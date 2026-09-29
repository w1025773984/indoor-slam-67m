#!/usr/bin/env python3
"""
Mock 2D 激光扫描发布器
- 模拟一个 4m x 4m 房间的激光雷达数据
- 让 slam_toolbox 能接收 /scan 并建图
- 用于在没有真实雷达时验证 SLAM pipeline

启动：ros2 run indoor_slam_67m fake_scan_publisher
"""

import rclpy
from rclpy.node import Node
from sensor_msgs.msg import LaserScan
from std_msgs.msg import Header
import math
import numpy as np


class FakeScanPublisher(Node):
    def __init__(self):
        super().__init__('fake_scan_publisher')

        # 参数
        self.declare_parameter('frame_id', 'laser')
        self.declare_parameter('range_min', 0.15)
        self.declare_parameter('range_max', 12.0)
        self.declare_parameter('angle_min', -math.pi)
        self.declare_parameter('angle_max', math.pi)
        self.declare_parameter('scan_rate', 10.0)  # Hz
        # 房间尺寸（米）：矩形房间 [x_min, x_max] x [y_min, y_max]
        self.declare_parameter('room_x_min', -2.0)
        self.declare_parameter('room_x_max', 2.0)
        self.declare_parameter('room_y_min', -2.0)
        self.declare_parameter('room_y_max', 2.0)
        # 模拟机器人位置
        self.declare_parameter('robot_x', 0.0)
        self.declare_parameter('robot_y', 0.0)

        self.frame_id = self.get_parameter('frame_id').value
        self.range_min = self.get_parameter('range_min').value
        self.range_max = self.get_parameter('range_max').value
        self.angle_min = self.get_parameter('angle_min').value
        self.angle_max = self.get_parameter('angle_max').value
        self.scan_rate = self.get_parameter('scan_rate').value

        self.room = {
            'x_min': self.get_parameter('room_x_min').value,
            'x_max': self.get_parameter('room_x_max').value,
            'y_min': self.get_parameter('room_y_min').value,
            'y_max': self.get_parameter('room_y_max').value,
        }
        self.robot_x = self.get_parameter('robot_x').value
        self.robot_y = self.get_parameter('robot_y').value

        # 启动时记录时间，让机器人慢慢动
        self.t = 0.0

        # 发布器
        self.publisher_ = self.create_publisher(LaserScan, '/scan', 10)

        # 定时器
        period = 1.0 / self.scan_rate
        self.timer = self.create_timer(period, self.publish_scan)

        self.get_logger().info(
            f'FakeScanPublisher 启动：房间 {self.room}, 机器人 ({self.robot_x}, {self.robot_y})'
        )

    def publish_scan(self):
        scan = LaserScan()
        scan.header = Header()
        scan.header.stamp = self.get_clock().now().to_msg()
        scan.header.frame_id = self.frame_id
        scan.angle_min = self.angle_min
        scan.angle_max = self.angle_max
        # 360 个扫描点
        scan.angle_increment = (self.angle_max - self.angle_min) / 360.0
        scan.time_increment = 0.0
        scan.scan_time = 1.0 / self.scan_rate
        scan.range_min = self.range_min
        scan.range_max = self.range_max
        scan.ranges = []

        # 机器人位置：沿矩形轨迹移动（这样 slam_toolbox 能看到地图）
        # 周期 = 16 秒一圈
        cycle = 16.0
        phase = (self.t % cycle) / cycle  # 0-1
        half = self.room['x_max'] - 1.0
        if phase < 0.25:
            # 左边沿墙向上
            tx = self.room['x_min'] + 0.5
            ty = self.room['y_min'] + phase * 4 * (self.room['y_max'] - self.room['y_min'] - 1)
        elif phase < 0.5:
            # 上边沿墙向右
            tx = self.room['x_min'] + 0.5 + (phase - 0.25) * 4 * (self.room['x_max'] - self.room['x_min'] - 1)
            ty = self.room['y_max'] - 0.5
        elif phase < 0.75:
            # 右边沿墙向下
            tx = self.room['x_max'] - 0.5
            ty = self.room['y_max'] - 0.5 - (phase - 0.5) * 4 * (self.room['y_max'] - self.room['y_min'] - 1)
        else:
            # 下边沿墙向左
            tx = self.room['x_max'] - 0.5 - (phase - 0.75) * 4 * (self.room['x_max'] - self.room['x_min'] - 1)
            ty = self.room['y_min'] + 0.5

        # 计算每个角度的距离（基于矩形房间的 ray casting）
        for i in range(361):
            angle = self.angle_min + i * scan.angle_increment
            distance = self._ray_cast(tx, ty, angle)
            scan.ranges.append(distance)

        self.publisher_.publish(scan)
        self.t += 1.0 / self.scan_rate

    def _ray_cast(self, x, y, angle):
        """矩形房间内 ray cast：返回 (x,y) 朝 angle 方向到达墙的距离"""
        dx = math.cos(angle)
        dy = math.sin(angle)

        if abs(dx) < 1e-6:
            t_x = float('inf')
        elif dx > 0:
            t_x = (self.room['x_max'] - x) / dx
        else:
            t_x = (self.room['x_min'] - x) / dx

        if abs(dy) < 1e-6:
            t_y = float('inf')
        elif dy > 0:
            t_y = (self.room['y_max'] - y) / dy
        else:
            t_y = (self.room['y_min'] - y) / dy

        t = min(t_x, t_y)
        if t < 0:
            t = max(t_x, t_y)
        if t == float('inf') or t < 0:
            t = self.range_max

        return max(self.range_min, min(self.range_max, t))


def main(args=None):
    rclpy.init(args=args)
    node = FakeScanPublisher()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()