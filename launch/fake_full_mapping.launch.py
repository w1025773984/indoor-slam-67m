# 用 mock 数据 + slam_toolbox 跑通 pipeline（不需要硬件）
# 启动 fake_scan_publisher + static_transform_publisher + slam_toolbox(mapping)

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    pkg_share = FindPackageShare('indoor_slam_67m')

    # 1. fake_scan_publisher（不依赖硬件）
    fake_scan_node = Node(
        package='indoor_slam_67m',
        executable='fake_scan_publisher',
        name='fake_scan_publisher',
        output='screen',
        parameters=[{
            'frame_id': 'laser',
            'range_min': 0.15,
            'range_max': 12.0,
            'room_x_min': -2.0,
            'room_x_max': 2.0,
            'room_y_min': -2.0,
            'room_y_max': 2.0,
        }],
    )

    # 2. odom → base_footprint（静态，让 slam_toolbox 有完整 tf 链）
    static_tf_odom_node = Node(
        package='tf2_ros',
        executable='static_transform_publisher',
        name='static_tf_odom_to_base',
        arguments=[
            '--x', '0', '--y', '0', '--z', '0',
            '--roll', '0', '--yaw', '0', '--pitch', '0',
            '--frame-id', 'odom',
            '--child-frame-id', 'base_footprint',
        ],
        output='screen',
    )

    # 3. base_footprint → laser
    static_tf_base_node = Node(
        package='tf2_ros',
        executable='static_transform_publisher',
        name='static_tf_base_to_laser',
        arguments=[
            '--x', '0', '--y', '0', '--z', '0.5',  # 雷达离地 0.5m
            '--roll', '0', '--yaw', '0', '--pitch', '0',
            '--frame-id', 'base_footprint',
            '--child-frame-id', 'laser',
        ],
        output='screen',
    )

    # 3. slam_toolbox mapping 模式
    slam_mapping_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            PathJoinSubstitution([pkg_share, 'launch', 'slam_mapping.launch.py']),
        ]),
    )

    # 4. RViz2（可选）
    rviz_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([
            PathJoinSubstitution([pkg_share, 'launch', 'view_rviz.launch.py']),
        ]),
    )

    return LaunchDescription([
        fake_scan_node,
        static_tf_odom_node,
        static_tf_base_node,
        slam_mapping_launch,
        rviz_launch,
    ])