#include "camera.h"
#include "chrono.h"
#include "gfx/gfx.h"
#include "result.h"
#include <GLFW/glfw3.h>

int main() {
    result_t result;
    if ((result = init_gfx()) != result_success) {
        print_result_error(result);
        return 1;
    }
    
    init_camera();

    microseconds_t program_start = get_current_microseconds();

    float delta = 1.0f/60.0f;
    while (!glfwWindowShouldClose(window)) {
        microseconds_t start = get_current_microseconds() - program_start;
        glfwPollEvents();
        update_camera(delta);

        if ((result = draw_gfx()) != result_success) {
            print_result_error(result);
            return 1;
        }

        microseconds_t end = get_current_microseconds() - program_start;
        microseconds_t delta_microseconds = end - start;
        delta = (float)delta_microseconds/1000000.0f;

        microseconds_t remaining_microseconds = (1000000l/glfwGetVideoMode(glfwGetPrimaryMonitor())->refreshRate) - delta_microseconds;
        if (remaining_microseconds > 0) {
            sleep_microseconds(remaining_microseconds);
        }

        end = get_current_microseconds() - program_start;
        microseconds_t new_delta_microseconds = end - start;
        delta = (float) new_delta_microseconds/1000000.0f;
    }

    (void) delta;

    term_gfx();

    return 0;
}