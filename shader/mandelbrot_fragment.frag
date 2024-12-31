#version 460

layout(location = 0) in vec2 fragment_position;

layout(location = 0) out vec4 fragment_color;

void main() {
    fragment_color = vec4(fragment_position.x, fragment_position.y, 0.0, 1.0);
}