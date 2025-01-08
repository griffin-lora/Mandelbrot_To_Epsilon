#pragma once
#include "result.h"
#include <vk_mem_alloc.h>
#include <stdbool.h>
#include <vulkan/vulkan.h>

result_t create_shader_module(const char* path, VkShaderModule* shader_module);
result_t write_to_buffer(VmaAllocation buffer_allocation, size_t num_bytes, const void* data);

typedef struct {
    VkBuffer buffer;
    VmaAllocation buffer_allocation;
} staging_t;

result_t reset_command_processing(VkCommandBuffer command_buffer, VkFence command_fence);
result_t submit_and_wait(VkQueue queue, VkCommandBuffer command_buffer, VkFence command_fence);