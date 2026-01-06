#ifndef LIGHTS_GLSL
#define LIGHTS_GLSL

struct PointLight
{
    vec3 color;
    float _pad0;
    vec3 position;
    float _pad1;
    uvec2 depthMapPos;
    uint depthMapSize;
    float farPlane;
};
struct DirLight
{
    vec3 color;
    float _pad0;
    vec3 direction;
    float _pad1;
    uvec2 depthMapPos;
    uint depthMapSize;
    float _pad2;
    vec4 _pad3;
    mat4 viewProj;
};
struct SpotLight
{
    vec3 color;     
    float _pad0;
    vec3 position;  
    float _pad1;
    vec3 direction; 
    float innerConeAngle;
    uvec2 depthMapPos;
    uint depthMapSize;
    float outerConeAngle;
    mat4 viewProj;
};

#endif