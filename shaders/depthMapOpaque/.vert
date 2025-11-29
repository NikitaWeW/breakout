#version 430 core
layout(location = 0) in vec4 a_position;
layout(location = 4) in vec4 a_boneIDs;
layout(location = 5) in vec4 a_weights;

uniform uvec2 u_atlasSize;

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;

uniform mat4 u_modelMat;

uniform mat4 u_boneMatrices[MAX_BONES];
uniform bool u_animated;

struct DrawLight
{
    mat4 viewMat;
    mat4 projMat;
    uvec2 atlasPos;
    uint atlasSize;
    float _pad0;
    vec4 _pad1;
    vec4 _pad2;
    vec4 _pad3;
};

layout(std430, binding = 0) buffer Lights
{
    DrawLight lights[];
};

void main() {
    vec4 position = vec4(0);
    if(u_animated) {
        for(int i = 0; i < 4; ++i) {
            if(a_boneIDs[i] == -1) continue;
            if(a_boneIDs[i] >= MAX_BONES) {
                position = a_position;
                break;
            }
            mat4 matrix = u_boneMatrices[int(a_boneIDs[i])];
            position += matrix * a_position * a_weights[i];
        }
    } else {
        position = a_position;
    }

    DrawLight light = lights[gl_InstanceID];
    vec4 clip = light.projMat * light.viewMat * u_modelMat * position;
    clip /= clip.w;
    clip.xyz = clamp(clip.xyz, vec3(-1), vec3(1));
    // Transform the clip coordinates into light viewport
    vec2 size = vec2(light.atlasSize) / vec2(u_atlasSize) ;
    clip.xy *= size;
    clip.xy += (2.0 * vec2(light.atlasPos) / vec2(u_atlasSize) + size - 1.0) * clip.w;
    gl_Position = clip;
}
