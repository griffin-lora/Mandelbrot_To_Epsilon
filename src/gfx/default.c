#include "default.h"
#include <stdalign.h>

alignas(64)

const VkPipelineInputAssemblyStateCreateInfo default_input_assembly_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    .primitiveRestartEnable = VK_FALSE
};

const VkPipelineViewportStateCreateInfo default_viewport_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .scissorCount = 1
};

const VkPipelineColorBlendStateCreateInfo default_color_blend_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .logicOpEnable = VK_FALSE,
    .attachmentCount = 1,
    .pAttachments = &(VkPipelineColorBlendAttachmentState) {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_FALSE
    }
};

const VkPipelineDynamicStateCreateInfo default_dynamic_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .dynamicStateCount = 2,
    .pDynamicStates = (VkDynamicState[2]) { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR }
};

const VkBufferCreateInfo vertex_buffer_create_info = {
    DEFAULT_VK_VERTEX_BUFFER
};

const VkBufferCreateInfo index_buffer_create_info = {
    DEFAULT_VK_BUFFER,
    .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
};

const VkBufferCreateInfo uniform_buffer_create_info = {
    DEFAULT_VK_BUFFER,
    .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
};

const VmaAllocationCreateInfo shared_write_allocation_create_info = {
    DEFAULT_VMA_ALLOCATION,
    .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
    .requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
};

const VmaAllocationCreateInfo shared_read_allocation_create_info = {
    DEFAULT_VMA_ALLOCATION,
    .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
    .requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
};

const VmaAllocationCreateInfo device_allocation_create_info = {
    DEFAULT_VMA_ALLOCATION,
    .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
};