#version 430 core
out vec4 o_color;

in VS_OUT {
    vec2 texCoords;
    vec3 fragPos;
    flat mat3 TBN;
} fs_in;

uniform vec4 u_color = vec4(1);

void main() 
{
    o_color = u_color;
}
