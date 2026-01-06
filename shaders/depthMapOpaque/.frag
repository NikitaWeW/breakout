#version 430 core

in VS_OUT {
    flat int omnidirectional;
    flat vec3 lightPos;
    flat float zfar;
    vec3 fragPos;
} fs_in;

layout (depth_any) out float gl_FragDepth;

void main() 
{
    if(bool(fs_in.omnidirectional))
        gl_FragDepth = length(fs_in.fragPos.xyz - fs_in.lightPos) / fs_in.zfar;
    else 
        gl_FragDepth = gl_FragCoord.z;
}