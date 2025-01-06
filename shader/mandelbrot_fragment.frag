#version 460

layout(set = 0, binding = 0) uniform sampler2D color_sampler;

layout(location = 0) in vec2 texel_coord;

layout(location = 0) out vec4 fragment_color;

void main() {
    fragment_color = texture(color_sampler, texel_coord);
}