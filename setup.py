from setuptools import setup
import os
from glob import glob

package_name = 'indoor_slam_67m'

setup(
    name=package_name,
    version='0.1.0',
    packages=[package_name],
    data_files=[
        # ROS2 包索引
        ('share/ament_index/resource_index/packages',
            ['resource/' + package_name]),
        # package.xml
        ('share/' + package_name, ['package.xml']),
        # launch 文件
        (os.path.join('share', package_name, 'launch'),
            glob('launch/*.py') + glob('launch/*.xml')),
        # config 文件
        (os.path.join('share', package_name, 'config'),
            glob('config/*.yaml') + glob('config/*.rviz')),
        # 地图（建好地图后保存到这里，会被一起 install）
        (os.path.join('share', package_name, 'maps'),
            glob('maps/*.pgm') + glob('maps/*.yaml')),
        # scripts
        (os.path.join('share', package_name, 'scripts'),
            glob('scripts/*.sh')),
        # URDF（xacro 模块化）
        (os.path.join('share', package_name, 'urdf'),
            glob('urdf/*.xacro')),
        (os.path.join('share', package_name, 'urdf', 'xacro'),
            glob('urdf/xacro/*.xacro')),
        # docs（可选）
        (os.path.join('share', package_name, 'docs'),
            glob('docs/*.md')),
    ],
    install_requires=['setuptools'],
    zip_safe=True,
    maintainer='User',
    maintainer_email='user@example.com',
    description='室内 67 平米家用 SLAM 项目（建图 + 定位）',
    license='MIT',
    tests_require=['pytest'],
    entry_points={
        'console_scripts': [
            'fake_scan_publisher = indoor_slam_67m.fake_scan_publisher:main',
        ],
    },
)