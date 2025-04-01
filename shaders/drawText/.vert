#version 330 core
layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_texCoord;
out vec2 v_texCoord;

uniform vec2 u_position;
uniform vec2 u_size;
void main() {
    v_texCoord = a_texCoord;
    gl_Position = vec4(u_position + a_position * u_size, 0, 1);
}
