#include "camera.h"
#include "gfx/gfx.h"
#include <GLFW/glfw3.h>
#include <cglm/struct/vec2.h>
#include <cglm/struct/mat3.h>
#include <cglm/struct/affine2d.h>
#include <cglm/util.h>
#include <stdio.h>

static bool in_movement_mode;
static vec2s movement_mode_last_cursor_position = {{ 0.0f, 0.0f }};

static float target_scale_factor = 1.0f;
static vec2s target_offset = {{ 0.0f, 0.0f }};

static float current_scale_factor = 1.0f;
static vec2s current_offset = {{ 0.0f, 0.0f }};

static void scroll(GLFWwindow*, double, double factor) {
    float scale_factor = 0.5f + (0.5f*(1.0f - ((float) factor)));
    target_scale_factor *= scale_factor;
}

static bool is_cursor_position_out_of_bounds(vec2s cursor_position) {
    int width;
    int height;
    glfwGetFramebufferSize(window, &width, &height);
    return cursor_position.x < 0.0f || cursor_position.x >= (float) width || cursor_position.y < 0.0f || cursor_position.y >= (float) height;
}

static vec2s get_offset(void) {
    int mouse_button = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1);
    
    if (in_movement_mode && mouse_button != GLFW_PRESS) {
        in_movement_mode = false;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        return (vec2s) {{ 0.0f, 0.0f }};
    }
    if (!in_movement_mode && mouse_button == GLFW_PRESS) {
        double cursor_x;
        double cursor_y;
        glfwGetCursorPos(window, &cursor_x, &cursor_y);

        const vec2s cursor_position = {{ (float)cursor_x, (float)cursor_y }};
        if (is_cursor_position_out_of_bounds(cursor_position)) {
            return (vec2s) {{ 0.0f, 0.0f }};
        }

        in_movement_mode = true;
        movement_mode_last_cursor_position = cursor_position;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        return (vec2s) {{ 0.0f, 0.0f }};
    }

    if (!in_movement_mode) {
        return (vec2s) {{ 0.0f, 0.0f }};
    }


    double cursor_x;
    double cursor_y;
    glfwGetCursorPos(window, &cursor_x, &cursor_y);

    const vec2s cursor_position = {{ (float)cursor_x, (float)cursor_y }};

    vec2s offset = glms_vec2_scale(glms_vec2_sub(movement_mode_last_cursor_position, cursor_position), (1.0f / 500.0f));
    movement_mode_last_cursor_position = cursor_position;
    return offset;
}

void init_camera(void) {
    glfwSetScrollCallback(window, scroll);
}

void update_camera(float delta) {
    target_offset = glms_vec2_add(target_offset, glms_vec2_scale(get_offset(), current_scale_factor));

    float lerp_time = 24.0f * delta;
    if (lerp_time > 1.0f) { lerp_time = 1.0f; }
    current_scale_factor = glm_lerp(current_scale_factor, target_scale_factor, lerp_time);
    current_offset = glms_vec2_lerp(current_offset, target_offset, lerp_time);
}

static mat3s get_preaspect_affine_map(float scale_factor, vec2s offset) {
    mat3s scale_affine_map = glms_scale2d_make((vec2s) {{ scale_factor, scale_factor }});
    mat3s translate_affine_map = glms_translate2d_make(offset);
    return glms_mat3_mul(translate_affine_map, scale_affine_map);
}

mat3s get_affine_map() {
    mat3s current_affine_map = get_preaspect_affine_map(current_scale_factor, current_offset);

    int width;
    int height;
    glfwGetFramebufferSize(window, &width, &height);
    mat3s aspect_affine_map = glms_scale2d_make((vec2s) {{ (float) width / (float) height, 1.0f }});

    mat3s affine_map = glms_mat3_mul(current_affine_map, aspect_affine_map);

    return affine_map;
}