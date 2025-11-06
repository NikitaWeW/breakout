#version 430 core

const vec3 cubePositions[14] = {
    vec3(-1, 1, 1), // Front-top-left
    vec3( 1, 1, 1), // Front-top-right
    vec3(-1,-1, 1), // Front-bottom-left
    vec3( 1,-1, 1), // Front-bottom-right
    vec3( 1,-1,-1), // Back-bottom-right
    vec3( 1, 1, 1), // Front-top-right
    vec3( 1, 1,-1), // Back-top-right
    vec3(-1, 1, 1), // Front-top-left
    vec3(-1, 1,-1), // Back-top-left
    vec3(-1,-1, 1), // Front-bottom-left
    vec3(-1,-1,-1), // Back-bottom-left
    vec3( 1,-1,-1), // Back-bottom-right
    vec3(-1, 1,-1), // Back-top-left
    vec3( 1, 1,-1)  // Back-top-right
};

out vec3 v_texCoords;

uniform mat4 u_projMat;
uniform mat4 u_viewMat;

void main() {
    v_texCoords = cubePositions[gl_VertexID];
    gl_Position = u_projMat * mat4(mat3(u_viewMat)) * vec4(v_texCoords,  1);
}
