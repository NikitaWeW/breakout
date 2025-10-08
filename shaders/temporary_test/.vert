#version 330 core
layout(location = 0) in vec4 a_position;
layout(location = 1) in vec2 a_texCoord;
layout(location = 2) in vec4 a_normal;
layout(location = 3) in vec4 a_tangent;
layout(location = 4) in vec4 a_boneIDs;
layout(location = 5) in vec4 a_weights;

out VS_OUT {
    vec2 texCoords;
    vec3 fragPos;
    flat mat3 TBN;
} vs_out;

const int MAX_BONES = 100;

uniform mat4 u_modelMat;
uniform mat4 u_normalMat;
uniform mat4 u_viewMat;
uniform mat4 u_projMat;

uniform mat4 u_boneMatrices[MAX_BONES];
uniform bool u_animated;

void main() {
    vec4 position = vec4(0);
    if(u_animated) {
        for(int i = 0; i < 4; ++i) {
            if(a_boneIDs[i] == -1) continue;
            if(a_boneIDs[i] >= MAX_BONES) {
                position = a_position;
                break;
            }
            vec4 localPosition = u_boneMatrices[int(a_boneIDs[i])] * a_position;
            position += localPosition * a_weights[i];
        }
    } else {
        position = a_position;
    }

    vs_out.fragPos = vec3(u_modelMat * position);
    gl_Position = u_projMat * u_viewMat * vec4(vs_out.fragPos, 1);
    vs_out.texCoords = a_texCoord;
    
    vec3 normal = normalize(vec3(u_normalMat * a_normal));
    vec3 tangent = normalize(vec3(u_normalMat * vec4(a_tangent.xyz, 0.0)));
    tangent = normalize(tangent - dot(tangent, normal) * normal);
    vec3 bitangent = cross(tangent, normal);
    vs_out.TBN = mat3(tangent, bitangent, normal);
}
