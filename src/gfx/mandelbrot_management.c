#include "mandelbrot_management.h"
#include "gfx/default.h"
#include "gfx/gfx.h"
#include "gfx/gfx_util.h"
#include "gfx/mandelbrot_compute_pipeline.h"
#include "gfx/mandelbrot_render_pipeline.h"
#include "result.h"
#include "util.h"
#include <stdint.h>
#include <string.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

static size_t front_frame_index = 0;

VkImage mandelbrot_color_images[NUM_MANDELBROT_FRAMES_IN_FLIGHT];
VkImageView mandelbrot_color_image_views[NUM_MANDELBROT_FRAMES_IN_FLIGHT];
mandelbrot_dispatch_t mandelbrot_dispatches[NUM_MANDELBROT_FRAMES_IN_FLIGHT];
size_t mandelbrot_frame_index_to_render_frame_index[NUM_MANDELBROT_FRAMES_IN_FLIGHT];

static VmaAllocation mandelbrot_color_image_allocations[NUM_MANDELBROT_FRAMES_IN_FLIGHT];
static VkFence mandelbrot_fences[NUM_MANDELBROT_FRAMES_IN_FLIGHT];
static VkCommandBuffer mandelbrot_command_buffers[NUM_MANDELBROT_FRAMES_IN_FLIGHT];

static VkCommandPool mandelbrot_command_pool;

static result_t create_mandelbrot_image(size_t frame_index, uint32_t width, uint32_t height) {
    mandelbrot_dispatches[frame_index] = (mandelbrot_dispatch_t) { width / 8, height / 8 };

    if (vmaCreateImage(allocator, &(VkImageCreateInfo) {
        DEFAULT_VK_IMAGE,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .extent = { width, height, 1 },
        .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
    }, &device_allocation_create_info, &mandelbrot_color_images[frame_index], &mandelbrot_color_image_allocations[frame_index], NULL) != VK_SUCCESS) {
        return result_image_create_failure;
    }

    if (vkCreateImageView(device, &(VkImageViewCreateInfo) {
        DEFAULT_VK_IMAGE_VIEW,
        .image = mandelbrot_color_images[frame_index],
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT
    }, NULL, &mandelbrot_color_image_views[frame_index]) != VK_SUCCESS) {
        return result_image_view_create_failure;
    }

    if (vkCreateFence(device, &(VkFenceCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    }, NULL, &mandelbrot_fences[frame_index]) != VK_SUCCESS) {
        return result_synchronization_primitive_create_failure;
    }

    return result_success;
}

static void destroy_mandelbrot_image(size_t index) {
    vkDestroyFence(device, mandelbrot_fences[index], NULL);
    vkDestroyImageView(device, mandelbrot_color_image_views[index], NULL);
    vmaDestroyImage(allocator, mandelbrot_color_images[index], mandelbrot_color_image_allocations[index]);
}

result_t init_mandelbrot_management(VkCommandBuffer command_buffer, VkFence command_fence, uint32_t graphics_queue_family_index) {
    result_t result;

    memset(mandelbrot_frame_index_to_render_frame_index, 0, sizeof(mandelbrot_frame_index_to_render_frame_index));

    int width;
    int height;
    glfwGetFramebufferSize(window, &width, &height);

    uint32_t ceil_width = ceil_pow2((uint32_t) width, 8);
    uint32_t ceil_height = ceil_pow2((uint32_t) height, 8);

    for (size_t i = 0; i < NUM_MANDELBROT_FRAMES_IN_FLIGHT; i++) {
        if ((result = create_mandelbrot_image(i, ceil_width, ceil_height)) != result_success) {
            return result;
        }
    }

    update_mandelbrot_compute_pipeline(front_frame_index);

    if (vkBeginCommandBuffer(command_buffer, &(VkCommandBufferBeginInfo) {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    }) != VK_SUCCESS) {
        return result_command_buffer_begin_failure;
    }

    for (size_t i = 0; i < NUM_MANDELBROT_FRAMES_IN_FLIGHT; i++) {
        record_mandelbrot_compute_pipeline_init_to_compute_transition(command_buffer, front_frame_index);
    }

    record_mandelbrot_compute_pipeline(command_buffer, front_frame_index);
    
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

result_t manage_mandelbrot_frames() {
    result_t result;

    {
        size_t back_frame_index = (front_frame_index + 1) % NUM_MANDELBROT_FRAMES_IN_FLIGHT;
        VkFence command_fence = mandelbrot_fences[back_frame_index];
        if (vkGetFenceStatus(device, command_fence) != VK_SUCCESS) {
            return result_success;
        }

        update_mandelbrot_render_pipeline(back_frame_index);
    }
    front_frame_index = (front_frame_index + 1) % NUM_MANDELBROT_FRAMES_IN_FLIGHT;
    size_t back_frame_index = (front_frame_index + 1) % NUM_MANDELBROT_FRAMES_IN_FLIGHT;
    
    // Make sure the gpu is not rendering using the mandelbrot back frame
    VkFence render_fence = in_flight_fences[mandelbrot_frame_index_to_render_frame_index[back_frame_index]];
    vkWaitForFences(device, 1, &render_fence, VK_TRUE, UINT64_MAX);

    VkCommandBuffer command_buffer = mandelbrot_command_buffers[back_frame_index];
    if (vkResetCommandBuffer(command_buffer, 0) != VK_SUCCESS) {
        return result_command_buffer_reset_failure;
    }

    if (vkBeginCommandBuffer(command_buffer, &(VkCommandBufferBeginInfo) {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
    }) != VK_SUCCESS) {
        return result_command_buffer_begin_failure;
    }

    int width;
    int height;
    glfwGetFramebufferSize(window, &width, &height);

    uint32_t ceil_width = ceil_pow2((uint32_t) width, 8);
    uint32_t ceil_height = ceil_pow2((uint32_t) height, 8);

    const mandelbrot_dispatch_t* dispatch = &mandelbrot_dispatches[back_frame_index];
    // If we have changed framebuffer size then we create a new mandelbrot color image, make sure to update render pipeline and get it ready for compute
    if (dispatch->width * 8 != ceil_width || dispatch->height * 8 != ceil_height) {
        destroy_mandelbrot_image(back_frame_index);
        if ((result = create_mandelbrot_image(back_frame_index, ceil_width, ceil_height)) != result_success) {
            return result;
        }

        record_mandelbrot_compute_pipeline_init_to_compute_transition(command_buffer, back_frame_index);
    } else {
        record_mandelbrot_compute_pipeline_fragment_to_compute_transition(command_buffer, back_frame_index);
    }

    update_mandelbrot_compute_pipeline(back_frame_index);
    record_mandelbrot_compute_pipeline(command_buffer, back_frame_index);

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
        return result_command_buffer_end_failure;
    }

    VkFence command_fence = mandelbrot_fences[back_frame_index];
    vkResetFences(device, 1, &command_fence);
    if (vkQueueSubmit(queue, 1, &(VkSubmitInfo) {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer
    }, command_fence) != VK_SUCCESS) {
        return result_queue_submit_failure;
    }

    return result_success;
}

void term_mandelbrot_management(void) {
    vkDestroyCommandPool(device, mandelbrot_command_pool, NULL);

    for (size_t i = 0; i < NUM_MANDELBROT_FRAMES_IN_FLIGHT; i++) {
        destroy_mandelbrot_image(i);
    }
}

size_t get_mandelbrot_front_frame_index(void) {
    return front_frame_index;
}