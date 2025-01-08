#include "result.h"
#include <stdio.h>
#include <string.h>

static const char* get_result_string(result_t result) {
    switch (result) {
        case result_failure: return "Generic failure";
        
        case result_window_create_failure: return "Failed to create window";
        case result_instance_create_failure: return "Failed to create instance";
        case result_surface_create_failure: return "Failed to create surface";
        case result_device_create_failure: return "Failed to create device";
        case result_allocator_create_failure: return "Failed to create allocator";
        case result_synchronization_primitive_create_failure: return "Failed to create synchronization primitives";
        case result_swapchain_create_failure: return "Failed to create swap chain";
        case result_image_view_create_failure: return "Failed to create image view";
        case result_framebuffer_create_failure: return "Failed to create framebuffer";
        case result_buffer_create_failure: return "Failed to create buffer";
        case result_image_create_failure: return "Failed to create image";
        case result_sampler_create_failure: return "Failed to create texture image sampler";
        case result_render_pass_create_failure: return "Failed to create frame render pass";
        case result_descriptor_set_layout_create_failure: return "Failed to create descriptor set layout";
        case result_descriptor_pool_create_failure: return "Failed to create descriptor pool";
        case result_pipeline_layout_create_failure: return "Failed to create pipeline layout";
        case result_shader_module_create_failure: return "Failed to create shader module";
        case result_graphics_pipelines_create_failure: return "Failed to create graphics pipelines";
        case result_compute_pipelines_create_failure: return "Failed to create compute pipelines";
        case result_query_pool_create_failure: return "Failed to create query pool";

        case result_descriptor_sets_allocate_failure: return "Failed to allocate descriptor sets";

        case result_command_buffers_allocate_failure: return "Failed to allocate command buffers";
        case result_command_buffer_begin_failure: return "Failed to begin command buffer"; 
        case result_command_buffer_end_failure: return "Failed to end command buffer"; 
        
        case result_swapchain_image_acquire_failure: return "Failed to acquire swapchain image";
        case result_queue_submit_failure: return "Failed to submit to graphics queue";
        case result_swapchain_image_present_failure: return "Failed to present swap chain image";
        case result_memory_map_failure: return "Failed to map buffer memory";
        case result_fences_wait_failure: return "Faled to wait for fences";
        case result_fences_reset_failure: return "Failed to reset fences";
        case result_command_buffer_reset_failure: return "Failed to command buffer";

        case result_file_access_failure: return "Failed to access file";
        case result_file_open_failure: return "Failed to open file";
        case result_file_read_failure: return "Failed to read file";

        case result_validation_layers_unavailable: return "Validation layers requested, but not available";
        case result_physical_device_support_unavailable: return "Failed to find physical devices with Vulkan support";
        case result_suitable_physical_device_unavailable: return "Failed to get a suitable physical device";
        case result_extension_support_unavailable: return "Failed to get a supported extension";

        case result_glfw_init_failure: return "Failed to initialize GLFW";

        default: return NULL;
    }
}

void print_result_error(result_t result) {
    printf("%s\n", get_result_string(result));
}
