#version 330 core
out vec4 o_color;

uniform vec4 u_color;

void main() {
    o_color = u_color;
    // o_color.rgb = pow(o_color.rgb, vec3(1/2.2)); // apply gamma correction
}