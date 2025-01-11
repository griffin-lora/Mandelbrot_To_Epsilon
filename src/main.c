#include "camera.h"
#include "chrono.h"
#include "gfx/gfx.h"
#include "gfx/mandelbrot_management.h"
#include "result.h"
#include <GLFW/glfw3.h>
#include <stdio.h>

int main() {
    result_t result;
    if ((result = init_gfx()) != result_success) {
        print_result_error(result);
        return 1;
    }
    
    init_camera();

    microseconds_t frame_render_time = 0;
    microseconds_t mandelbrot_frame_compute_time = 0;

    float delta = 1.0f/60.0f;
    while (!glfwWindowShouldClose(window)) {
        microseconds_t logic_start = get_current_microseconds();
        glfwPollEvents();
        update_camera(delta);

        if ((result = draw_gfx(&frame_render_time, &mandelbrot_frame_compute_time)) != result_success) {
            print_result_error(result);
            return 1;
        }

        microseconds_t logic_end = get_current_microseconds();
        microseconds_t logic_time = logic_end - logic_start;

        microseconds_t remaining_logic_time = (1000000l/glfwGetVideoMode(glfwGetPrimaryMonitor())->refreshRate) - logic_time;
        if (remaining_logic_time > 0) {
            sleep_microseconds(remaining_logic_time);
        }

        logic_end = get_current_microseconds();
        microseconds_t total_logic_time = logic_end - logic_start;
        delta = (float) total_logic_time/1000000.0f;

        // printf("FPS: %f, Logic time: %ldμs, Frame render time: %ldμs, Mandelbrot frame compute time: %ldμs\n", delta, logic_time, frame_render_time, mandelbrot_frame_compute_time);
    }

    term_gfx();

    return 0;
}