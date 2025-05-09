#version 330 core
out vec4 o_color;

in vec2 v_texCoord;
uniform sampler2D u_texture;
uniform vec4 u_color;

void main() {
    o_color = u_color * texture(u_texture, v_texCoord);
    if(o_color.a < 1e-5) discard;
    o_color.rgb = pow(o_color.rgb, vec3(1/2.2)); // apply gamma correction
}