#include "camera.h"
#include "gfx/gfx.h"
#include <GLFW/glfw3.h>
#include <cglm/struct/vec2.h>
#include <cglm/struct/mat3.h>
#include <cglm/struct/affine2d.h>
#include <cglm/util.h>

static bool in_movement_mode;
static vec2s movement_mode_cursor_position = {{ 0.0f, 0.0f }};

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
        movement_mode_cursor_position = cursor_position;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        return (vec2s) {{ 0.0f, 0.0f }};
    }

    if (!in_movement_mode) {
        return (vec2s) {{ 0.0f, 0.0f }};
    }


    double cursor_x;
    double cursor_y;
    glfwGetCursorPos(window, &cursor_x, &cursor_y);

    const vec2s cursor_position = {{ (float)cursor_x, (float)cursor_y }};
    if (is_cursor_position_out_of_bounds(cursor_position)) {
        return (vec2s) {{ 0.0f, 0.0f }};
    }

    glfwSetCursorPos(window, movement_mode_cursor_position.x, movement_mode_cursor_position.y);

    return glms_vec2_scale(glms_vec2_sub(movement_mode_cursor_position, cursor_position), (1.0f / 5000.0f));
}

void init_camera(void) {
    glfwSetScrollCallback(window, scroll);
}

void update_camera(void) {
    target_offset = glms_vec2_add(target_offset, glms_vec2_scale(get_offset(), current_scale_factor));
}

mat3s get_affine_map(float delta) {
    (void) delta;

    current_scale_factor = glm_lerp(current_scale_factor, target_scale_factor, 0.2f);
    current_offset = glms_vec2_lerp(current_offset, target_offset, 0.2f);

    mat3s scale_affine_map = glms_scale2d_make((vec2s) {{ current_scale_factor, current_scale_factor }});
    mat3s translate_affine_map = glms_translate2d_make(current_offset);

    mat3s preaspect_affine_map = glms_mat3_mul(translate_affine_map, scale_affine_map);

    int width;
    int height;
    glfwGetFramebufferSize(window, &width, &height);
    mat3s aspect_affine_map = glms_scale2d_make((vec2s) {{ (float) width / (float) height, 1.0f }});

    mat3s affine_map = glms_mat3_mul(preaspect_affine_map, aspect_affine_map);

    return affine_map;
}