#version 330 core

uniform mat4 u_projectionMat;
uniform mat4 u_viewMat;
uniform vec3 u_cameraPosition;

out VS_OUT {
    vec3 fragmentPosition;
    vec3 cameraPosition;
    vec2 texCoord;
} vs_out;

const vec3 vertices[4] = vec3[4](
    vec3(-1.0f, 0.0f,  1.0f),
    vec3( 1.0f, 0.0f,  1.0f),
    vec3( 1.0f, 0.0f, -1.0f),
    vec3(-1.0f, 0.0f, -1.0f) 
);
const vec2 texCoords[4] = vec2[4](
    vec2(0.0f, 1.0f),
    vec2(1.0f, 1.0f),
    vec2(1.0f, 0.0f),
    vec2(0.0f, 0.0f)
);

const float gridSize = 50;
const float gridHeight = -10; // ignore oit and call it a feature. TODO: second OIT try
const float gridSpacing = 10;

void main() {
    vec3 vertexPosition = vertices[gl_VertexID] * gridSize;
    vertexPosition += floor(u_cameraPosition / gridSpacing) * gridSpacing;
    vertexPosition.y = gridHeight;
    vs_out.fragmentPosition = vertexPosition;
    vs_out.cameraPosition = u_cameraPosition;
    vs_out.texCoord = texCoords[gl_VertexID];
    gl_Position = u_projectionMat * u_viewMat * vec4(vs_out.fragmentPosition, 1);
}
