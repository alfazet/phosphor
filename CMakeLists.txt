cmake_minimum_required(VERSION 3.30)
project(phosphor LANGUAGES C CXX)
include(FetchContent)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(include)
file(GLOB SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_executable(phosphor ${SOURCES})

FetchContent_Declare(
    opencl_headers
    GIT_REPOSITORY https://github.com/KhronosGroup/OpenCL-Headers
    GIT_TAG v2026.05.29
)
FetchContent_MakeAvailable(opencl_headers)
set(OpenCL_INCLUDE_DIR ${opencl_headers_SOURCE_DIR} CACHE PATH "" FORCE)

FetchContent_Declare(
    opencl_hpp
    GIT_REPOSITORY https://github.com/KhronosGroup/OpenCL-CLHPP
    GIT_TAG v2026.05.29
)
FetchContent_MakeAvailable(opencl_hpp)
target_include_directories(phosphor PRIVATE ${opencl_headers_SOURCE_DIR} ${opencl_hpp_SOURCE_DIR}/include)

find_package(OpenCL REQUIRED)
target_link_libraries(phosphor OpenCL::OpenCL)

FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm
        GIT_TAG 1.0.3
)
FetchContent_MakeAvailable(glm)
target_link_libraries(phosphor glm)

FetchContent_Declare(
        stb
        GIT_REPOSITORY https://github.com/nothings/stb
)
FetchContent_MakeAvailable(stb)
target_include_directories(phosphor PRIVATE ${stb_SOURCE_DIR})

set(BUILD_SHARED_LIBS ON CACHE INTERNAL "Build Assimp as a static library")
set(ASSIMP_BUILD_TESTS OFF CACHE INTERNAL "Turn off Assimp tests")
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE INTERNAL "Turn off Assimp viewer/tools")
set(ASSIMP_INSTALL OFF CACHE INTERNAL "Turn off Assimp installation rules")
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE "Turn off Assimp compressed asset support")
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE INTERNAL "Make warnings non-fatal")
FetchContent_Declare(
        assimp
        GIT_REPOSITORY https://github.com/assimp/assimp.git
        GIT_TAG v6.0.5
)
FetchContent_MakeAvailable(assimp)
target_link_libraries(phosphor assimp::assimp)

target_compile_definitions(phosphor PRIVATE PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
if (${CMAKE_BUILD_TYPE} STREQUAL "debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
endif ()
