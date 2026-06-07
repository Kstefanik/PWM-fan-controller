cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(PWM-fan-controller)

zephyr_include_directories(src/common)

add_subdirectory(src/common)
add_subdirectory(src/modules/control_mgr)
add_subdirectory(src/modules/sensor_mgr)
add_subdirectory(src/modules/usb_mgr)
add_subdirectory(src/modules/tacho_mgr)

# Include SEGGER Ozone file generation
add_subdirectory(debug)

