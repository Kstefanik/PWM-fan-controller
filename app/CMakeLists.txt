cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
#project(wiatrak_cpp)
project(wiatrak_cpp LANGUAGES C CXX)

#target_sources(app PRIVATE src/app_logic.c)

target_sources(app PRIVATE 
    src/app_logic.c 
    src/zbus_defs.c
)