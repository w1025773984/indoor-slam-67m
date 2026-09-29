# 启动 robot_state_publisher 发布 URDF tf
# 用法：ros2 launch indoor_slam_67m robot_state.launch.py

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, Command, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    pkg_share = FindPackageShare('indoor_slam_67m')

    # URDF 文件路径
    urdf_file = PathJoinSubstitution([
        pkg_share,
        'urdf',
        'slam_robot.urdf.xacro',
    ])

    # robot_state_publisher
    robot_state_publisher_node = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        name='robot_state_publisher',
        output='screen',
        parameters=[{
            'robot_description': Command(['xacro', ' ', urdf_file]),
            'use_sim_time': LaunchConfiguration('use_sim_time'),
        }],
    )

    use_sim_time_arg = DeclareLaunchArgument(
        'use_sim_time',
        default_value='false',
        description='是否使用仿真时间'
    )

    return LaunchDescription([
        use_sim_time_arg,
        robot_state_publisher_node,
    ])