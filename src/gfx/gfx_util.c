#include "gfx_util.h"
#include "gfx.h"
#include "default.h"
#include "result.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <malloc.h>
#include <vulkan/vulkan.h>

result_t create_shader_module(const char* path, VkShaderModule* shader_module) {
    if (access(path, F_OK) != 0) {
        return result_file_access_failure;
    }

    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        return result_file_open_failure;
    }

    struct stat st;
    stat(path, &st);

    size_t num_bytes = (size_t)st.st_size;
    size_t aligned_num_bytes = num_bytes % 32ul == 0 ? num_bytes : num_bytes + (32ul - (num_bytes % 32ul));

    uint32_t* bytes = memalign(64, aligned_num_bytes);
    memset(bytes, 0, aligned_num_bytes);
    if (fread(bytes, num_bytes, 1, file) != 1) {
        return result_file_read_failure;
    }

    fclose(file);

    if (vkCreateShaderModule(device, &(VkShaderModuleCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = num_bytes,
        .pCode = bytes
    }, NULL, shader_module) != VK_SUCCESS) {
        return result_shader_module_create_failure;
    }

    free(bytes);
    return result_success;
}

result_t write_to_buffer(VmaAllocation buffer_allocation, size_t num_bytes, const void* data) {
    void* mapped_data;
    if (vmaMapMemory(allocator, buffer_allocation, &mapped_data) != VK_SUCCESS) {
        return result_memory_map_failure;
    }
    memcpy(mapped_data, data, num_bytes);
    vmaUnmapMemory(allocator, buffer_allocation);

    return result_success;
}

result_t reset_command_processing(VkCommandBuffer command_buffer, VkFence command_fence) {
    if (vkResetFences(device, 1, &command_fence) != VK_SUCCESS) {
        return result_fences_reset_failure;
    }

    if (vkResetCommandBuffer(command_buffer, 0) != VK_SUCCESS) {
        return result_command_buffer_reset_failure;
    }
    
    return result_success;
}

result_t submit_and_wait(VkQueue queue, VkCommandBuffer command_buffer, VkFence command_fence) {
    if (vkQueueSubmit(queue, 1, &(VkSubmitInfo) {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer
    }, command_fence) != VK_SUCCESS) {
        return result_queue_submit_failure;
    }

    if (vkWaitForFences(device, 1, &command_fence, VK_TRUE, UINT64_MAX) != VK_SUCCESS) {
        return result_fences_wait_failure;
    }

    return result_success;
}