#version 330 core
out vec4 o_color;
in vec2 v_texCoord;

uniform vec4 u_color;
uniform sampler2D u_atlas;

in VS_OUT {
    vec2 texCoord;
    vec2 glyphOffset;
    vec2 glyphSize;
} fs_in;

void main() {
    o_color = u_color * texture(u_atlas, fs_in.glyphOffset + fs_in.texCoord * fs_in.glyphSize).r;
}