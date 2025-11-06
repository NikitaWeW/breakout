#version 430 core
layout (location = 0) out vec4  o_color;
layout (location = 1) out vec4  o_accumulation;
layout (location = 2) out float o_revealage;

uniform bool u_transparent = false;

const uint MAX_LIGHTS = 5u;
const float ambientKoeffitient = 0.05;
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

// TODO: atlas
layout( binding = 5) uniform samplerCube u_pointLightSamplers[MAX_LIGHTS];
layout( binding = 10) uniform sampler2D   u_dirLightSamplers  [MAX_LIGHTS];
layout( binding = 15) uniform sampler2D   u_spotLightSamplers [MAX_LIGHTS];

struct PointLight
{
    vec3 color;
    float attenuation;
    vec3 position;
    float farPlane;
}; // 64 bytes
struct DirLight
{
    vec3 direction;
    float _pad0;
    vec3 color;
    float _pad1;
    mat4 viewProj;
}; // 96 bytes
struct SpotLight
{
    vec3 position;
    float innerConeAngle;
    vec3 direction;
    float outerConeAngle;
    vec3 _pad0;
    float attenuation;
    vec3 color;
    float _pad1;
    mat4 viewProj;
}; // 128 bytes

in VS_OUT {
    vec2 texCoords;
    vec3 fragPos;
    flat mat3 TBN;
} fs_in;

uniform mat4 u_viewMat;
uniform Material u_material;
layout(std140) uniform u_lights {
    uint numPointLights;
    PointLight pointLights[MAX_LIGHTS];
    uint numDirLights;
    DirLight dirLights[MAX_LIGHTS];
    uint numSpotLights;
    SpotLight spotLights[MAX_LIGHTS];
};

// TODO: PBR
vec3 calculateLight(PointLight light, samplerCube depthMap, vec3 normal, vec3 viewDir, vec2 texCoords);
vec3 calculateLight(DirLight light, sampler2D depthMap, vec3 normal, vec3 viewDir, vec2 texCoords);
vec3 calculateLight(SpotLight light, sampler2D depthMap, vec3 normal, vec3 viewDir, vec2 texCoords);

void main() 
{
    vec2 texCoords = fs_in.texCoords;
    vec3 viewDir = normalize(-u_viewMat[3].xyz - fs_in.fragPos);
    vec3 normal = fs_in.TBN * normalize(texture(u_material.textures.normal, texCoords).rgb * 2.0 - 1.0);

    vec4 color = texture(u_material.textures.albedo, texCoords) * u_material.properties.albedo;

    if(u_transparent && color.a > opaqueThreshold) discard;
    if(!u_transparent && color.a < opaqueThreshold) discard;

    vec3 lightColor = vec3(0);
    for(uint i = 0u; i < numPointLights; ++i) {
        lightColor += calculateLight(pointLights[i], u_pointLightSamplers[i], normal, viewDir, texCoords).xyz;
    }
    for(uint i = 0u; i < numDirLights; ++i) {
        lightColor += calculateLight(dirLights[i], u_dirLightSamplers[i], normal, viewDir, texCoords).rgb;
    }
    for(uint i = 0u; i < numSpotLights; ++i) {
        lightColor += calculateLight(spotLights[i], u_spotLightSamplers[i], normal, viewDir, texCoords).xyz;
    }

    color.rgb *= lightColor;

    if(u_transparent) {
        // float weight = max(min(1.0, max(max(color.r, color.g), color.b) * color.a), color.a) * clamp(0.03 / (1e-5 + pow(gl_FragCoord.z / 200, 4.0)), 1e-2, 3e3);
        float weight = clamp(pow(min(1.0, color.a * 10.0) + 0.01, 3.0) * 1e8 * pow(1.0 - gl_FragCoord.z * 0.9, 3.0), 1e-2, 3e3);
        o_accumulation = vec4(color.rgb * color.a, color.a) * weight;
        o_revealage = color.a;
    } else {
        o_color = color;
    }
}

vec3 calculateShadow(PointLight light, samplerCube depthMap, vec3 normal, vec3 viewDir);
vec3 calculateShadow(DirLight light, sampler2D depthMap, vec3 normal, vec3 viewDir);
vec3 calculateShadow(SpotLight light, sampler2D depthMap, vec3 normal, vec3 viewDir);

vec3 calculateLight(PointLight light, samplerCube depthMap, vec3 normal, vec3 viewDir, vec2 texCoords) 
{
    vec3 lightDir = normalize(light.position - fs_in.fragPos);
    normal = normalize(normal);

    float distanceLightFragment = length(light.position - fs_in.fragPos);
    float attenuation = 1.0 / (light.attenuation * distanceLightFragment * distanceLightFragment);

    vec3 ambient = light.color * ambientKoeffitient * attenuation;
    vec3 diffuse = 
        light.color * 
        attenuation *
        vec3(max(dot(normal, lightDir), 0.0));
    vec3 specular = vec3(0); // dunno it just doesn't work and gives nans
    vec3 shadow = calculateShadow(light, depthMap, normal, viewDir);

    return vec3(ambient + (1 - shadow) * (diffuse + specular));
}
vec3 calculateLight(DirLight light, sampler2D depthMap, vec3 normal, vec3 viewDir, vec2 texCoords) 
{
    vec3 ambient = light.color * ambientKoeffitient;
    vec3 diffuse = 
        light.color * 
        vec3(max(dot(normal, -light.direction), 0.0));
    vec3 specular = vec3(0); // dunno it just doesn't work and gives nans
    vec3 shadow = calculateShadow(light, depthMap, normal, viewDir);

    return ambient + (1 - shadow) * (diffuse + specular);
}
vec3 calculateLight(SpotLight light, sampler2D depthMap, vec3 normal, vec3 viewDir, vec2 texCoords)
{
    vec3 lightDir = normalize(light.position - fs_in.fragPos);
    float distanceLightFragment = length(light.position - fs_in.fragPos);
    float attenuation = 1.0 / (light.attenuation * distanceLightFragment * distanceLightFragment);

    float theta = dot(lightDir, normalize(-light.direction));
    vec3 ambient = light.color * ambientKoeffitient * attenuation * max(theta, 0.0);
    if(theta > light.outerConeAngle) {
        float epsilon = light.innerConeAngle - light.outerConeAngle;
        float intensity = clamp((theta - light.outerConeAngle) / epsilon, 0.0, 1.0);

        vec3 diffuse = 
            light.color * 
            intensity *
            attenuation *
            vec3(max(dot(normal, lightDir), 0.0));
        vec3 specular = vec3(0); // dunno it just doesn't work and gives nans
        vec3 shadow = calculateShadow(light, depthMap, normal, viewDir);

        return ambient + (1 - shadow) * (diffuse + specular);
    } else {
        return ambient;
    }
}
// float linearizeDepth(float depth, float near_plane, float far_plane) { return (2.0 * near_plane * far_plane) / (far_plane + near_plane - (depth * 2.0 - 1.0) * (far_plane - near_plane)); }

// array of offset direction for sampling
const vec3 gridSamplingDisk[20] = vec3[]
(
   vec3(1, 1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1, 1,  1), 
   vec3(1, 1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
   vec3(1, 1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1, 1,  0),
   vec3(1, 0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1, 0, -1),
   vec3(0, 1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0, 1, -1)
);
vec3 calculateShadow(PointLight light, samplerCube depthMap, vec3 normal, vec3 viewDir)
{
    return vec3(0);
    vec3 lightToFrag = fs_in.fragPos - light.position;
    float currentDepth = length(lightToFrag) / light.farPlane;

    float shadow = 0.0;
    float bias = max(0.05 * (1.0 - dot(normal, normalize(-lightToFrag))), 0.005);
    float diskRadius = (1.0 + currentDepth) * 0.005;

    for(int i = 0; i < gridSamplingDisk.length(); ++i)
    {
        float closestDepth = texture(depthMap, normalize(lightToFrag) + normalize(gridSamplingDisk[i]) * diskRadius).r;
        shadow += float(currentDepth - bias > closestDepth);
    }
    shadow /= float(gridSamplingDisk.length());

    return vec3(shadow);
}
vec3 calculateShadow(DirLight light, sampler2D depthMap, vec3 normal, vec3 viewDir)
{
    return vec3(0);
    vec4 fragPosLightSpace = light.viewProj * vec4(fs_in.fragPos, 1);
    vec3 projectedCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projectedCoords = projectedCoords * 0.5 + 0.5;
    if(
        projectedCoords.z > 1.0 ||
        projectedCoords.x < 0.0 || 
        projectedCoords.x > 1.0 ||
        projectedCoords.y < 0.0 || 
        projectedCoords.y > 1.0
    ) return vec3(0.0);

    float currentDepth = projectedCoords.z;
    // very primitive PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(depthMap, 0);
    float bias = (1.0 - max(0.0f, dot(normal, normalize(-light.direction)))) * 0.01;
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float closestDepth = texture(depthMap, projectedCoords.xy + vec2(x, y) * texelSize).r;
            shadow += float(currentDepth - bias > closestDepth);
        }
    }
    shadow /= 9.0;
        
    return vec3(shadow);
}
vec3 calculateShadow(SpotLight light, sampler2D depthMap, vec3 normal, vec3 viewDir)
{
    return vec3(0);
    vec4 fragPosLightSpace = light.viewProj * vec4(fs_in.fragPos, 1);
    vec3 projectedCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projectedCoords = projectedCoords * 0.5 + 0.5;
    if(
        projectedCoords.z > 1.0 ||
        projectedCoords.x < 0.0 || 
        projectedCoords.x > 1.0 ||
        projectedCoords.y < 0.0 || 
        projectedCoords.y > 1.0
    ) return vec3(0.0);

    float currentDepth = projectedCoords.z;

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(depthMap, 0);
    float bias = 30 * max(abs(dFdx(projectedCoords.z)), abs(dFdy(projectedCoords.z)));
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float closestDepth = texture(depthMap, projectedCoords.xy + vec2(x, y) * texelSize).r;
            shadow += float(currentDepth - bias > closestDepth);
        }    
    }
    shadow /= 9.0;

    return vec3(shadow);
}
