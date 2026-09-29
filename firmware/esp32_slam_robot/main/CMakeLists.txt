idf_component_register(
    SRCS
        "main.cpp"
        "motor.cpp"
        "encoder.cpp"
        "pid.cpp"
        "imu.cpp"
        "odometry.cpp"
        "ros2_node.cpp"
    INCLUDE_DIRS "."
    REQUIRES driver micro_ros_espidf
)