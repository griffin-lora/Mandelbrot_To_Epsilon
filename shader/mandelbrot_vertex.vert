#version 460

layout(push_constant, std430) uniform push_constants_t {
    mat3 affine_map;
};

layout(location = 0) in vec2 vertex_position;

layout(location = 0) out vec2 fragment_position;

void main() {
    gl_Position = vec4(vertex_position, 0.0, 1.0);

    fragment_position = (affine_map * vec3(vertex_position, 1.0)).xy;
}