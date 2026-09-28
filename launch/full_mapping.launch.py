# 一键启动建图：rplidar + slam_toolbox(mapping) + rviz

from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, DeclareLaunchArgument
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    pkg_share = FindPackageShare('indoor_slam_67m')

    # 启动 RPLIDAR
    rplidar_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            PathJoinSubstitution([pkg_share, 'launch', 'rplidar.launch.py']),
        ]),
    )

    # 启动 slam_toolbox mapping 模式
    slam_mapping_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            PathJoinSubstitution([pkg_share, 'launch', 'slam_mapping.launch.py']),
        ]),
    )

    # 启动 RViz2
    rviz_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            PathJoinSubstitution([pkg_share, 'launch', 'view_rviz.launch.py']),
        ]),
    )

    return LaunchDescription([
        rplidar_launch,
        slam_mapping_launch,
        rviz_launch,
    ])