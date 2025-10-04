#version 430 core
out vec4 o_color;

in VS_OUT {
    vec2 texCoords;
    vec3 fragPos;
    flat mat3 TBN;
} fs_in;

struct Material
{
    sampler2D albedo;
    sampler2D normal;
    sampler2D metallic;
    sampler2D roughness;
};

uniform Material u_material;

uniform vec4 u_color = vec4(1);

void main() 
{
    o_color = u_color * texture(u_material.albedo, fs_in.texCoords);
}
