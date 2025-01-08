#pragma once
#include "result.h"
#include <cglm/types-struct.h>
#include <vulkan/vulkan.h>

result_t init_mandelbrot_render_pipeline(VkQueue queue, VkCommandBuffer command_buffer, VkFence command_fence, VkDescriptorPool descriptor_pool, const VkPhysicalDeviceProperties* physical_device_properties);
void update_mandelbrot_render_pipeline(size_t frame_index);
result_t draw_mandelbrot_render_pipeline(VkCommandBuffer command_buffer, size_t mandelbrot_frame_index, size_t render_frame_index, const mat3s* affine_map);
void term_mandelbrot_render_pipeline(void);