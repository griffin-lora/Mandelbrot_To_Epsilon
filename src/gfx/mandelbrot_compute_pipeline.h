#pragma once
#include "result.h"
#include <vulkan/vulkan.h>

result_t init_mandelbrot_compute_pipeline(VkDescriptorPool descriptor_pool);
// Technically, this does update the descriptor sets soo
void write_mandelbrot_compute_pipeline_descriptors(size_t frame_index);
result_t record_mandelbrot_compute_pipeline(VkCommandBuffer command_buffer, size_t frame_index);
void term_mandelbrot_compute_pipeline(void);