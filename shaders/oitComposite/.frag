#version 430

layout(binding = 0) uniform sampler2D u_accum;
layout(binding = 1) uniform sampler2D u_revealage;

out vec4 o_color;

const float EPSILON = 1e-5;
float max3(vec3 v)
{
    return max(max(v.x, v.y), v.z);
}

void main() {
    ivec2 coords = ivec2(gl_FragCoord.xy);
    float revealage = texelFetch(u_revealage, coords, 0).r;
    if(abs(revealage - 1.0f) < EPSILON) discard;

    vec4 accumulated = texelFetch(u_accum, coords, 0);

    // suppress overflow
    if(isinf(max3(abs(accumulated.rgb)))) accumulated.rgb = vec3(accumulated.a);

    vec3 averageColor = accumulated.rgb / max(accumulated.a, EPSILON);
    
    o_color = vec4(averageColor, 1 - revealage);
}