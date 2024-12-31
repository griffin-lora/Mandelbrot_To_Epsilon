#pragma once
#include <vulkan/vulkan.h>

typedef struct {
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;
} pipeline_t;

void destroy_pipeline(const pipeline_t* pipeline);