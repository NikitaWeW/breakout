#version 330 core
out vec4 o_color;
in vec2 v_texCoord;

uniform vec3 u_color;
uniform sampler2D u_atlas;
uniform vec2 u_glyphOffset;
uniform vec2 u_glyphSize;

void main() {
    o_color = vec4(u_color, 1) * texture(u_atlas, u_glyphOffset + v_texCoord * u_glyphSize).r;
}