#version 330 core
layout(location = 0) out vec4 o_accum;
layout(location = 1) out float o_reveal;

in vec2 v_texCoord;
uniform sampler2D u_texture;
uniform vec4 u_color;

void main() {
    vec4 color = u_color * texture(u_texture, v_texCoord);
    if(o_accum.a < 1e-5) discard;
    float weight = clamp(pow(min(1.0, color.a * 10.0) + 0.01, 3.0) * 1e8 * pow(1.0 - gl_FragCoord.z * 0.9, 3.0), 1e-2, 3e3);

    o_accum = vec4(color.rgb * color.a, color.a) * weight;
    o_reveal = color.a;
}