# 一键启动定位：rplidar + slam_toolbox(localization) + map_server + rviz

from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    pkg_share = FindPackageShare('indoor_slam_67m')

    rplidar_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            PathJoinSubstitution([pkg_share, 'launch', 'rplidar.launch.py']),
        ]),
    )

    slam_localization_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            PathJoinSubstitution([pkg_share, 'launch', 'slam_localization.launch.py']),
        ]),
    )

    rviz_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            PathJoinSubstitution([pkg_share, 'launch', 'view_rviz.launch.py']),
        ]),
    )

    return LaunchDescription([
        rplidar_launch,
        slam_localization_launch,
        rviz_launch,
    ])