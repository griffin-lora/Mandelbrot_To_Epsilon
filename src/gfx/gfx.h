#pragma once
#include "result.h"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

#define NUM_FRAMES_IN_FLIGHT 2

extern GLFWwindow* window;
extern VkDevice device;
extern VmaAllocator allocator;
extern VkQueue queue;
extern VkSurfaceFormatKHR surface_format;

extern VkSampleCountFlagBits render_multisample_flags;
extern VkRenderPass frame_render_pass;
extern VkFence in_flight_fences[NUM_FRAMES_IN_FLIGHT];

result_t init_gfx(void);
result_t draw_gfx();
void term_gfx(void);