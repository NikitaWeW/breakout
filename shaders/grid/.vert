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
const int indices[6] = int[6](
    0, 1, 2,
    0, 2, 3
);

const float gridSize = 100;
const float gridHeight = -0.5;

void main() {
    vec3 vertexPosition = vertices[indices[gl_VertexID]] * gridSize;
    vertexPosition.x += u_cameraPosition.x;
    vertexPosition.y = gridHeight;
    vertexPosition.z += u_cameraPosition.z;
    vs_out.fragmentPosition = vertexPosition;
    vs_out.cameraPosition = u_cameraPosition;
    vs_out.texCoord = texCoords[indices[gl_VertexID]];
    gl_Position = u_projectionMat * u_viewMat * vec4(vs_out.fragmentPosition, 1);
}
