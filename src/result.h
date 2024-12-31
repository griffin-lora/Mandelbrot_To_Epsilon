#pragma once

typedef enum {
    result_success = 0,
    result_failure = 1,

    result_window_create_failure,
    result_instance_create_failure,
    result_surface_create_failure,
    result_device_create_failure,
    result_allocator_create_failure,
    result_synchronization_primitive_create_failure,
    result_swapchain_create_failure,
    result_image_view_create_failure,
    result_framebuffer_create_failure,
    result_command_pool_create_failure,
    result_buffer_create_failure,
    result_image_create_failure,
    result_sampler_create_failure,
    result_render_pass_create_failure,
    result_descriptor_set_layout_create_failure,
    result_descriptor_pool_create_failure,
    result_pipeline_layout_create_failure,
    result_shader_module_create_failure,
    result_graphics_pipelines_create_failure,
    result_compute_pipelines_create_failure,

    result_descriptor_sets_allocate_failure,

    result_command_buffers_allocate_failure,
    result_command_buffer_begin_failure,
    result_command_buffer_end_failure,

    result_swapchain_image_acquire_failure,
    result_queue_submit_failure,
    result_swapchain_image_present_failure,
    result_memory_map_failure,
    result_fences_wait_failure,
    result_fences_reset_failure,
    result_command_buffer_reset_failure,

    result_file_access_failure,
    result_file_open_failure,
    result_file_read_failure,
    
    result_validation_layers_unavailable,
    result_physical_device_support_unavailable,
    result_suitable_physical_device_unavailable,
    result_extension_support_unavailable,

    result_glfw_init_failure
} result_t;

void print_result_error(result_t result);
