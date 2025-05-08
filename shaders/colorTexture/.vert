#version 330 core
layout(location = 0) in vec4 a_position;
layout(location = 1) in vec4 a_normal;
layout(location = 2) in vec2 a_texCoord;
layout(location = 3) in vec4 a_tangent;
layout(location = 4) in ivec4 a_boneIDs;
layout(location = 5) in vec4 a_weights;

out vec2 v_texCoord;

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;

uniform mat4 u_modelMat;
uniform mat4 u_viewMat;
uniform mat4 u_projectionMat;
uniform mat4 u_boneMatrices[MAX_BONES];
uniform bool u_animated;

void main() {
    vec4 position = vec4(0);
    if(u_animated) {
        for(int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
            if(a_boneIDs[i] == -1) continue;
            if(a_boneIDs[i] >= MAX_BONES) {
                position = a_position;
                break;
            }
            vec4 localPosition = u_boneMatrices[a_boneIDs[i]] * a_position;
            position += localPosition * a_weights[i];
        }
    } else {
        position = a_position;
    }

    gl_Position = u_projectionMat * u_viewMat * u_modelMat * position;
    v_texCoord = a_texCoord;
}
