#version 330 core
out vec4 o_color;

struct Material
{
    sampler2D diffuse;
};

in vec2 v_texCoord;
uniform Material u_material;
uniform vec4 u_color;

void main() {
    o_color = u_color * texture(u_material.diffuse, v_texCoord);
    if(o_color.a < 1e-5) discard;
    o_color.rgb = pow(o_color.rgb, vec3(1/2.2)); // apply gamma correction
}