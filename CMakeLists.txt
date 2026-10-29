cmake_minimum_required(VERSION 4.0)

project(chip8
    LANGUAGES C
)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED 23)

include(FetchContent)

FetchContent_Declare(
        unity
        GIT_REPOSITORY  https://github.com/ThrowTheSwitch/Unity
        GIT_TAG         v2.7.0
)

# ---------------------------
# Base Target
# ---------------------------

find_package(SDL3 CONFIG REQUIRED)

add_executable(chip8
    src/main.c
    src/chip8.c
    src/audio.c
)

target_include_directories(chip8
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(chip8
    PRIVATE SDL3::SDL3
)
