cmake_minimum_required(VERSION 3.20.0)

# Resolve workspace-local Zephyr path to avoid inheriting a stale global ZEPHYR_BASE.
# get_filename_component(WORKSPACE_ROOT ${CMAKE_CURRENT_LIST_DIR}/.. ABSOLUTE)
# set(LOCAL_ZEPHYR_BASE ${WORKSPACE_ROOT}/zephyr)

# if (EXISTS ${LOCAL_ZEPHYR_BASE}/share/zephyr-package/cmake/ZephyrConfig.cmake)
#     set(ENV{ZEPHYR_BASE} ${LOCAL_ZEPHYR_BASE})
# endif ()

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(PWM-fan-controller)

# Include files that are common for all modules
add_subdirectory(src/common)

# Include modules
add_subdirectory(src/modules/control_mgr)
add_subdirectory(src/modules/sensor_mgr)
add_subdirectory(src/modules/usb_mgr)
add_subdirectory(src/modules/tacho_mgr)


# Include SEGGER Ozone file generation
add_subdirectory(debug)

