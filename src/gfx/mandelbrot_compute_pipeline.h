#pragma once
#include "result.h"
#include <vulkan/vulkan.h>

result_t init_mandelbrot_compute_pipeline(VkDescriptorPool descriptor_pool);
// Technically, this does update the descriptor sets soo
void update_mandelbrot_compute_pipeline(size_t frame_index);
void record_mandelbrot_compute_pipeline_init_to_compute_transition(VkCommandBuffer command_buffer, size_t frame_index);
void record_mandelbrot_compute_pipeline_fragment_to_compute_transition(VkCommandBuffer command_buffer, size_t frame_index);
void record_mandelbrot_compute_pipeline(VkCommandBuffer command_buffer, size_t frame_index);
void term_mandelbrot_compute_pipeline(void);