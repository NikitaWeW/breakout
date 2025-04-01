#version 330 core
layout(location = 0) in vec2 a_position;
uniform vec2 u_glyphMin;
uniform vec2 u_glyphMax;

void main() {
    vec2 size = u_glyphMax - u_glyphMin;
    vec2 positionUV = (a_position.xy - u_glyphMin) / size;
    vec2 positionNDC = positionUV * 2.0 - 1.0;
    gl_Position = vec4(positionNDC, 0, 1);
}
