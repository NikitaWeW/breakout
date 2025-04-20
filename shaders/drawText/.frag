#version 330 core
out vec4 o_color;
in vec2 v_texCoord;

uniform vec4 u_color;
uniform sampler2DMS u_atlas;

in VS_OUT {
    vec2 texCoord;
    vec2 glyphOffset;
    vec2 glyphSize;
} fs_in;

vec4 textureMultisample(sampler2DMS sampler, vec2 texCoord, int numSamples)
{
    ivec2 size = textureSize(sampler);
    vec4 color = vec4(0.0);

    for (int i = 0; i < numSamples; i++)
        color += texelFetch(sampler, ivec2(size * texCoord), i);

    color /= float(numSamples);

    return color;
}

void main() {
    o_color = u_color * textureMultisample(u_atlas, fs_in.glyphOffset + fs_in.texCoord * fs_in.glyphSize, 4).r;
    if(o_color.a < 1e-5) discard;
}