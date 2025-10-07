cmake_minimum_required(VERSION 4.1)
project(Mandelbrot_Set LANGUAGES C CXX)
include(FetchContent)

find_package(Threads REQUIRED)
find_package(Vulkan REQUIRED)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
)
FetchContent_MakeAvailable(glfw)

add_library(vma STATIC link/vk_mem_alloc.cpp)

file(GLOB SRC CONFIGURE_DEPENDS "src/*.h" "src/*.c" "src/gfx/*.c" "src/gfx/*.h")
add_executable(app ${SRC})
target_compile_options(app PRIVATE -O2 -std=c2x -Wall -Wextra -Wpedantic -Wconversion -Wno-override-init -Wno-pointer-arith -Wno-newline-eof -Wno-nullability-extension -Werror -Wfatal-errors)
target_compile_definitions(app PRIVATE GLFW_INCLUDE_VULKAN CGLM_FORCE_DEPTH_ZERO_TO_ONE)
target_include_directories(app PRIVATE "src" "src/gfx" "src/link")
target_link_libraries(app PRIVATE Threads::Threads Vulkan::Vulkan glfw vma)