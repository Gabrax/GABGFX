cmake_minimum_required(VERSION 3.16)
project("${EXE_NAME}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable("${CMAKE_PROJECT_NAME}")

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded") # Or "MultiThreadedDLL" if you prefer /MD

    if(RELEASE)
        add_compile_options(/MP /O3)
    endif()

    if(DEBUG)
        add_compile_options(/MP /Zi /Od) 
        add_link_options(/DEBUG /INCREMENTAL)
    endif()

    target_compile_definitions("${CMAKE_PROJECT_NAME}" PUBLIC _CRT_SECURE_NO_WARNINGS)

elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(
        -Wall -Wextra -Wpedantic
        -Wno-unused-function
        -Wno-unused-parameter
        -ferror-limit=0
    )
endif()

include(FetchContent)

FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG v5.3.0
)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "Disable Assimp tests" FORCE)
set(ASSIMP_BUILD_DOCS OFF CACHE BOOL "Disable Assimp documentation" FORCE)

FetchContent_Declare(
    raylib
    GIT_REPOSITORY "https://github.com/raysan5/raylib.git"
    GIT_TAG "master"
    GIT_PROGRESS TRUE
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  OpenCL_SDK
  GIT_REPOSITORY https://github.com/KhronosGroup/OpenCL-SDK.git
  GIT_TAG        main
  GIT_PROGRESS TRUE
)

set(OPENCL_SDK_BUILD_SAMPLES OFF CACHE BOOL "Disable SDK samples" FORCE)
set(OPENCL_SDK_BUILD_OPENGL_SAMPLES OFF CACHE BOOL "Disable OpenGL samples" FORCE)
set(OPENCL_SDK_BUILD_VULKAN_SAMPLES OFF CACHE BOOL "Disable Vulkan samples" FORCE)
set(OPENCL_SDK_TEST_SAMPLES OFF CACHE BOOL "Disable sample tests" FORCE)
set(OPENCL_SDK_FETCH_DEPENDENCIES ON CACHE BOOL "Fetch dependencies automatically" FORCE)

FetchContent_MakeAvailable(raylib OpenCL_SDK assimp)

add_subdirectory(vendor/stb_ds)
add_subdirectory(vendor/raygui)

file(GLOB_RECURSE MY_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

target_sources("${CMAKE_PROJECT_NAME}" PRIVATE ${MY_SOURCES})

target_link_libraries("${CMAKE_PROJECT_NAME}" PRIVATE assimp raylib OpenCL::OpenCL stb_ds raygui)
