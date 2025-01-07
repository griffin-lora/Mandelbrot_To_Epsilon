#version 460

layout(push_constant, std430) uniform push_constants_t {
    mat3 affine_map;
};

layout(location = 0) in vec2 vertex_position;

layout(location = 0) out vec2 texel_coord;

void main() {
    gl_Position = vec4(vertex_position, 0.0, 1.0);

    texel_coord = 0.5*((affine_map * vec3(vertex_position, 1.0)).xy + vec2(1.0, 1.0));
}