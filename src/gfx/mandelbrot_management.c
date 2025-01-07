#include "mandelbrot_management.h"
#include "gfx/default.h"
#include "gfx/gfx.h"
#include "gfx/gfx_util.h"
#include "gfx/mandelbrot_compute_pipeline.h"
#include "result.h"
#include <stdio.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

static size_t frame_index = 0;

VkImage mandelbrot_color_images[NUM_MANDELBROT_FRAMES_IN_FLIGHT];
VkImageView mandelbrot_color_image_views[NUM_MANDELBROT_FRAMES_IN_FLIGHT];
static VmaAllocation mandelbrot_color_image_allocations[NUM_MANDELBROT_FRAMES_IN_FLIGHT];
static VkFence mandelbrot_fences[NUM_MANDELBROT_FRAMES_IN_FLIGHT];
static VkCommandBuffer mandelbrot_command_buffers[NUM_MANDELBROT_FRAMES_IN_FLIGHT];

static VkCommandPool mandelbrot_command_pool;

result_t init_mandelbrot_management(VkCommandBuffer command_buffer, VkFence command_fence, uint32_t graphics_queue_family_index) {
    result_t result;

    for (size_t i = 0; i < NUM_MANDELBROT_FRAMES_IN_FLIGHT; i++) {
        if (vmaCreateImage(allocator, &(VkImageCreateInfo) {
            DEFAULT_VK_IMAGE,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = VK_FORMAT_R8G8B8A8_UNORM,
            .extent = { 640, 640, 1 },
            .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
        }, &device_allocation_create_info, &mandelbrot_color_images[i], &mandelbrot_color_image_allocations[i], NULL) != VK_SUCCESS) {
            return result_image_create_failure;
        }

        if (vkCreateImageView(device, &(VkImageViewCreateInfo) {
            DEFAULT_VK_IMAGE_VIEW,
            .image = mandelbrot_color_images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = VK_FORMAT_R8G8B8A8_UNORM,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT
        }, NULL, &mandelbrot_color_image_views[i]) != VK_SUCCESS) {
            return result_image_view_create_failure;
        }

        if (vkCreateFence(device, &(VkFenceCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
        }, NULL, &mandelbrot_fences[i]) != VK_SUCCESS) {
            return result_synchronization_primitive_create_failure;
        }
    }

    write_mandelbrot_compute_pipeline_descriptors(frame_index);

    if (vkBeginCommandBuffer(command_buffer, &(VkCommandBufferBeginInfo) {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    }) != VK_SUCCESS) {
        return result_command_buffer_begin_failure;
    }

    for (size_t i = 0; i < NUM_MANDELBROT_FRAMES_IN_FLIGHT; i++) {
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &(VkImageMemoryBarrier) {
            DEFAULT_VK_IMAGE_MEMORY_BARRIER,
            .image = mandelbrot_color_images[i],
            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT
        });
    }

    if ((result = record_mandelbrot_compute_pipeline(command_buffer, frame_index)) != result_success) {
        return result;
    }
    
    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
        return result_command_buffer_end_failure;
    }

    if ((result = submit_and_wait(command_buffer, command_fence)) != result_success) {
        return result;
    }
    if ((result = reset_command_processing(command_buffer, command_fence)) != result_success) {
        return result;
    }

    if (vkCreateCommandPool(device, &(VkCommandPoolCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = graphics_queue_family_index
    }, NULL, &mandelbrot_command_pool) != VK_SUCCESS) {
        return result_command_pool_create_failure;
    }

    if (vkAllocateCommandBuffers(device, &(VkCommandBufferAllocateInfo) {
        DEFAULT_VK_COMMAND_BUFFER,
        .commandPool = mandelbrot_command_pool,
        .commandBufferCount = NUM_MANDELBROT_FRAMES_IN_FLIGHT
    }, mandelbrot_command_buffers) != VK_SUCCESS) {
        return result_command_buffers_allocate_failure;
    }

    return result_success;
}

result_t manage_mandelbrot_frames(void) {
    if (true) {
        return result_success;
    }

    result_t result;

    VkFence command_fence = mandelbrot_fences[(frame_index + 1) % NUM_MANDELBROT_FRAMES_IN_FLIGHT];
    if (vkGetFenceStatus(device, command_fence) != VK_SUCCESS) {
        return result_success;
    }
    frame_index = (frame_index + 1) % NUM_MANDELBROT_FRAMES_IN_FLIGHT;
    size_t next_frame_index = (frame_index + 1) % NUM_MANDELBROT_FRAMES_IN_FLIGHT;

    VkFence next_command_fence = mandelbrot_fences[next_frame_index];

    write_mandelbrot_compute_pipeline_descriptors(next_frame_index);

    VkCommandBuffer command_buffer = mandelbrot_command_buffers[next_frame_index];
    if (vkResetCommandBuffer(command_buffer, 0) != VK_SUCCESS) {
        return result_command_buffer_reset_failure;
    }

    if (vkBeginCommandBuffer(command_buffer, &(VkCommandBufferBeginInfo) {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
    }) != VK_SUCCESS) {
        return result_command_buffer_begin_failure;
    }

    if ((result = record_mandelbrot_compute_pipeline(command_buffer, next_frame_index)) != result_success) {
        return result;
    }

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
        return result_command_buffer_end_failure;
    }

    vkResetFences(device, 1, &next_command_fence);
    if (vkQueueSubmit(queue, 1, &(VkSubmitInfo) {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer
    }, next_command_fence) != VK_SUCCESS) {
        return result_queue_submit_failure;
    }

    return result_success;
}

void term_mandelbrot_management(void) {
    vkDestroyCommandPool(device, mandelbrot_command_pool, NULL);

    for (size_t i = 0; i < NUM_MANDELBROT_FRAMES_IN_FLIGHT; i++) {
        vkDestroyFence(device, mandelbrot_fences[i], NULL);
        vkDestroyImageView(device, mandelbrot_color_image_views[i], NULL);
        vmaDestroyImage(allocator, mandelbrot_color_images[i], mandelbrot_color_image_allocations[i]);
    }
}

size_t get_front_frame_index(void) {
    return frame_index;
}