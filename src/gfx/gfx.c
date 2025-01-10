#include "gfx.h"
#include "camera.h"
#include "chrono.h"
#include "gfx/default.h"
#include "gfx/gfx_util.h"
#include "gfx/mandelbrot_compute_pipeline.h"
#include "gfx/mandelbrot_management.h"
#include "gfx/mandelbrot_render_pipeline.h"
#include "result.h"
#include "util.h"
#include <GLFW/glfw3.h>
#include <cglm/struct/mat3.h>
#include <cglm/types-struct.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480

typedef union {
    uint32_t data[2];
    struct {
        uint32_t graphics;
        uint32_t presentation;
    };
} queue_family_indices_t;

GLFWwindow* window;
VkDevice device;
static VkQueue graphics_queue;
static VkPhysicalDevice physical_device;
static VkPhysicalDeviceProperties physical_device_properties;
VmaAllocator allocator;
static queue_family_indices_t queue_family_indices;
VkSurfaceFormatKHR surface_format;
static VkPresentModeKHR present_mode;
static VkSemaphore image_available_semaphores[NUM_FRAMES_IN_FLIGHT];
static VkSemaphore render_finished_semaphores[NUM_FRAMES_IN_FLIGHT];
VkFence in_flight_fences[NUM_FRAMES_IN_FLIGHT];
static uint32_t num_swapchain_images;
static VkImage* swapchain_images;
static VkImageView* swapchain_image_views;
static VkFramebuffer* swapchain_framebuffers;
static VkSwapchainKHR swapchain;
static VkInstance instance;
static VkSurfaceKHR surface;
static VkExtent2D swap_image_extent;
static VkQueue presentation_queue;
static bool framebuffer_resized;
static VkCommandPool command_pool;

VkSampleCountFlagBits render_multisample_flags;

VkRenderPass frame_render_pass;
static VkCommandBuffer frame_command_buffers[NUM_FRAMES_IN_FLIGHT];

static size_t frame_index = 0;

static VkImage frame_image;
static VmaAllocation frame_image_allocation;
static VkImageView frame_image_view;

static VkCommandBuffer generic_command_buffer;
static VkFence generic_command_fence;

static VkDescriptorPool generic_descriptor_pool;

static VkQueryPool timestamp_query_pool;

static const char* layers[] = {
    "VK_LAYER_KHRONOS_validation"
};

static const char* extensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_SPIRV_1_4_EXTENSION_NAME
};

static result_t check_layers(void) {
    uint32_t num_available_layers;
    vkEnumerateInstanceLayerProperties(&num_available_layers, NULL);
    
    VkLayerProperties available_layers[num_available_layers];
    vkEnumerateInstanceLayerProperties(&num_available_layers, available_layers);

    for (size_t i = 0; i < NUM_ELEMS(layers); i++) {
        bool not_found = true;
        for (size_t j = 0; j < num_available_layers; j++) {
            if (strcmp(layers[i], available_layers[j].layerName) == 0) {
                not_found = false;
                break;
            }
        }

        if (not_found) {
            return result_validation_layers_unavailable;
        }
    }

    return result_success;
}

static result_t check_extensions(VkPhysicalDevice physical_device) {
    uint32_t num_available_extensions;
    vkEnumerateDeviceExtensionProperties(physical_device, NULL, &num_available_extensions, NULL);
    
    VkExtensionProperties available_extensions[num_available_extensions];
    vkEnumerateDeviceExtensionProperties(physical_device, NULL, &num_available_extensions, available_extensions);

    for (size_t i = 0; i < NUM_ELEMS(extensions); i++) {
        bool not_found = true;
        for (size_t j = 0; j < num_available_extensions; j++) {
            if (strcmp(extensions[i], available_extensions[j].extensionName) == 0) {
                not_found = false;
                break;
            }
        }

        if (not_found) {
            return result_extension_support_unavailable;
        }
    }

    return result_success;
}

static uint32_t get_graphics_queue_family_index(uint32_t num_queue_families, const VkQueueFamilyProperties queue_families[]) {
    for (uint32_t i = 0; i < num_queue_families; i++) {
        if ((queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT)) {
            return i;
        }
    }
    return NULL_UINT32;
}

static uint32_t get_presentation_queue_family_index(VkPhysicalDevice physical_device, uint32_t num_queue_families, const VkQueueFamilyProperties queue_families[]) {
    for (uint32_t i = 0; i < num_queue_families; i++) {
        if (!(queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
            continue;
        }

        VkBool32 presentation_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &presentation_support);
        if (presentation_support) {
            return i;
        }
    }
    return NULL_UINT32;
}

static result_t get_physical_device(uint32_t num_physical_devices, const VkPhysicalDevice physical_devices[], VkPhysicalDevice* out_physical_device, uint32_t* out_num_surface_formats, uint32_t* out_num_present_modes, queue_family_indices_t* out_queue_family_indices) {
    result_t result = result_suitable_physical_device_unavailable;

    printf("Physical devices: %d\n", num_physical_devices);
    for (size_t i = 0; i < num_physical_devices; i++) {
        VkPhysicalDevice physical_device = physical_devices[i];
        
        VkPhysicalDeviceProperties physical_device_properties;
        vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);
        printf("Physical device: %s\n", physical_device_properties.deviceName);
    }

    for (size_t i = 0; i < num_physical_devices; i++) {
        VkPhysicalDevice physical_device = physical_devices[i];

        // RenderDoc loads llvmpipe for some reason so this forces it to load the correct physical device
        VkPhysicalDeviceProperties physical_device_properties;
        vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);
        if (physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU || physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU || physical_device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_OTHER) {
            continue;
        }
        if (physical_device_properties.limits.timestampPeriod == 0) {
            continue;
        }

        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(physical_device, &features);

        if (!features.samplerAnisotropy) {
            continue;
        }

        if ((result = check_extensions(physical_device)) != result_success) {
            continue;
        }
        
        uint32_t num_surface_formats;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &num_surface_formats, NULL);

        if (num_surface_formats == 0) {
            continue;
        }

        uint32_t num_present_modes;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &num_present_modes, NULL);

        if (num_present_modes == 0) {
            continue;
        }

        uint32_t num_queue_families;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &num_queue_families, NULL);

        VkQueueFamilyProperties queue_families[num_queue_families];
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &num_queue_families, queue_families);

        uint32_t graphics_queue_family_index = get_graphics_queue_family_index(num_queue_families, queue_families);
        if (graphics_queue_family_index == NULL_UINT32) {
            continue;
        }
        if (!physical_device_properties.limits.timestampComputeAndGraphics) {
            if (queue_families[graphics_queue_family_index].timestampValidBits == 0) {
                continue;
            }
        }

        uint32_t presentation_queue_family_index = get_presentation_queue_family_index(physical_device, num_queue_families, queue_families);
        if (presentation_queue_family_index == NULL_UINT32) {
            break;
        }

        *out_physical_device = physical_device;
        *out_num_surface_formats = num_surface_formats;
        *out_num_present_modes = num_present_modes;
        *out_queue_family_indices = (queue_family_indices_t) {{ graphics_queue_family_index, presentation_queue_family_index }};
        return result_success;
    }
    return result;
}

static VkSurfaceFormatKHR get_surface_format(uint32_t num_surface_formats, const VkSurfaceFormatKHR surface_formats[]) {
    for (size_t i = 0; i < num_surface_formats; i++) {
        VkSurfaceFormatKHR surface_format = surface_formats[i];
        if (surface_format.format == VK_FORMAT_B8G8R8A8_SRGB && surface_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return surface_format;
        }
    }

    return surface_formats[0];
}

static VkPresentModeKHR get_present_mode(uint32_t num_present_modes, const VkPresentModeKHR present_modes[]) {
    for (size_t i = 0; i < num_present_modes; i++) {
        VkPresentModeKHR present_mode = present_modes[i];
        if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return present_mode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

static VkExtent2D get_swap_image_extent(const VkSurfaceCapabilitiesKHR* capabilities) {
    if (capabilities->currentExtent.width != NULL_UINT32) {
        return capabilities->currentExtent;
    }

    int width;
    int height;
    glfwGetFramebufferSize(window, &width, &height);
    VkExtent2D extent = { (uint32_t)width, (uint32_t)height };

    extent.width = clamp_uint32(extent.width, capabilities->minImageExtent.width, capabilities->maxImageExtent.width);
    extent.height = clamp_uint32(extent.height, capabilities->minImageExtent.height, capabilities->maxImageExtent.height);

    return extent;
}

static result_t init_swapchain(void) {
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &capabilities);
    swap_image_extent = get_swap_image_extent(&capabilities); // NOTE: Not actually a problem? (https://github.com/KhronosGroup/Vulkan-ValidationLayers/issues/1340)

    uint32_t min_num_swapchain_images = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0) {
        min_num_swapchain_images = clamp_uint32(min_num_swapchain_images, 0, capabilities.maxImageCount);
    }

    VkSwapchainCreateInfoKHR info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = min_num_swapchain_images,
        .imageFormat = surface_format.format,
        .imageColorSpace = surface_format.colorSpace,
        .imageExtent = swap_image_extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = present_mode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };

    if (queue_family_indices.graphics != queue_family_indices.presentation) {
        info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        info.queueFamilyIndexCount = 2;
        info.pQueueFamilyIndices = queue_family_indices.data;
    } else {
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        info.queueFamilyIndexCount = 0;
        info.pQueueFamilyIndices = NULL;
    }

    if (vkCreateSwapchainKHR(device, &info, NULL, &swapchain) != VK_SUCCESS) {
        return result_swapchain_create_failure;
    }

    return result_success;
}

static result_t init_swapchain_framebuffers(void) {
    vkGetSwapchainImagesKHR(device, swapchain, &num_swapchain_images, swapchain_images);

    for (size_t i = 0; i < num_swapchain_images; i++) {
        if (vkCreateImageView(device, &(VkImageViewCreateInfo) {
            DEFAULT_VK_IMAGE_VIEW,
            .image = swapchain_images[i],
            .format = surface_format.format,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT
        }, NULL, &swapchain_image_views[i]) != VK_SUCCESS) {
            return result_image_view_create_failure;
        }
    }

    for (size_t i = 0; i < num_swapchain_images; i++) {
        if (vkCreateFramebuffer(device, &(VkFramebufferCreateInfo) {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = frame_render_pass,
            .attachmentCount = 2,
            .pAttachments = (VkImageView[2]) { frame_image_view, swapchain_image_views[i] },
            .width = swap_image_extent.width,
            .height = swap_image_extent.height,
            .layers = 1
        }, NULL, &swapchain_framebuffers[i]) != VK_SUCCESS) {
            return result_framebuffer_create_failure;
        }
    }
    return result_success;
}

result_t init_swapchain_dependents(void) {
    if (vmaCreateImage(allocator, &(VkImageCreateInfo) {
        DEFAULT_VK_IMAGE,
        .extent.width = swap_image_extent.width,
        .extent.height = swap_image_extent.height,
        .format = surface_format.format,
        .samples = render_multisample_flags,
        .usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
    }, &device_allocation_create_info, &frame_image, &frame_image_allocation, NULL) != VK_SUCCESS) {
        return result_image_create_failure;
    }

    if (vkCreateImageView(device, &(VkImageViewCreateInfo) {
        DEFAULT_VK_IMAGE_VIEW,
        .image = frame_image,
        .format = surface_format.format,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT
    }, NULL, &frame_image_view) != VK_SUCCESS) {
        return result_image_view_create_failure;
    }

    return result_success;
}

void term_swapchain_dependents(void) {
    vkDestroyImageView(device, frame_image_view, NULL);
    vmaDestroyImage(allocator, frame_image, frame_image_allocation);
}

static void term_swapchain(void) {
    for (size_t i = 0; i < num_swapchain_images; i++) {
        vkDestroyImageView(device, swapchain_image_views[i], NULL);
        vkDestroyFramebuffer(device, swapchain_framebuffers[i], NULL);
    }
    
    vkDestroySwapchainKHR(device, swapchain, NULL);
}

void reinit_swapchain(void) {
    // Even the tutorials use bad hacks, im all in
    int width;
    int height;
    glfwGetFramebufferSize(window, &width, &height);

    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }
    //
    
    term_swapchain_dependents();
    term_swapchain();
    init_swapchain();
    init_swapchain_dependents();
    init_swapchain_framebuffers();
}

static void framebuffer_resize(GLFWwindow*, int, int) {
    framebuffer_resized = true;
}

static VkSampleCountFlagBits get_max_multisample_flags(const VkPhysicalDeviceProperties* properties) {
    VkSampleCountFlags flags = properties->limits.framebufferColorSampleCounts;

    // Way too overkill for this project
    // if (flags & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    // if (flags & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    // if (flags & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (flags & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (flags & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (flags & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}

static result_t init_vk_core(void) {
    result_t result;

    if ((result = check_layers()) != result_success) {
        return result;
    }

    uint32_t num_instance_extensions;
    const char** instance_extensions = glfwGetRequiredInstanceExtensions(&num_instance_extensions);
    
    if (vkCreateInstance(&(VkInstanceCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &(VkApplicationInfo) {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "Vulkan Voxel",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "No Engine",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = VK_API_VERSION_1_3
        },
        .enabledExtensionCount = num_instance_extensions,
        .ppEnabledExtensionNames = instance_extensions,
        .enabledLayerCount = NUM_ELEMS(layers),
        .ppEnabledLayerNames = layers
    }, NULL, &instance) != VK_SUCCESS) {
        return result_instance_create_failure;
    }

    if (glfwCreateWindowSurface(instance, window, NULL, &surface) != VK_SUCCESS) {
        return result_surface_create_failure;
    }

    uint32_t num_physical_devices;
    vkEnumeratePhysicalDevices(instance, &num_physical_devices, NULL);
    if (num_physical_devices == 0) {
        return result_physical_device_support_unavailable;
    }

    uint32_t num_surface_formats;
    uint32_t num_present_modes;

    {
        VkPhysicalDevice physical_devices[num_physical_devices];
        vkEnumeratePhysicalDevices(instance, &num_physical_devices, physical_devices);

        if ((result = get_physical_device(num_physical_devices, physical_devices, &physical_device, &num_surface_formats, &num_present_modes, &queue_family_indices)) != result_success) {
            return result;
        }
    }

    vkGetPhysicalDeviceProperties(physical_device, &physical_device_properties);
    printf("Loaded physical device \"%s\"\n", physical_device_properties.deviceName);

    render_multisample_flags = get_max_multisample_flags(&physical_device_properties);

    if (vkCreateDevice(physical_device, &(VkDeviceCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &(VkPhysicalDeviceFeatures2) {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .features = {
                .samplerAnisotropy = VK_TRUE
            }
        },
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = (VkDeviceQueueCreateInfo[1]) {
            {
                .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                .queueFamilyIndex = queue_family_indices.graphics,
                .queueCount = 1,
                .pQueuePriorities = (float[1]) { 1.0f }
            }
        },
        .pEnabledFeatures = NULL,

        .enabledExtensionCount = NUM_ELEMS(extensions),
        .ppEnabledExtensionNames = extensions,
        .enabledLayerCount = NUM_ELEMS(layers),
        .ppEnabledLayerNames = layers
    }, NULL, &device) != VK_SUCCESS) {
        return result_device_create_failure;
    }

    if (vmaCreateAllocator(&(VmaAllocatorCreateInfo) {
        .instance = instance,
        .physicalDevice = physical_device,
        .device = device,
        .pAllocationCallbacks = NULL,
        .pDeviceMemoryCallbacks = NULL,
        .vulkanApiVersion = VK_API_VERSION_1_0,
        .flags = 0 // Don't think any are needed
    }, &allocator) != VK_SUCCESS) {
        return result_allocator_create_failure;
    }

    vkGetDeviceQueue(device, queue_family_indices.graphics, 0, &graphics_queue);
    vkGetDeviceQueue(device, queue_family_indices.presentation, 0, &presentation_queue);

    {
        VkSurfaceFormatKHR surface_formats[num_surface_formats];
        vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &num_surface_formats, surface_formats);

        surface_format = get_surface_format(num_surface_formats, surface_formats);
    }

    {
        VkPresentModeKHR present_modes[num_present_modes];
        vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &num_present_modes, present_modes);

        present_mode = get_present_mode(num_present_modes, present_modes);
    }

    if ((result = init_swapchain()) != result_success) {
        return result;
    }
    
    if ((result = init_swapchain_dependents()) != result_success) {
        return result;
    }

    for (size_t i = 0; i < NUM_FRAMES_IN_FLIGHT; i++) {
        if (
            vkCreateSemaphore(device, &(VkSemaphoreCreateInfo) { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO }, NULL, &image_available_semaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &(VkSemaphoreCreateInfo) { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO }, NULL, &render_finished_semaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &(VkFenceCreateInfo) {
                .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                .flags = VK_FENCE_CREATE_SIGNALED_BIT
            }, NULL, &in_flight_fences[i]) != VK_SUCCESS
        ) {
            return result_synchronization_primitive_create_failure;
        }
    }

    if (vkCreateCommandPool(device, &(VkCommandPoolCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queue_family_indices.graphics
    }, NULL, &command_pool) != VK_SUCCESS) {
        return result_command_pool_create_failure;
    }

    if (vkAllocateCommandBuffers(device, &(VkCommandBufferAllocateInfo) {
        DEFAULT_VK_COMMAND_BUFFER,
        .commandPool = command_pool,
        .commandBufferCount = NUM_FRAMES_IN_FLIGHT
    }, frame_command_buffers) != VK_SUCCESS) {
        return result_command_buffers_allocate_failure;
    }

    if (vkCreateRenderPass(device, &(VkRenderPassCreateInfo) {
        DEFAULT_VK_RENDER_PASS,

        .attachmentCount = 2,
        .pAttachments = (VkAttachmentDescription[2]) {
            {
                DEFAULT_VK_ATTACHMENT,
                .format = surface_format.format,
                .samples = render_multisample_flags,
                .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            },
            {
                DEFAULT_VK_ATTACHMENT,
                .format = surface_format.format,
                .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
                .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
            }
        },

        .pSubpasses = &(VkSubpassDescription) {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,

            .colorAttachmentCount = 1,
            .pColorAttachments = &(VkAttachmentReference) {
                .attachment = 0,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            },

            .pResolveAttachments = &(VkAttachmentReference) {
                .attachment = 1,
                .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
            }
        },

        .dependencyCount = 1,
        .pDependencies = &(VkSubpassDependency) {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
        }
    }, NULL, &frame_render_pass) != VK_SUCCESS) {
        return result_render_pass_create_failure;
    }

    vkGetSwapchainImagesKHR(device, swapchain, &num_swapchain_images, NULL);
    swapchain_images = malloc(num_swapchain_images*sizeof(VkImage));
    swapchain_image_views = malloc(num_swapchain_images*sizeof(VkImageView));
    swapchain_framebuffers = malloc(num_swapchain_images*sizeof(VkFramebuffer));

    if ((result = init_swapchain_framebuffers()) != result_success) {
        return result;
    }

    if (vkAllocateCommandBuffers(device, &(VkCommandBufferAllocateInfo) {
        DEFAULT_VK_COMMAND_BUFFER,
        .commandPool = command_pool
    }, &generic_command_buffer) != VK_SUCCESS) {
        return result_command_buffers_allocate_failure;
    }

    if (vkCreateFence(device, &(VkFenceCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
    }, NULL, &generic_command_fence) != VK_SUCCESS) {
        return result_synchronization_primitive_create_failure;
    }

    if (vkCreateDescriptorPool(device, &(VkDescriptorPoolCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = 2,
        .pPoolSizes = (VkDescriptorPoolSize[2]) {
            {
                .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .descriptorCount = 1
            },
            {
                .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 12
            }
        },
        .maxSets = 1
    }, NULL, &generic_descriptor_pool) != VK_SUCCESS) {
        return result_descriptor_pool_create_failure;
    }

    if (vkCreateQueryPool(device, &(VkQueryPoolCreateInfo) {
        .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
        .queryType = VK_QUERY_TYPE_TIMESTAMP,
        .queryCount = 2 * NUM_FRAMES_IN_FLIGHT
    }, NULL, &timestamp_query_pool) != VK_SUCCESS) {
        return result_query_pool_create_failure;
    }

    if (vkBeginCommandBuffer(generic_command_buffer, &(VkCommandBufferBeginInfo) {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    }) != VK_SUCCESS) {
        return result_command_buffer_begin_failure;
    }

    for (size_t i = 0; i < NUM_FRAMES_IN_FLIGHT; i++) {
        vkCmdWriteTimestamp(generic_command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, timestamp_query_pool, 2 * (uint32_t) i);
        vkCmdWriteTimestamp(generic_command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, timestamp_query_pool, 2 * (uint32_t) i + 1);
    }
    
    if (vkEndCommandBuffer(generic_command_buffer) != VK_SUCCESS) {
        return result_command_buffer_end_failure;
    }

    if ((result = submit_and_wait(graphics_queue, generic_command_buffer, generic_command_fence)) != result_success) {
        return result;
    }
    if ((result = reset_command_processing(generic_command_buffer, generic_command_fence)) != result_success) {
        return result;
    }

    if ((result = init_mandelbrot_compute_pipeline(generic_descriptor_pool)) != result_success) {
        return result;
    }

    if ((result = init_mandelbrot_management(graphics_queue, generic_command_buffer, generic_command_fence, queue_family_indices.graphics)) != result_success) {
        return result;
    }

    if ((result = init_mandelbrot_render_pipeline(graphics_queue, generic_command_buffer, generic_command_fence, generic_descriptor_pool, &physical_device_properties)) != result_success) {
        return result;
    }

    return result_success;
}

static void term_vk_core(void) {
    vkDeviceWaitIdle(device);
    term_mandelbrot_render_pipeline();
    term_mandelbrot_management();
    term_mandelbrot_compute_pipeline();

    vkDestroyQueryPool(device, timestamp_query_pool, NULL);
    vkDestroyDescriptorPool(device, generic_descriptor_pool, NULL);
    
    vkDestroyRenderPass(device, frame_render_pass, NULL);
    vkDestroyCommandPool(device, command_pool, NULL);

    vkDestroyFence(device, generic_command_fence, NULL);

    term_swapchain_dependents();
    
    term_swapchain();

    for (size_t i = 0; i < NUM_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, image_available_semaphores[i], NULL);
        vkDestroySemaphore(device, render_finished_semaphores[i], NULL);
        vkDestroyFence(device, in_flight_fences[i], NULL);
    }

    vmaDestroyAllocator(allocator);

    vkDestroyDevice(device, NULL);
    vkDestroySurfaceKHR(instance, surface, NULL);
    vkDestroyInstance(instance, NULL);

    free(swapchain_images);
    free(swapchain_image_views);
    free(swapchain_framebuffers);
}

static result_t init_glfw_core(void) {
    if (!glfwInit()) {
        return result_glfw_init_failure;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    if ((window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan", NULL, NULL)) == NULL) {
        return result_window_create_failure;
    }
    glfwSetFramebufferSizeCallback(window, framebuffer_resize);

    return result_success;
}

static void term_glfw_core(void) {
    glfwDestroyWindow(window);
    glfwTerminate();
}

result_t draw_gfx(microseconds_t* out_frame_render_time, microseconds_t* out_mandelbrot_frame_compute_time) {
    result_t result;

    if ((result = manage_mandelbrot_frames(&physical_device_properties, out_mandelbrot_frame_compute_time)) != result_success) {
        return result;
    }

    VkSemaphore image_available_semaphore = image_available_semaphores[frame_index];
    VkSemaphore render_finished_semaphore = render_finished_semaphores[frame_index];
    VkFence in_flight_fence = in_flight_fences[frame_index];
    vkWaitForFences(device, 1, &in_flight_fence, VK_TRUE, UINT64_MAX);

    uint64_t timestamps[2];
    vkGetQueryPoolResults(device, timestamp_query_pool, 2 * (uint32_t) frame_index, 2, sizeof(timestamps), timestamps, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);

    *out_frame_render_time = get_query_microseconds(timestamps[0], timestamps[1], physical_device_properties.limits.timestampPeriod);

    uint32_t image_index;
    {
        VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, image_available_semaphore, VK_NULL_HANDLE, &image_index);
        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            reinit_swapchain();
            return result_success;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            return result_swapchain_image_acquire_failure;
        }
    }

    vkResetFences(device, 1, &in_flight_fence);

    VkCommandBuffer command_buffer = frame_command_buffers[frame_index];

    vkResetCommandBuffer(command_buffer, 0);

    if (vkBeginCommandBuffer(command_buffer, &(VkCommandBufferBeginInfo) {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO
    }) != VK_SUCCESS) {
        return result_command_buffer_begin_failure;
    }

    vkCmdResetQueryPool(command_buffer, timestamp_query_pool, 2 * (uint32_t) frame_index, 2);
    vkCmdWriteTimestamp(command_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, timestamp_query_pool, 2 * (uint32_t) frame_index);

    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)swap_image_extent.width,
        .height = (float)swap_image_extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = { 0, 0 },
        .extent = swap_image_extent
    };
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    vkCmdBeginRenderPass(command_buffer, &(VkRenderPassBeginInfo) {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = frame_render_pass,
        .framebuffer = swapchain_framebuffers[image_index],
        .renderArea.offset = { 0, 0 },
        .renderArea.extent = swap_image_extent,
        .clearValueCount = 1,
        .pClearValues = (VkClearValue[1]) {
            { .color = { .float32 = { 0.62f, 0.78f, 1.0f, 1.0f } } }
        }
    }, VK_SUBPASS_CONTENTS_INLINE);

    mat3s tween_affine_map = glms_mat3_mul(glms_mat3_inv(mandelbrot_compute_affine_map), get_affine_map());
    if ((result = draw_mandelbrot_render_pipeline(command_buffer, get_mandelbrot_front_frame_index(), frame_index, &tween_affine_map)) != result_success) {
        return result;
    }

    vkCmdEndRenderPass(command_buffer);

    vkCmdWriteTimestamp(command_buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, timestamp_query_pool, 2 * (uint32_t) frame_index + 1);

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
        return result_command_buffer_end_failure;
    }

    VkPipelineStageFlags wait_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    
    if (vkQueueSubmit(graphics_queue, 1, &(VkSubmitInfo) {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &image_available_semaphore,
        .pWaitDstStageMask = &wait_stage_flags,
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &render_finished_semaphore
    }, in_flight_fence) != VK_SUCCESS) {
        return result_queue_submit_failure;
    }

    {
        VkResult result = vkQueuePresentKHR(presentation_queue, &(VkPresentInfoKHR) {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &render_finished_semaphore,
            .swapchainCount = 1,
            .pSwapchains = &swapchain,
            .pImageIndices = &image_index
        });
        
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized) {
            framebuffer_resized = false;
            vkWaitForFences(device, 1, &in_flight_fence, VK_TRUE, UINT64_MAX);
            reinit_swapchain();
        } else if (result != VK_SUCCESS) {
            return result_swapchain_image_present_failure;
        }
    }

    frame_index += 1;
    frame_index %= NUM_FRAMES_IN_FLIGHT;

    return result_success;
}

result_t init_gfx() {
    result_t result;

    if ((result = init_glfw_core()) != result_success) {
        return result;
    }

    if ((result = init_vk_core()) != result_success) {
        return result;
    }

    return result_success;
}

bool should_window_close() {
    return glfwWindowShouldClose(window);
}

void term_gfx() {
    term_vk_core();
    term_glfw_core();
}