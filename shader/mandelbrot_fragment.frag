#version 460

layout(location = 0) in vec2 fragment_position;

layout(location = 0) out vec4 fragment_color;

vec2 square(vec2 z) {
    return vec2(z.x*z.x - z.y*z.y, 2.0*z.x*z.y);
}

float square_modulus(vec2 z) {
    return z.x*z.x + z.y*z.y;
}

const vec3 colors[16] = vec3[16](
    vec3(0.258824, 0.117647, 0.058824),
    vec3(0.098039, 0.027451, 0.101961),
    vec3(0.035294, 0.003922, 0.184314),
    vec3(0.015686, 0.015686, 0.286275),
    vec3(0.000000, 0.027451, 0.392157),
    vec3(0.047059, 0.172549, 0.541176),
    vec3(0.094118, 0.321569, 0.694118),
    vec3(0.223529, 0.490196, 0.819608),
    vec3(0.525490, 0.709804, 0.898039),
    vec3(0.827451, 0.925490, 0.972549),
    vec3(0.945098, 0.913725, 0.749020),
    vec3(0.972549, 0.788235, 0.372549),
    vec3(1.000000, 0.666667, 0.000000),
    vec3(0.800000, 0.501961, 0.000000),
    vec3(0.600000, 0.341176, 0.000000),
    vec3(0.415686, 0.203922, 0.011765)
);

void main() {
    vec2 c = fragment_position;
    vec2 z = vec2(0.0, 0.0);
    for (int i = 0; i < 250; i++) {
        z = square(z) + c;
        if (square_modulus(z) >= 4.0) {
            vec3 color = colors[i % 16];
            fragment_color = vec4(color, 1.0);
            return;
        }
    }
    
    fragment_color = vec4(0.0, 0.0, 0.0, 1.0);
}