#include "mandelbrot_compute_pipeline.h"
#include "gfx/default.h"
#include "gfx/gfx.h"
#include "gfx/gfx_util.h"
#include "gfx/mandelbrot_management.h"
#include "gfx/pipeline.h"
#include "result.h"
#include <cglm/types-struct.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

static pipeline_t pipeline;

static VkDescriptorSetLayout descriptor_set_layout;
static VkDescriptorSet descriptor_set;

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

    return result_success;
}

void update_mandelbrot_compute_pipeline(size_t frame_index) {
    vkUpdateDescriptorSets(device, 1, (VkWriteDescriptorSet[1]) {
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptor_set,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount = 1,
            .pImageInfo = &(VkDescriptorImageInfo) {
                .imageView = mandelbrot_color_image_views[frame_index],
                .imageLayout = VK_IMAGE_LAYOUT_GENERAL
            }
        },
    }, 0, NULL);
}

void record_mandelbrot_compute_pipeline_init_to_compute_transition(VkCommandBuffer command_buffer, size_t frame_index) {
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &(VkImageMemoryBarrier) {
        DEFAULT_VK_IMAGE_MEMORY_BARRIER,
        .image = mandelbrot_color_images[frame_index],
        .newLayout = VK_IMAGE_LAYOUT_GENERAL,
        .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT
    });
}

void record_mandelbrot_compute_pipeline_fragment_to_compute_transition(VkCommandBuffer command_buffer, size_t frame_index) {
    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &(VkImageMemoryBarrier) {
        DEFAULT_VK_IMAGE_MEMORY_BARRIER,
        .image = mandelbrot_color_images[frame_index],
        .oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_GENERAL,
        .srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT
    });
}

void record_mandelbrot_compute_pipeline(VkCommandBuffer command_buffer, size_t frame_index) {
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline.pipeline_layout, 0, 1, &descriptor_set, 0, NULL);
    
    const mandelbrot_dispatch_t* dispatch = &mandelbrot_dispatches[frame_index];

    vkCmdDispatch(command_buffer, dispatch->width, dispatch->height, 1);

    vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &(VkImageMemoryBarrier) {
        DEFAULT_VK_IMAGE_MEMORY_BARRIER,
        .image = mandelbrot_color_images[frame_index],
        .oldLayout = VK_IMAGE_LAYOUT_GENERAL,
        .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT
    });
}

void term_mandelbrot_compute_pipeline(void) {
    destroy_pipeline(&pipeline);

    vkDestroyDescriptorSetLayout(device, descriptor_set_layout, NULL);
}