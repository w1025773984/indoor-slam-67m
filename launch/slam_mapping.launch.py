# slam_toolbox 建图模式启动文件
# 使用 async 模式：实时处理 scan，适合手持/遥控扫图

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    # 参数文件路径
    params_file = PathJoinSubstitution([
        FindPackageShare('indoor_slam_67m'),
        'config',
        'slam_toolbox_mapping.yaml',
    ])

    use_sim_time_arg = DeclareLaunchArgument(
        'use_sim_time',
        default_value='false',
        description='是否使用仿真时间（真实雷达用 false）'
    )

    # slam_toolbox async 节点（建图）
    slam_toolbox_node = Node(
        package='slam_toolbox',
        executable='async_slam_toolbox_node',
        name='slam_toolbox',
        output='screen',
        parameters=[
            params_file,
            {'use_sim_time': LaunchConfiguration('use_sim_time')},
        ],
    )

    return LaunchDescription([
        use_sim_time_arg,
        slam_toolbox_node,
    ])