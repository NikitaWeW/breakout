#version 430

layout(binding = 0) uniform sampler2D u_texture;
uniform float u_exposure = 1;

out vec4 o_color;

void main() {
    vec2 texCoord = gl_FragCoord.xy / textureSize(u_texture, 0);
    vec3 hdrColor = texture(u_texture, texCoord).rgb;
    vec3 mappedColor = 1 - exp(-hdrColor * u_exposure);
    
    o_color = vec4(mappedColor, 1);
    o_color.rgb = pow(o_color.rgb, vec3(1/2.2)); // apply gamma correction
}