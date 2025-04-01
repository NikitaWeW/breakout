#version 330 core
out vec4 o_color;

uniform sampler2D u_texture;
in vec2 v_texCoord;

void main() {
    const float offset = 0.001;
    float kernel[9] = float[]( // blur kernel
        1.0 / 16, 2.0 / 16, 1.0 / 16,
        2.0 / 16, 4.0 / 16, 2.0 / 16,
        1.0 / 16, 2.0 / 16, 1.0 / 16
    );
    vec2 offsets[9] = vec2[](
        vec2(-offset, offset), // top-left
        vec2( 0.0f, offset), // top-center
        vec2( offset, offset), // top-right
        vec2(-offset, 0.0f), // center-left
        vec2( 0.0f, 0.0f), // center-center
        vec2( offset, 0.0f), // center-right
        vec2(-offset, -offset), // bottom-left
        vec2( 0.0f, -offset), // bottom-center
        vec2( offset, -offset) // bottom-right
    );
    vec3 samples[9];
    for(int i = 0; i < 9; i++)
        samples[i] = vec3(texture(u_texture, v_texCoord.st + offsets[i]));
    vec3 color = vec3(0.0);
    for(int i = 0; i < 9; i++)
        color += samples[i] * kernel[i];
    o_color = vec4(color, 1.0);
}