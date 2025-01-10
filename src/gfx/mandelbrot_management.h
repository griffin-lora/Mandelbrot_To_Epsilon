#pragma once
#include "chrono.h"
#include "result.h"
#include <cglm/types-struct.h>
#include <vulkan/vulkan.h>

#define NUM_MANDELBROT_FRAMES_IN_FLIGHT 2

typedef struct {
    uint32_t width;
    uint32_t height;
} mandelbrot_dispatch_t;

extern VkImage mandelbrot_color_images[NUM_MANDELBROT_FRAMES_IN_FLIGHT];
extern VkImageView mandelbrot_color_image_views[NUM_MANDELBROT_FRAMES_IN_FLIGHT];
extern mandelbrot_dispatch_t mandelbrot_dispatches[NUM_MANDELBROT_FRAMES_IN_FLIGHT];
extern size_t mandelbrot_frame_index_to_render_frame_index[NUM_MANDELBROT_FRAMES_IN_FLIGHT];

extern mat3s mandelbrot_compute_affine_map;

result_t init_mandelbrot_management(VkQueue queue, VkCommandBuffer command_buffer, VkFence command_fence, uint32_t queue_family_index);
result_t manage_mandelbrot_frames(const VkPhysicalDeviceProperties* physical_device_properties, microseconds_t* out_mandelbrot_frame_compute_time);
void term_mandelbrot_management(void);

size_t get_mandelbrot_front_frame_index(void);