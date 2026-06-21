cmake_minimum_required(VERSION 3.31)
project(phosphor LANGUAGES C CXX)
include(FetchContent)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(include)
file(GLOB SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_executable(phosphor ${SOURCES}
        include/random.hpp
        src/scene.cpp
)

FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm
        GIT_TAG 8d1fd52e5ab5590e2c81768ace50c72bae28f2ed # v1.0.3
)
FetchContent_MakeAvailable(glm)
target_link_libraries(phosphor glm)

FetchContent_Declare(
        stb
        GIT_REPOSITORY https://github.com/nothings/stb.git
        GIT_TAG 5c205738c191bcb0abc65c4febfa9bd25ff35234
)
FetchContent_MakeAvailable(stb)
target_include_directories(phosphor PRIVATE ${stb_SOURCE_DIR})

target_compile_definitions(phosphor PRIVATE PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
if ("${CMAKE_BUILD_TYPE}" STREQUAL "debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
else ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
endif ()
