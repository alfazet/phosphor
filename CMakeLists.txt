cmake_minimum_required(VERSION 4.0)
project(phosphor LANGUAGES C CXX)
include(FetchContent)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(include)
add_executable(phosphor src/main.cpp src/app.cpp)

FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 7b6aead9fb88b3623e3b3725ebb42670cbe4c579 # v3.4
)
FetchContent_MakeAvailable(glfw)
target_link_libraries(phosphor glfw)
target_include_directories(phosphor PRIVATE ${glfw_SOURCE_DIR}/include)

FetchContent_Declare(
        glad
        GIT_REPOSITORY https://github.com/Dav1dde/glad.git
        GIT_TAG 73db193f853e2ee079bf3ca8a64aa2eaf6459043 # v2.0.8
        SOURCE_SUBDIR cmake
)
FetchContent_MakeAvailable(glad)
glad_add_library(glad_gl_core_46 STATIC API gl:core=4.6)
target_link_libraries(phosphor glad_gl_core_46)

FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm
        GIT_TAG 8d1fd52e5ab5590e2c81768ace50c72bae28f2ed # v1.0.3
)
FetchContent_MakeAvailable(glm)
target_link_libraries(phosphor glm)

FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui
        GIT_TAG 6d910d5487d11ca567b61c7824b0c78c569d62f0 # v1.92.5
)
FetchContent_MakeAvailable(imgui)
file(GLOB imgui_SOURCES ${imgui_SOURCE_DIR}/*.cpp ${imgui_SOURCE_DIR}/*.h)
add_library(
        imgui
        ${imgui_SOURCES}
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.h
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.h
)
target_include_directories(
        imgui
        PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
)
target_include_directories(phosphor PRIVATE ${imgui_SOURCE_DIR})
target_link_libraries(phosphor imgui)

target_compile_definitions(phosphor PRIVATE PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
if ("${CMAKE_BUILD_TYPE}" STREQUAL "debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
endif ()