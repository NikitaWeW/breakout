#version 330 core
out vec4 o_color;

uniform vec4 u_fgColor;
uniform vec4 u_bgColor;
uniform sampler2D u_atlas;
uniform float u_screenPxRange;

in VS_OUT {
    vec2 texCoord;
    vec2 glyphOffset;
    vec2 glyphSize;
} fs_in;

float median(float r, float g, float b) {
    return max(min(r, g), min(max(r, g), b));
}

void main() {
    vec2 glyphTexCoord = fs_in.glyphOffset + fs_in.texCoord * fs_in.glyphSize;
    vec3 msd = texture(u_atlas, glyphTexCoord).rgb;
    float sd = median(msd.r, msd.g, msd.b);
    float screenPxDistance = u_screenPxRange * (sd - 0.5);
    float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
    o_color = mix(u_bgColor, u_fgColor, opacity);
    gl_FragDepth = 0;
}
