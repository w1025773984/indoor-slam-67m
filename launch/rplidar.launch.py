# RPLIDAR A2 驱动启动文件
# 单独启动雷达节点，用于调试

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node


def generate_launch_description():
    # 参数声明
    serial_port_arg = DeclareLaunchArgument(
        'serial_port',
        default_value='/dev/rplidar',
        description='RPLIDAR 串口设备路径'
    )

    serial_baudrate_arg = DeclareLaunchArgument(
        'serial_baudrate',
        default_value='115200',
        description='RPLIDAR 波特率（A2M8 用 115200）'
    )

    frame_id_arg = DeclareLaunchArgument(
        'frame_id',
        default_value='laser',
        description='雷达坐标系名称'
    )

    scan_mode_arg = DeclareLaunchArgument(
        'scan_mode',
        default_value='Standard',
        description='扫描模式：Standard / Boost / Sensitivity'
    )

    # RPLIDAR 节点
    rplidar_node = Node(
        package='rplidar_ros2',
        executable='rplidar_node',
        name='rplidar_node',
        output='screen',
        parameters=[{
            'serial_port': LaunchConfiguration('serial_port'),
            'serial_baudrate': LaunchConfiguration('serial_baudrate'),
            'frame_id': LaunchConfiguration('frame_id'),
            'inverted': False,
            'angle_compensate': True,
            'scan_mode': LaunchConfiguration('scan_mode'),
        }],
    )

    return LaunchDescription([
        serial_port_arg,
        serial_baudrate_arg,
        frame_id_arg,
        scan_mode_arg,
        rplidar_node,
    ])