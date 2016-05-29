cmake_minimum_required(VERSION 2.6)
project(ANALYSE)
set(ANALYSE_VERSION_MAJOR 1)
set(ANALYSE_VERSION_MINOR 24)

set(CMAKE_BUILD_TYPE Release)
#set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_FLAGS "-std=gnu99 ${CMAKE_C_FLAGS} -Wall -Werror")
#set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Wall -Werror")
#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Werror")

execute_process(COMMAND lsb_release -sd OUTPUT_VARIABLE OS_TYPE)

set(MOD_PREFIX "mod_")
configure_file(
    "${PROJECT_SOURCE_DIR}/src/version.h.in"
    "${PROJECT_SOURCE_DIR}/src/version.h"
    )

add_subdirectory(src)
add_subdirectory(module)

