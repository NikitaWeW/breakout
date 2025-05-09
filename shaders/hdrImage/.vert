#version 330 core

vec2 vertices[4] = vec2[4](
    vec2(-1,  1),
    vec2( 1,  1),
    vec2( 1, -1),
    vec2(-1, -1) 
);

void main() {
    gl_Position = vec4(vertices[gl_VertexID], 0, 1);
}
