#include "mandelbrot_render_pipeline.h"
#include "gfx.h"
#include "gfx/default.h"
#include "gfx/pipeline.h"
#include "gfx/gfx_util.h"
#include "result.h"
#include <cglm/types-struct.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

typedef struct {
    float aspect;
} push_constants_t;

static vec2s mandelbrot_vertices[4] = {
    {{ 1.0f, 1.0f }},
    {{ 1.0f, -1.0f }},
    {{ -1.0f, -1.0f }},
    {{ -1.0f, 1.0f }},
};

static uint16_t mandelbrot_indices[6] = {
    0, 1, 3,
    1, 2, 3
};

static pipeline_t pipeline;
// static VkDescriptorSetLayout descriptor_set_layout;
// static VkDescriptorSet descriptor_set;

static VkBuffer vertex_staging_buffer;
static VmaAllocation vertex_staging_buffer_allocation;
static VkBuffer vertex_buffer;
static VmaAllocation vertex_buffer_allocation;

static VkBuffer index_staging_buffer;
static VmaAllocation index_staging_buffer_allocation;
static VkBuffer index_buffer;
static VmaAllocation index_buffer_allocation;

result_t init_mandelbrot_render_pipeline(VkCommandBuffer command_buffer, VkFence command_fence, VkDescriptorPool descriptor_pool, const VkPhysicalDeviceProperties* physical_device_properties) {
    (void) physical_device_properties;
    (void) descriptor_pool;

    result_t result;
    
    VkShaderModule vertex_shader_module;
    if ((result = create_shader_module("shader/mandelbrot_vertex.spv", &vertex_shader_module)) != result_success) {
        return result;
    }
    VkShaderModule fragment_shader_module;
    if ((result = create_shader_module("shader/mandelbrot_fragment.spv", &fragment_shader_module)) != result_success) {
        return result;
    }

    // if (vkCreateDescriptorSetLayout(device, &(VkDescriptorSetLayoutCreateInfo) {
    //     .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
    //     .bindingCount = 0
    // }, NULL, &descriptor_set_layout) != VK_SUCCESS) {
    //     return result_descriptor_set_layout_create_failure;
    // }

    // if (vkAllocateDescriptorSets(device, &(VkDescriptorSetAllocateInfo) {
    //     .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
    //     .descriptorPool = descriptor_pool,
    //     .descriptorSetCount = 0,
    //     .pSetLayouts = &descriptor_set_layout
    // }, &descriptor_set) != VK_SUCCESS) {
    //     return result_descriptor_sets_allocate_failure;
    // }
    
    // vkUpdateDescriptorSets(device, 1, (VkWriteDescriptorSet[1]) {
    //     {
    //         .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    //         .dstSet = descriptor_set,
    //         .dstBinding = 0,
    //         .dstArrayElement = 0,
    //         .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    //         .descriptorCount = 1
    //     }
    // }, 0, NULL);
    
    if (vkCreatePipelineLayout(device, &(VkPipelineLayoutCreateInfo) {
        DEFAULT_VK_PIPELINE_LAYOUT,
        .setLayoutCount = 0,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &(VkPushConstantRange) {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            .size = sizeof(push_constants_t)
        }
    }, NULL, &pipeline.pipeline_layout) != VK_SUCCESS) {
        return result_pipeline_layout_create_failure;
    }

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &(VkGraphicsPipelineCreateInfo) {
        DEFAULT_VK_GRAPHICS_PIPELINE,

        .stageCount = 2,
        .pStages = (VkPipelineShaderStageCreateInfo[2]) {
            {
                DEFAULT_VK_SHADER_STAGE,
                .stage = VK_SHADER_STAGE_VERTEX_BIT,
                .module = vertex_shader_module
            },
            {
                DEFAULT_VK_SHADER_STAGE,
                .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                .module = fragment_shader_module
            }
        },

        .pVertexInputState = &(VkPipelineVertexInputStateCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = (VkVertexInputBindingDescription[1]) {
                {
                    .binding = 0,
                    .stride = sizeof(vec2s),
                    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
                }
            },

            .vertexAttributeDescriptionCount = 1,
            .pVertexAttributeDescriptions = (VkVertexInputAttributeDescription[1]) {
                {
                    .binding = 0,
                    .location = 0,
                    .format = VK_FORMAT_R32G32_SFLOAT,
                    .offset = 0
                }
            }
        },
        .pRasterizationState = &(VkPipelineRasterizationStateCreateInfo) { DEFAULT_VK_RASTERIZATION },
        .pMultisampleState = &(VkPipelineMultisampleStateCreateInfo) {
            DEFAULT_VK_MULTISAMPLE,
            .rasterizationSamples = render_multisample_flags
        },
        .layout = pipeline.pipeline_layout,
        .renderPass = frame_render_pass
    }, NULL, &pipeline.pipeline) != VK_SUCCESS) {
        return result_graphics_pipelines_create_failure;
    }
    
    vkDestroyShaderModule(device, fragment_shader_module, NULL);
    vkDestroyShaderModule(device, vertex_shader_module, NULL);

    if (vmaCreateBuffer(allocator, &(VkBufferCreateInfo) {
        DEFAULT_VK_VERTEX_BUFFER,
        .size = sizeof(mandelbrot_vertices)
    }, &device_allocation_create_info, &vertex_buffer, &vertex_buffer_allocation, NULL) != VK_SUCCESS) {
        return result_buffer_create_failure;
    }

    if (vmaCreateBuffer(allocator, &(VkBufferCreateInfo) {
        DEFAULT_VK_INDEX_BUFFER,
        .size = sizeof(mandelbrot_indices)
    }, &device_allocation_create_info, &index_buffer, &index_buffer_allocation, NULL) != VK_SUCCESS) {
        return result_buffer_create_failure;
    }

    if (vmaCreateBuffer(allocator, &(VkBufferCreateInfo) {
        DEFAULT_VK_STAGING_BUFFER,
        .size = sizeof(mandelbrot_vertices)
    }, &shared_write_allocation_create_info, &vertex_staging_buffer, &vertex_staging_buffer_allocation, NULL) != VK_SUCCESS) {
        return result_buffer_create_failure;
    }

    if (vmaCreateBuffer(allocator, &(VkBufferCreateInfo) {
        DEFAULT_VK_STAGING_BUFFER,
        .size = sizeof(mandelbrot_indices)
    }, &shared_write_allocation_create_info, &index_staging_buffer, &index_staging_buffer_allocation, NULL) != VK_SUCCESS) {
        return result_buffer_create_failure;
    }
    
    const void* mandelbrot_vertex_array = mandelbrot_vertices;
    if ((result = writes_to_buffer(vertex_staging_buffer_allocation, sizeof(mandelbrot_vertices), 1, &mandelbrot_vertex_array)) != result_success) {
        return result;
    }
    
    const void* mandelbrot_index_array = mandelbrot_indices;
    if ((result = writes_to_buffer(index_staging_buffer_allocation, sizeof(mandelbrot_indices), 1, &mandelbrot_index_array)) != result_success) {
        return result;
    }

    if (vkBeginCommandBuffer(command_buffer, &(VkCommandBufferBeginInfo) {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    }) != VK_SUCCESS) {
        return result_command_buffer_begin_failure;
    }

    vkCmdCopyBuffer(command_buffer, vertex_staging_buffer, vertex_buffer, 1, &(VkBufferCopy) {
        .size = sizeof(mandelbrot_vertices)
    });
    vkCmdCopyBuffer(command_buffer, index_staging_buffer, index_buffer, 1, &(VkBufferCopy) {
        .size = sizeof(mandelbrot_indices)
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

    vmaDestroyBuffer(allocator, index_staging_buffer, index_staging_buffer_allocation);
    vmaDestroyBuffer(allocator, vertex_staging_buffer, vertex_staging_buffer_allocation);

    return result_success;
}

result_t draw_mandelbrot_render_pipeline(VkCommandBuffer command_buffer) {
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);

    push_constants_t push_constants;
    push_constants.aspect = 640.0f/480.0f;
    vkCmdPushConstants(command_buffer, pipeline.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push_constants), &push_constants);

    // vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline_layout, 0, 1, (VkDescriptorSet[1]) { descriptor_set }, 0, NULL);
    vkCmdBindVertexBuffers(command_buffer, 0, 1, &vertex_buffer, (VkDeviceSize[1]) { 0 });
    vkCmdBindIndexBuffer(command_buffer, index_buffer, 0, VK_INDEX_TYPE_UINT16);

    // vkCmdDraw(command_buffer, 3, 1, 0, 0);

    vkCmdDrawIndexed(command_buffer, 6, 1, 0, 0, 0);

    return result_success;
}

void term_mandelbrot_render_pipeline() {
    vmaDestroyBuffer(allocator, index_buffer, index_buffer_allocation);
    vmaDestroyBuffer(allocator, vertex_buffer, vertex_buffer_allocation);
    destroy_pipeline(&pipeline);
    // vkDestroyDescriptorSetLayout(device, descriptor_set_layout, NULL);
}