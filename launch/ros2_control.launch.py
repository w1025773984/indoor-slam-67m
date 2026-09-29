# 启动 ros2_control + controller_manager + DiffDriveController
# 用法：ros2 launch indoor_slam_67m ros2_control.launch.py

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, Command, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    pkg_share = FindPackageShare('indoor_slam_67m')

    # URDF（带 ros2_control 标签）
    urdf_file = PathJoinSubstitution([
        pkg_share,
        'urdf',
        'slam_robot.ros2_control.xacro',
    ])

    # controllers.yaml
    controllers_file = PathJoinSubstitution([
        pkg_share,
        'config',
        'ros2_control_controllers.yaml',
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

    # controller_manager（用 ros2_control_node 启动器）
    controller_manager_node = Node(
        package='controller_manager',
        executable='ros2_control_node',
        name='controller_manager',
        output='screen',
        parameters=[
            {'robot_description': Command(['xacro', ' ', urdf_file])},
            controllers_file,
            {'use_sim_time': LaunchConfiguration('use_sim_time')},
        ],
    )

    # spawn joint_state_broadcaster
    joint_state_broadcaster_spawner = Node(
        package='controller_manager',
        executable='spawner',
        name='joint_state_broadcaster_spawner',
        arguments=['joint_state_broadcaster', '--controller-manager', '/controller_manager'],
        output='screen',
    )

    # spawn diff_drive_controller
    diff_drive_spawner = Node(
        package='controller_manager',
        executable='spawner',
        name='diff_drive_controller_spawner',
        arguments=['diff_drive_controller', '--controller-manager', '/controller_manager'],
        output='screen',
    )

    return LaunchDescription([
        DeclareLaunchArgument('use_sim_time', default_value='false'),
        robot_state_publisher_node,
        controller_manager_node,
        joint_state_broadcaster_spawner,
        diff_drive_spawner,
    ])