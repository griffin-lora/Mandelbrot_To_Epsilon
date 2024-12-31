#version 460

layout(push_constant, std430) uniform push_constants_t {
    float aspect;
};

layout(location = 0) in vec2 vertex_position;

layout(location = 0) out vec2 fragment_position;

void main() {
    gl_Position = vec4(vertex_position, 0.0, 1.0);
        
    mat2 aspect_map = mat2(
        aspect, 0.0,
        0.0, 1.0
    );
    fragment_position = aspect_map*0.5*(vertex_position + vec2(1.0, 1.0));
}