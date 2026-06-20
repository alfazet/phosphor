cmake_minimum_required(VERSION 3.31)
project(phosphor LANGUAGES C CXX)
include(FetchContent)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(include)
add_executable(phosphor src/main.cpp)

target_compile_definitions(phosphor PRIVATE PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
if ("${CMAKE_BUILD_TYPE}" STREQUAL "debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
endif ()