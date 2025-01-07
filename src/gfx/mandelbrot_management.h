#pragma once
#include "result.h"
#include <vulkan/vulkan.h>

#define NUM_MANDELBROT_FRAMES_IN_FLIGHT 2

extern VkImage mandelbrot_color_images[NUM_MANDELBROT_FRAMES_IN_FLIGHT];
extern VkImageView mandelbrot_color_image_views[NUM_MANDELBROT_FRAMES_IN_FLIGHT];

result_t init_mandelbrot_management(VkCommandBuffer command_buffer, VkFence command_fence, uint32_t graphics_queue_family_index);
result_t manage_mandelbrot_frames(void);
void term_mandelbrot_management(void);

size_t get_front_frame_index(void);