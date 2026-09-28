# slam_toolbox 定位模式启动文件
# 基于已保存的地图做实时定位

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    # 定位参数文件
    params_file = PathJoinSubstitution([
        FindPackageShare('indoor_slam_67m'),
        'config',
        'slam_toolbox_localization.yaml',
    ])

    # 地图文件（YAML，会自动加载配套的 PGM）
    map_yaml = PathJoinSubstitution([
        FindPackageShare('indoor_slam_67m'),
        'maps',
        'home_67m.yaml',
    ])

    use_sim_time_arg = DeclareLaunchArgument(
        'use_sim_time',
        default_value='false',
        description='是否使用仿真时间'
    )

    # slam_toolbox 定位节点
    slam_localization_node = Node(
        package='slam_toolbox',
        executable='localization_slam_toolbox_node',
        name='slam_toolbox',
        output='screen',
        parameters=[
            params_file,
            {'use_sim_time': LaunchConfiguration('use_sim_time')},
        ],
    )

    # 地图服务器（加载已保存地图）
    map_server_node = Node(
        package='nav2_map_server',
        executable='map_server',
        name='map_server',
        output='screen',
        parameters=[{
            'yaml_filename': map_yaml,
            'use_sim_time': LaunchConfiguration('use_sim_time'),
        }],
    )

    # 生命周期管理器（激活 map_server）
    lifecycle_manager_node = Node(
        package='nav2_lifecycle_manager',
        executable='lifecycle_manager',
        name='lifecycle_manager_localization',
        output='screen',
        parameters=[{
            'use_sim_time': LaunchConfiguration('use_sim_time'),
            'autostart': True,
            'node_names': ['map_server'],
        }],
    )

    return LaunchDescription([
        use_sim_time_arg,
        map_server_node,
        slam_localization_node,
        lifecycle_manager_node,
    ])