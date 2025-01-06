#include "mandelbrot_compute_pipeline.h"
#include "gfx/default.h"
#include "gfx/gfx.h"
#include "gfx/gfx_util.h"
#include "gfx/pipeline.h"
#include "result.h"
#include <cglm/types-struct.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

static pipeline_t pipeline;

static VkDescriptorSetLayout descriptor_set_layout;
static VkDescriptorSet descriptor_set;

VkImage mandelbrot_color_image;
VkImageView mandelbrot_color_image_view;
VmaAllocation mandelbrot_color_image_allocation;

result_t init_mandelbrot_compute_pipeline(VkDescriptorPool descriptor_pool) {
    result_t result;

    VkShaderModule shader_module;
    if ((result = create_shader_module("shader/mandelbrot.spv", &shader_module)) != result_success) {
        return result;
    }

    if (vkCreateDescriptorSetLayout(device, &(VkDescriptorSetLayoutCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = (VkDescriptorSetLayoutBinding[1]) {
            {
                DEFAULT_VK_DESCRIPTOR_BINDING,
                .binding = 0,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
            }
        }
    }, NULL, &descriptor_set_layout) != VK_SUCCESS) {
        return result_descriptor_set_layout_create_failure;
    }

    if (vkAllocateDescriptorSets(device, &(VkDescriptorSetAllocateInfo) {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &descriptor_set_layout
    }, &descriptor_set) != VK_SUCCESS) {
        return result_descriptor_sets_allocate_failure;
    }

    if (vkCreatePipelineLayout(device, &(VkPipelineLayoutCreateInfo) {
        DEFAULT_VK_PIPELINE_LAYOUT,
        .setLayoutCount = 1,
        .pSetLayouts = (VkDescriptorSetLayout[1]) {
            descriptor_set_layout
        },
    }, NULL, &pipeline.pipeline_layout) != VK_SUCCESS) {
        return result_pipeline_layout_create_failure;
    }

    if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &(VkComputePipelineCreateInfo) {
        DEFAULT_VK_COMPUTE_PIPELINE,
        .stage = {
            DEFAULT_VK_SHADER_STAGE,
            .stage = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = shader_module
        },
        .layout = pipeline.pipeline_layout
    }, NULL, &pipeline.pipeline) != VK_SUCCESS) {
        return result_compute_pipelines_create_failure;
    }

    vkDestroyShaderModule(device, shader_module, NULL);

    if (vmaCreateImage(allocator, &(VkImageCreateInfo) {
        DEFAULT_VK_IMAGE,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .extent = { 640, 640, 1 },
        .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
    }, &device_allocation_create_info, &mandelbrot_color_image, &mandelbrot_color_image_allocation, NULL) != VK_SUCCESS) {
        return result_image_create_failure;
    }

    if (vkCreateImageView(device, &(VkImageViewCreateInfo) {
        DEFAULT_VK_IMAGE_VIEW,
        .image = mandelbrot_color_image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT
    }, NULL, &mandelbrot_color_image_view) != VK_SUCCESS) {
        return result_image_view_create_failure;
    }
    
    vkUpdateDescriptorSets(device, 1, (VkWriteDescriptorSet[1]) {
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptor_set,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
            .pImageInfo = &(VkDescriptorImageInfo) {
                .imageView = mandelbrot_color_image_view,
                .imageLayout = VK_IMAGE_LAYOUT_GENERAL
            }
        },
    }, 0, NULL);

    return result_success;
}

result_t record_mandelbrot_compute_pipeline(VkCommandBuffer command_buffer) {
    if (vkBeginCommandBuffer(command_buffer, &(VkCommandBufferBeginInfo) {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    }) != VK_SUCCESS) {
        return result_command_buffer_begin_failure;
    }

    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &(VkImageMemoryBarrier) {
        DEFAULT_VK_IMAGE_MEMORY_BARRIER,
        .image = mandelbrot_color_image,
        .newLayout = VK_IMAGE_LAYOUT_GENERAL,
        .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT
    });
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline_layout, 0, 1, &descriptor_set, 0, NULL);

    vkCmdDispatch(command_buffer, 10, 10, 1);

    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &(VkImageMemoryBarrier) {
        DEFAULT_VK_IMAGE_MEMORY_BARRIER,
        .image = mandelbrot_color_image,
        .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT
    });

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
        return result_command_buffer_end_failure;
    }

    return result_success;
}

void term_mandelbrot_compute_pipeline(void) {
    destroy_pipeline(&pipeline);

    vkDestroyImageView(device, mandelbrot_color_image_view, NULL);
    vmaDestroyImage(allocator, mandelbrot_color_image, mandelbrot_color_image_allocation);
    vkDestroyDescriptorSetLayout(device, descriptor_set_layout, NULL);
}