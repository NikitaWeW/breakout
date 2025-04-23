#version 330 core
layout(location = 0) in vec4 a_position;
layout(location = 2) in vec2 a_texCoord;

out vec2 v_texCoord;

uniform mat4 u_modelMat;
uniform mat4 u_viewMat;
uniform mat4 u_projectionMat;

void main() {
    gl_Position = u_projectionMat * u_viewMat * u_modelMat * a_position;
    v_texCoord = a_texCoord;
}
