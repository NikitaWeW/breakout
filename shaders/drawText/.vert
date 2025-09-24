#version 330 core
layout(location = 0) in vec2 a_quadPosition;
layout(location = 1) in vec2 a_quadSize;
layout(location = 2) in vec2 a_glyphOffset;
layout(location = 3) in vec2 a_glyphSize;
out VS_OUT {
    vec2 texCoord;
    vec2 glyphOffset;
    vec2 glyphSize;
} vs_out;

vec2 vertices[4] = vec2[4](
    vec2(0, 0), 
    vec2(1, 0),
    vec2(1, 1),
    vec2(0, 1)
);

uniform mat4 u_projMat;
uniform float u_depth = 0;
void main() {
    vs_out.texCoord = vertices[gl_VertexID];
    vs_out.glyphOffset = a_glyphOffset;
    vs_out.glyphSize = a_glyphSize;
    vec4 position = u_projMat * vec4(a_quadPosition + vs_out.texCoord * a_quadSize, 0, 1);
    gl_Position = vec4(position.xy, u_depth, position.w);
}
