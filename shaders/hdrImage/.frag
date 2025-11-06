#version 430

layout(binding = 0) uniform sampler2D u_texture;
uniform float u_exposure = 1;

in vec2 v_texCoord;
out vec4 o_color;

void main() {
    vec3 hdrColor = texture(u_texture, v_texCoord).rgb;
    vec3 mappedColor = 1 - exp(-hdrColor * u_exposure);
    
    o_color.rgb = mappedColor;
    o_color.rgb = pow(o_color.rgb, vec3(1/2.2)); // apply gamma correction
    o_color.a = 1;
    // o_color.rgb = hdrColor;
}