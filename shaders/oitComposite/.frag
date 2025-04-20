#version 330 core
out vec4 o_color;

uniform sampler2D u_accum;
uniform sampler2D u_reveal;

const float EPSILON = 1e-5;

bool aproximatlyEqual(float a, float b) {
    return abs(a - b) <= (abs(a) < abs(b) ? abs(b) : abs(a)) * EPSILON;
}
float max3(vec3 v)
{
    return max(max(v.x, v.y), v.z);
}

void main() {
    ivec2 coords = ivec2(gl_FragCoord.xy);
    float revelage = texelFetch(u_reveal, coords, 0).r;
    if(aproximatlyEqual(revelage, 1)) discard;
    vec4 accumulation = texelFetch(u_accum, coords, 0);
    if(isinf(max3(abs(accumulation.rgb)))) {
        accumulation.rgb = vec3(accumulation.a);
    }
    vec3 averageColor = accumulation.rgb / max(accumulation.a, EPSILON);
    o_color = vec4(averageColor, 1 - revelage);
}