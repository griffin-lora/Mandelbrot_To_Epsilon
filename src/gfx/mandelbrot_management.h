#pragma once
#include "result.h"
#include <vulkan/vulkan.h>

#define NUM_MANDELBROT_FRAMES_IN_FLIGHT 2

typedef struct {
    uint32_t width;
    uint32_t height;
} mandelbrot_dispatch_t;

extern VkImage mandelbrot_color_images[NUM_MANDELBROT_FRAMES_IN_FLIGHT];
extern VkImageView mandelbrot_color_image_views[NUM_MANDELBROT_FRAMES_IN_FLIGHT];
extern mandelbrot_dispatch_t mandelbrot_dispatches[NUM_MANDELBROT_FRAMES_IN_FLIGHT];

result_t init_mandelbrot_management(VkCommandBuffer command_buffer, VkFence command_fence, uint32_t graphics_queue_family_index);
result_t manage_mandelbrot_frames();
void term_mandelbrot_management(void);

size_t get_front_frame_index(void);