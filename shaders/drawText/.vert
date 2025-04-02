#version 330 core
layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_texCoord;
out vec2 v_texCoord;

uniform vec2 u_position;
uniform vec2 u_size;
uniform mat4 u_projMat;
void main() {
    v_texCoord = a_texCoord;
    vec4 position = u_projMat * vec4(u_position + a_position * u_size, 0, 1);
    gl_Position = vec4(position.xy, 0, position.w);
}
