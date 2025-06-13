#version 430

layout(binding = 0) uniform sampler2D u_accum;
layout(binding = 1) uniform sampler2D u_revelage;

out vec4 o_color;


const float EPSILON = 1e-5;
bool isApproximatelyEqual(float a, float b)
{
    return abs(a - b) <= (abs(a) < abs(b) ? abs(b) : abs(a)) * EPSILON;
}
float max3(vec3 v)
{
    return max(max(v.x, v.y), v.z);
}

void main() {
    ivec2 coords = ivec2(gl_FragCoord.xy);
    float revelage = texelFetch(u_revelage, coords, 0).r;
    if(isApproximatelyEqual(revelage, 1.0f)) discard;

    vec4 accumulated = texelFetch(u_accum, coords, 0);

    // suppress overflow
    if(isinf(max3(abs(accumulated.rgb)))) accumulated.rgb = vec3(accumulated.a);

    vec3 averageColor = accumulated.rgb / max(accumulated.a, EPSILON);
    
    o_color = vec4(averageColor, 1 - revelage);
    // o_color = vec4(1);
}