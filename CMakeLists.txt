cmake_minimum_required(VERSION 3.31)
project(Mandelbrot_Set LANGUAGES C CXX)

include(${CMAKE_CURRENT_BINARY_DIR}/conan_toolchain.cmake)
set(CMAKE_BUILD_TYPE Release)

find_package(Threads REQUIRED)
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(cglm REQUIRED)
find_package(VulkanMemoryAllocator REQUIRED)

add_library(vma STATIC link/vk_mem_alloc.cpp)
target_link_libraries(vma PUBLIC GPUOpen::VulkanMemoryAllocator)

file(GLOB SRC CONFIGURE_DEPENDS "src/*.c" "src/gfx/*.c")
add_executable(app ${SRC})
target_compile_options(app PRIVATE -O2 -std=c2x -Wall -Wextra -Wpedantic -Wconversion -Wno-override-init -Wno-pointer-arith -Wno-newline-eof -Wno-nullability-extension -Werror -Wfatal-errors)
target_compile_definitions(app PRIVATE GLFW_INCLUDE_VULKAN CGLM_FORCE_DEPTH_ZERO_TO_ONE)
target_include_directories(app PRIVATE "src" "src/gfx")
target_link_libraries(app PRIVATE Threads::Threads Vulkan::Vulkan glfw cglm::cglm vma)