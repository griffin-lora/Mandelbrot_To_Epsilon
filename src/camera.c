#include "camera.h"
#include "gfx/gfx.h"
#include <GLFW/glfw3.h>
#include <cglm/struct/vec2.h>
#include <cglm/struct/mat3.h>
#include <cglm/struct/affine2d.h>
#include <stdio.h>

static bool in_movement_mode;
static vec2s movement_mode_cursor_position = {{ 0.0f, 0.0f }};

static mat3s scale_affine_map;
static mat3s translate_affine_map;

static void scroll(GLFWwindow*, double, double factor) {
    float scale_factor = 0.5f + (0.5f*(1.0f - ((float) factor)));

    scale_affine_map = glms_mat3_mul(glms_scale2d_make((vec2s) {{ scale_factor, scale_factor }}), scale_affine_map);
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

    return glms_vec2_scale(glms_vec2_sub(movement_mode_cursor_position, cursor_position), (1.0f / 10000.0f));
}

void init_camera(void) {
    scale_affine_map = glms_mat3_identity();
    translate_affine_map = glms_mat3_identity();
    
    glfwSetScrollCallback(window, scroll);
}

void update_camera(float delta) {
    vec2s offset = get_offset();

    vec3s relative_offset = (vec3s) {{ offset.x, offset.y, 0.0f }};
    relative_offset = glms_mat3_mulv(scale_affine_map, relative_offset);

    translate_affine_map = glms_mat3_mul(translate_affine_map, glms_translate2d_make((vec2s) {{ relative_offset.x, relative_offset.y }}));

    (void) delta;
}

mat3s get_affine_map(void) {
    int width;
    int height;
    glfwGetFramebufferSize(window, &width, &height);
    mat3s aspect_affine_map = glms_scale2d_make((vec2s) {{ (float) width / (float) height, 1.0f }});

    mat3s affine_map = glms_mat3_mul(glms_mat3_mul(translate_affine_map, scale_affine_map), aspect_affine_map);

    return affine_map;
}