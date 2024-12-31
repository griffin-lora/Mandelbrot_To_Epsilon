#pragma once
#include "result.h"
#include <vk_mem_alloc.h>
#include <stdbool.h>
#include <vulkan/vulkan.h>

result_t create_shader_module(const char* path, VkShaderModule* shader_module);
result_t write_to_buffer(VmaAllocation buffer_allocation, size_t num_bytes, const void* data);
result_t writes_to_buffer(VmaAllocation buffer_allocation, size_t num_write_bytes, size_t num_writes, const void* const data_array[]);

typedef struct {
    VkBuffer buffer;
    VmaAllocation buffer_allocation;
} staging_t;

result_t create_image(VkCommandBuffer command_buffer, VkFence command_fence, const VkImageCreateInfo* image_create_info, VkDeviceSize num_pixel_bytes, const void* const* pixel_arrays, VkImage* image, VmaAllocation* image_allocation);

void begin_pipeline(
    VkCommandBuffer command_buffer,
    VkFramebuffer image_framebuffer, VkExtent2D image_extent,
    uint32_t num_clear_values, const VkClearValue clear_values[],
    VkRenderPass render_pass, VkDescriptorSet descriptor_set, VkPipelineLayout pipeline_layout, VkPipeline pipeline
);

void end_pipeline(VkCommandBuffer command_buffer);

result_t reset_command_processing(VkCommandBuffer command_buffer, VkFence command_fence);
result_t submit_and_wait(VkCommandBuffer command_buffer, VkFence command_fence);