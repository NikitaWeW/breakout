#version 330 core
layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_texCoord;

layout(location = 2) in vec2 a_quadPosition;
layout(location = 3) in vec2 a_quadSize;
layout(location = 4) in vec2 a_glyphOffset;
layout(location = 5) in vec2 a_glyphSize;
out VS_OUT {
    vec2 texCoord;
    vec2 glyphOffset;
    vec2 glyphSize;
} vs_out;

uniform mat4 u_projMat;
void main() {
    vs_out.texCoord = a_texCoord;
    vs_out.glyphOffset = a_glyphOffset;
    vs_out.glyphSize = a_glyphSize;
    vec4 position = u_projMat * vec4(a_quadPosition + a_position * a_quadSize, 0, 1);
    gl_Position = vec4(position.xy, 0, position.w);
}
