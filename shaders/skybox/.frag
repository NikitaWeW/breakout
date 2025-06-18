#version 430 core
layout (depth_greater) out float gl_FragDepth;


in vec3 v_texCoords;

out vec4 o_color;

layout(binding = 0) uniform samplerCube u_skybox;

void main() {
    o_color = texture(u_skybox, v_texCoords);
    // o_color = vec4(vec3((texture(skybox, v_texCoords) * 10).r), 1); // omni-directional shadow depth cubemap debug
    gl_FragDepth = 1.0;
}