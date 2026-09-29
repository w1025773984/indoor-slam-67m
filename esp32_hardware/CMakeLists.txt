cmake_minimum_required(VERSION 3.16)
project(esp32_hardware LANGUAGES CXX)

# Default to C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(hardware_interface REQUIRED)
find_package(pluginlib REQUIRED)
find_package(std_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)

# Library
add_library(${PROJECT_NAME} SHARED
    src/esp32_hardware_interface.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/esp32_hardware>
)

ament_target_dependencies(${PROJECT_NAME}
    rclcpp
    rclcpp_lifecycle
    hardware_interface
    pluginlib
    std_msgs
    geometry_msgs
    sensor_msgs
)

# Pluginlib export
pluginlib_export_plugin_description_file(hardware_interface esp32_hardware.xml)

# Install
install(
    TARGETS ${PROJECT_NAME}
    EXPORT export_${PROJECT_NAME}
    RUNTIME DESTINATION lib/${PROJECT_NAME}
    ARCHIVE DESTINATION lib/${PROJECT_NAME}
    LIBRARY DESTINATION lib/${PROJECT_NAME}
)

install(
    DIRECTORY include/
    DESTINATION include/esp32_hardware
)

ament_package()