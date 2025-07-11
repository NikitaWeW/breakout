#version 330 core
in vec4 v_fragPos;

uniform vec3 lightPos;
uniform float u_farPlane;

void main()
{
    float lightDistance = length(v_fragPos.xyz - lightPos);
    
    // map to [0;1] range by dividing by u_farPlane
    lightDistance = lightDistance / u_farPlane;
    
    // write this as modified depth
    gl_FragDepth = lightDistance;
}