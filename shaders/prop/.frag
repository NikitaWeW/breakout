#version 430 core
#extension GL_ARB_shading_language_include: enable // So validators don't freak out

#include "../lighting/calculateLight.glsl"


layout (location = 0) out vec4  o_color;
layout (location = 1) out vec4  o_accumulation;
layout (location = 2) out float o_revealage;

const float opaqueThreshold = 0.95;

struct MaterialTextures 
{
    sampler2D albedo;
    sampler2D normal;
    sampler2D roughness;
    sampler2D metallic;
};
struct MaterialProperties
{
    float shininess;
    vec4 albedo;
};
struct Material
{
    MaterialTextures textures;
    MaterialProperties properties;
};

layout(std430, binding = 0) buffer PointLightsSSBO {
    PointLight pointLights[];
};
layout(std430, binding = 1) buffer DirLightsSSBO {
    DirLight dirLights[];
};
layout(std430, binding = 2) buffer SpotLightsSSBO {
    SpotLight spotLights[];
};


layout(binding = 1) uniform sampler2D u_shadowMapAtlas;
// No idea why pointLights.length() doesn't work.
uniform uint u_numPointLights;
uniform uint u_numDirLights;
uniform uint u_numSpotLights;
uniform mat4 u_viewMat;
uniform Material u_material;
uniform bool u_transparent = false;

in VS_OUT {
    vec2 texCoords;
    vec3 fragPos;
    mat3 TBN;
} fs_in;

void main() 
{
    vec2 texCoords = fs_in.texCoords;
    vec3 viewDir = normalize(-u_viewMat[3].xyz - fs_in.fragPos);
    vec3 normal = fs_in.TBN * normalize(texture(u_material.textures.normal, texCoords).rgb * 2.0 - 1.0);
    normal = fs_in.TBN[2];

    vec4 color = texture(u_material.textures.albedo, texCoords) * u_material.properties.albedo;

    if(u_transparent && color.a > opaqueThreshold) discard;
    if(!u_transparent && color.a < opaqueThreshold) discard;

    vec3 lightColor = vec3(0);
    for(uint i = 0u; i < u_numPointLights; ++i) {
        lightColor += calculateLight(pointLights[i], normal, viewDir, texCoords, fs_in.fragPos, u_shadowMapAtlas);
    }
    for(uint i = 0u; i < u_numDirLights; ++i) {
        lightColor += calculateLight(dirLights[i], normal, viewDir, texCoords, fs_in.fragPos, u_shadowMapAtlas);
    }
    for(uint i = 0u; i < u_numSpotLights; ++i) {
        lightColor += calculateLight(spotLights[i], normal, viewDir, texCoords, fs_in.fragPos, u_shadowMapAtlas);
    }

    color.rgb *= lightColor;
    // color.rgb = lightColor;
    // color.rgb = calculateLight(pointLights[0], normal, viewDir, texCoords, fs_in.fragPos, u_shadowMapAtlas);

    if(u_transparent) {
        // float weight = max(min(1.0, max(max(color.r, color.g), color.b) * color.a), color.a) * clamp(0.03 / (1e-5 + pow(gl_FragCoord.z / 200, 4.0)), 1e-2, 3e3);
        float weight = clamp(pow(min(1.0, color.a * 10.0) + 0.01, 3.0) * 1e8 * pow(1.0 - gl_FragCoord.z * 0.9, 3.0), 1e-2, 3e3);
        o_accumulation = vec4(color.rgb * color.a, color.a) * weight;
        o_revealage = color.a;
    } else {
        o_color = color;
    }
}
