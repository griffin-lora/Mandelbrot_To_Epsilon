#include "gfx/gfx.h"
#include "gfx/pipeline.h"

void destroy_pipeline(const pipeline_t* pipeline) {
    vkDestroyPipeline(device, pipeline->pipeline, NULL);
    vkDestroyPipelineLayout(device, pipeline->pipeline_layout, NULL);
}