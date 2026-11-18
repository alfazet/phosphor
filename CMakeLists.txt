cmake_minimum_required(VERSION 3.30)
project(phosphor LANGUAGES C CXX)
include(FetchContent)
include(CTest)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(include)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
list(FILTER SOURCES EXCLUDE REGEX "/main\\.cpp$")

add_library(phosphor_core STATIC ${SOURCES})
target_include_directories(phosphor_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

find_package(OpenCL REQUIRED)
target_link_libraries(phosphor_core PUBLIC OpenCL::OpenCL)

find_package(TBB QUIET)
if(TBB_FOUND)
    target_link_libraries(phosphor_core PUBLIC TBB::tbb)
else()
    message(WARNING "optional dependency Intel TBB not found - the image writer will fall back to sequential execution")
endif()

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
set(BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(opencl_hpp)
target_include_directories(phosphor_core PUBLIC ${opencl_headers_SOURCE_DIR} ${opencl_hpp_SOURCE_DIR}/include)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm
    GIT_TAG 1.0.3
)
FetchContent_MakeAvailable(glm)
target_link_libraries(phosphor_core PUBLIC glm)

set(BUILD_SHARED_LIBS ON CACHE INTERNAL "build Assimp as a static library")
set(ASSIMP_BUILD_TESTS OFF CACHE INTERNAL "turn off Assimp tests")
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE INTERNAL "turn off Assimp viewer/tools")
set(ASSIMP_INSTALL OFF CACHE INTERNAL "turn off Assimp installation rules")
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE "turn off Assimp compressed asset support")
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE INTERNAL "make warnings non-fatal")
FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG v6.0.5
)
FetchContent_MakeAvailable(assimp)
target_link_libraries(phosphor_core PUBLIC assimp::assimp)

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb
)
FetchContent_MakeAvailable(stb)
target_include_directories(phosphor_core PUBLIC ${stb_SOURCE_DIR})

target_compile_definitions(phosphor_core PUBLIC PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}" CL_TARGET_OPENCL_VERSION=300)
target_compile_options(phosphor_core PUBLIC -Wno-deprecated-declarations)
if (${CMAKE_BUILD_TYPE} STREQUAL "debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
endif ()

file(GLOB_RECURSE PHOSPHOR_MAIN CONFIGURE_DEPENDS src/main.cpp)
if (NOT PHOSPHOR_MAIN)
    message(FATAL_ERROR "entry point not found, expected src/main.cpp")
endif ()
add_executable(phosphor ${PHOSPHOR_MAIN})
target_link_libraries(phosphor PRIVATE phosphor_core)

if (BUILD_TESTING)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.16.1
        GIT_SHALLOW TRUE
        EXCLUDE_FROM_ALL
    )
    set(CATCH_INSTALL_DOCS OFF CACHE BOOL "" FORCE)
    set(CATCH_INSTALL_EXTRAS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(Catch2)

    add_executable(unit_tests
        tests/unit/test_photon_hash.cpp
    )
    target_link_libraries(unit_tests PRIVATE phosphor_core Catch2::Catch2WithMain)
    target_include_directories(unit_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)

    include(Catch)
    catch_discover_tests(unit_tests
        TEST_PREFIX "unit."
        PROPERTIES LABELS unit)

    # option(PHOSPHOR_BUILD_DEVICE_TESTS "Build tests that require an OpenCL device" ON)
    # if (PHOSPHOR_BUILD_DEVICE_TESTS)
    #     add_executable(device_tests
    #         tests/device/test_photon_gather.cpp
    #     )
    #     target_link_libraries(device_tests PRIVATE phosphor_core Catch2::Catch2WithMain)
    #     target_compile_definitions(device_tests PRIVATE
    #         PHOSPHOR_KERNEL_DIR="${CMAKE_CURRENT_SOURCE_DIR}/src")
    #     catch_discover_tests(device_tests
    #         TEST_PREFIX "device."
    #         PROPERTIES LABELS "device;requires-opencl")
    # endif ()
endif ()
