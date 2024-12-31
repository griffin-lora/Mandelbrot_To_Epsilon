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

result_t writes_to_buffer(VmaAllocation buffer_allocation, size_t num_write_bytes, size_t num_writes, const void* const data_array[]) {
    void* mapped_data;
    if (vmaMapMemory(allocator, buffer_allocation, &mapped_data) != VK_SUCCESS) {
        return result_memory_map_failure;
    }
    for (size_t i = 0; i < num_writes; i++) {
        memcpy(mapped_data + (i*num_write_bytes), data_array[i], num_write_bytes);
    }
    vmaUnmapMemory(allocator, buffer_allocation);

    return result_success;
}

result_t create_image(VkCommandBuffer command_buffer, VkFence command_fence, const VkImageCreateInfo* image_create_info, VkDeviceSize num_pixel_bytes, const void* const* pixel_arrays, VkImage* image, VmaAllocation* image_allocation) {
    result_t result;

    staging_t staging;

    uint32_t width = image_create_info->extent.width;
    uint32_t height = image_create_info->extent.height;
    uint32_t num_mip_levels = image_create_info->mipLevels;

    VkDeviceSize num_layer_bytes = image_create_info->extent.width * image_create_info->extent.height * num_pixel_bytes;
    uint32_t num_layers = image_create_info->arrayLayers;
    VkDeviceSize num_image_bytes = num_layer_bytes * num_layers;
    
    if (vmaCreateBuffer(allocator, &(VkBufferCreateInfo) {
        DEFAULT_VK_STAGING_BUFFER,
        .size = num_image_bytes
    }, &shared_write_allocation_create_info, &staging.buffer, &staging.buffer_allocation, NULL) != VK_SUCCESS) {
        return result_buffer_create_failure;
    }

    if (vmaCreateImage(allocator, image_create_info, &device_allocation_create_info, image, image_allocation, NULL) != VK_SUCCESS) {
        return result_image_create_failure;
    }

    if ((result = writes_to_buffer(staging.buffer_allocation, num_layer_bytes, num_layers, pixel_arrays)) != result_success) {
        return result;
    }

    if (vkBeginCommandBuffer(command_buffer, &(VkCommandBufferBeginInfo) {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    }) != VK_SUCCESS) {
        return result_command_buffer_begin_failure;
    }

    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &(VkImageMemoryBarrier) {
        DEFAULT_VK_IMAGE_MEMORY_BARRIER,
        .image = *image,
        .subresourceRange.levelCount = num_mip_levels,
        .subresourceRange.layerCount = num_layers,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT
    });
    
    vkCmdCopyBufferToImage(command_buffer, staging.buffer, *image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &(VkBufferImageCopy) {
        DEFAULT_VK_BUFFER_IMAGE_COPY,
        .imageSubresource.layerCount = num_layers,
        .imageExtent.width = width,
        .imageExtent.height = height
    });

    int32_t mip_width = (int32_t)width;
    int32_t mip_height = (int32_t)height;

    for (uint32_t i = 1; i < num_mip_levels; i++) {
        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &(VkImageMemoryBarrier) {
            DEFAULT_VK_IMAGE_MEMORY_BARRIER,
            .image = *image,
            .subresourceRange.baseMipLevel = i - 1,
            .subresourceRange.layerCount = num_layers,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT
        });

        vkCmdBlitImage(command_buffer, *image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, *image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &(VkImageBlit) {
            DEFAULT_VK_IMAGE_BLIT,
            .srcOffsets[1] = { mip_width, mip_height, 1 },
            .srcSubresource.mipLevel = i - 1,
            .srcSubresource.layerCount = num_layers,
            .dstOffsets[1] = { mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1 },
            .dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .dstSubresource.mipLevel = i,
            .dstSubresource.layerCount = num_layers
        }, VK_FILTER_LINEAR);

        vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &(VkImageMemoryBarrier) {
            DEFAULT_VK_IMAGE_MEMORY_BARRIER,
            .image = *image,
            .subresourceRange.baseMipLevel = i - 1,
            .subresourceRange.layerCount = num_layers,
            .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT
        });

        if (mip_width > 1) { mip_width /= 2; }
        if (mip_height > 1) { mip_height /= 2; }
    }

    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &(VkImageMemoryBarrier) {
        DEFAULT_VK_IMAGE_MEMORY_BARRIER,
        .image = *image,
        .subresourceRange.baseMipLevel = num_mip_levels - 1,
        .subresourceRange.layerCount = num_layers,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT
    });

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
        return result_command_buffer_end_failure;
    }

    if ((result = submit_and_wait(command_buffer, command_fence)) != result_success) {
        return result;
    }
    if ((result = reset_command_processing(command_buffer, command_fence)) != result_success) {
        return result;
    }
    
    vmaDestroyBuffer(allocator, staging.buffer, staging.buffer_allocation);

    return result_success;
}

void begin_pipeline(
    VkCommandBuffer command_buffer,
    VkFramebuffer image_framebuffer, VkExtent2D image_extent,
    uint32_t num_clear_values, const VkClearValue clear_values[],
    VkRenderPass render_pass, VkDescriptorSet descriptor_set, VkPipelineLayout pipeline_layout, VkPipeline pipeline
) {
    vkCmdBeginRenderPass(command_buffer, &(VkRenderPassBeginInfo) {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = render_pass,
        .framebuffer = image_framebuffer,
        .renderArea.offset = { 0, 0 },
        .renderArea.extent = image_extent,
        .clearValueCount = num_clear_values,
        .pClearValues = clear_values
    }, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    
    if (descriptor_set != NULL) {
        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_set, 0, NULL);
    }

    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)image_extent.width,
        .height = (float)image_extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = { 0, 0 },
        .extent = image_extent
    };
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);
}

void end_pipeline(VkCommandBuffer command_buffer) {
    vkCmdEndRenderPass(command_buffer);
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

result_t submit_and_wait(VkCommandBuffer command_buffer, VkFence command_fence) {
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