#version 430 core
#extension GL_ARB_shader_viewport_layer_array : enable
#extension GL_AMD_vertex_shader_layer : enable
#extension GL_NV_viewport_array2 : enable
#define HAS_VERTEX_LAYERED_RENDERING (GL_ARB_shader_viewport_layer_array || GL_AMD_vertex_shader_layer || GL_NV_viewport_array2)

layout(location = 0) in vec4 a_position;
layout(location = 4) in vec4 a_boneIDs;
layout(location = 5) in vec4 a_weights;

uniform uvec2 u_atlasSize;

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;

uniform mat4 u_modelMat;

uniform mat4 u_boneMatrices[MAX_BONES];
uniform bool u_animated;

uniform uint u_first;

struct DrawLight
{
    mat4 viewMat;
    mat4 projMat;
    int omnidirectional;
    float farPlane;
    vec2 _pad1;
    vec3 position;
    float _pad2;
    vec4 _pad3;
    vec4 _pad4;
};

layout(std430, binding = 0) buffer Lights
{
    DrawLight lights[];
};

out VS_OUT {
    flat int omnidirectional;
    flat vec3 lightPos;
    flat float zfar;
    vec3 fragPos;
} vs_out;

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

    gl_ViewportIndex = gl_InstanceID;
    DrawLight light = lights[gl_InstanceID + u_first];
    vs_out.omnidirectional = light.omnidirectional;
    vs_out.fragPos = (u_modelMat * position).xyz;
    vs_out.zfar = light.farPlane;
    vs_out.lightPos = light.position;
    gl_Position = light.projMat * light.viewMat * vec4(vs_out.fragPos, 1);
}
