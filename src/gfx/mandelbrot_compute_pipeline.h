#pragma once
#include "result.h"
#include <vulkan/vulkan.h>

extern VkImageView mandelbrot_color_image_view;

result_t init_mandelbrot_compute_pipeline(VkDescriptorPool descriptor_pool);
result_t record_mandelbrot_compute_pipeline(VkCommandBuffer command_buffer);
void term_mandelbrot_compute_pipeline(void);