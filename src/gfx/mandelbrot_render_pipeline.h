#pragma once
#include "result.h"
#include <cglm/types-struct.h>
#include <vulkan/vulkan.h>

result_t init_mandelbrot_render_pipeline(VkCommandBuffer command_buffer, VkFence command_fence, VkDescriptorPool descriptor_pool, const VkPhysicalDeviceProperties* physical_device_properties);
result_t draw_mandelbrot_render_pipeline(VkCommandBuffer command_buffer, const mat3s* affine_map);
void term_mandelbrot_render_pipeline(void);