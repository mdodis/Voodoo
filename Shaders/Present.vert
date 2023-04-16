#version 460

layout (location = 0) out vec2 uv;

vec2 quad_positions[6] = {
    vec2(-1.0,  1.0), // top left
    vec2(-1.0, -1.0), // bottom left
    vec2( 1.0, -1.0), // bottom right
    vec2( 1.0,  1.0), // top right
    vec2(-1.0,  1.0), // top left
    vec2( 1.0, -1.0), // bottom right
};

vec2 quad_uvs[6] = {
    vec2( 0.0,  1.0), // top left
    vec2( 0.0,  0.0), // bottom left
    vec2( 1.0,  0.0), // bottom right
    vec2( 1.0,  1.0), // top right
    vec2( 0.0,  1.0), // top left
    vec2( 1.0,  0.0), // bottom right
};


void main() {
    vec2 v = quad_positions[gl_VertexIndex];
    uv = quad_uvs[gl_VertexIndex];
    gl_Position = vec4(v.x, v.y, 0.0, 1.0);
}