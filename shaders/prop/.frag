#version 430 core
out vec4 o_color;

const uint MAX_LIGHTS = 100u;
const float ambientKoeffitient = 0.05;
const float opaqueTreshold = 0.95;

struct Material
{
    sampler2D diffuse;
    sampler2D normal;
    sampler2D rough;
    float shininess;
};

// TODO: atlas
uniform samplerCube u_pointLightSamplers[5];
uniform sampler2D   u_dirLightSamplers  [5];
uniform sampler2D   u_spotLightSamplers [5];

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

uniform Material u_material;
uniform vec3 u_camPos;
layout(std140) uniform u_lights {
    uint numPointLights;
    PointLight pointLights[MAX_LIGHTS];
    uint numDirLights;
    DirLight dirLights[MAX_LIGHTS];
    uint numSpotLights;
    SpotLight spotLights[MAX_LIGHTS];
};
uniform vec4 u_color;

vec3 calculateLight(PointLight light, samplerCube depthMap, Material material, vec3 normal, vec3 viewDir, vec2 texCoords, vec3 fragPos);
vec3 calculateLight(DirLight light, sampler2D depthMap, Material material, vec3 normal, vec3 viewDir, vec2 texCoords, vec3 fragPos);
vec3 calculateLight(SpotLight light, sampler2D depthMap, Material material, vec3 normal, vec3 viewDir, vec2 texCoords, vec3 fragPos);

void main() 
{
    vec2 texCoords = fs_in.texCoords;
    vec3 viewDir = normalize(u_camPos - fs_in.fragPos);
    vec3 normal = normalize(fs_in.TBN * normalize(texture(u_material.normal, texCoords).rgb * 2.0 - 1.0));
    vec3 fragPos = fs_in.fragPos;

    o_color = texture(u_material.diffuse, texCoords) * u_color;

    if(o_color.a < opaqueTreshold) discard;

    vec3 lightColor = vec3(0);
    // for(uint i = 0u; i < numPointLights; ++i) {
    //     lightColor += calculateLight(pointLights[i], u_pointLightSamplers[i], u_material, normal, viewDir, texCoords, fragPos).xyz;
    // }
    for(uint i = 0u; i < numDirLights; ++i) {
        lightColor += calculateLight(dirLights[i], u_dirLightSamplers[i], u_material, normal, viewDir, texCoords, fragPos).rgb;
    }
    for(uint i = 0u; i < numSpotLights; ++i) {
        lightColor += calculateLight(spotLights[i], u_spotLightSamplers[i], u_material, normal, viewDir, texCoords, fragPos).xyz;
    }

    o_color *= vec4(lightColor, 1);
    // o_color = vec4(lightColor, 1);
}

vec3 calculateShadow(PointLight light, samplerCube depthMap, vec3 fragPos, vec3 normal);
vec3 calculateShadow(DirLight light, sampler2D depthMap, vec3 fragPos, vec3 normal);
vec3 calculateShadow(SpotLight light, sampler2D depthMap, vec3 fragPos, vec3 normal);

vec3 calculateLight(PointLight light, samplerCube depthMap, Material material, vec3 normal, vec3 viewDir, vec2 texCoords, vec3 fragPos) 
{
    vec3 lightDir = normalize(light.position - fragPos);
    normal = normalize(normal);
    viewDir = normalize(viewDir);

    float distanceLightFragment = length(light.position - fragPos);
    float attenuation = 1.0 / (light.attenuation * distanceLightFragment * distanceLightFragment);

    vec3 ambient = light.color * ambientKoeffitient * attenuation;
    vec3 diffuse = 
        light.color * 
        attenuation *
        vec3(max(dot(normal, lightDir), 0.0));
    vec3 specular = 
        light.color * 
        attenuation *
        pow(max(dot(normal, normalize(lightDir + viewDir)), 0.0), u_material.shininess) *
        vec3(1 - texture(material.rough, texCoords));
    vec3 shadow = calculateShadow(light, depthMap, fragPos, normal);
return shadow; // REMOVE ME
    return vec3(ambient + (1 - shadow) * (diffuse + specular));
}
vec3 calculateLight(DirLight light, sampler2D depthMap, Material material, vec3 normal, vec3 viewDir, vec2 texCoords, vec3 fragPos) 
{
    vec3 lightDir = normalize(-light.direction);

    vec3 ambient = light.color * ambientKoeffitient;
    vec3 diffuse = 
        light.color * 
        vec3(max(dot(normal, lightDir), 0.0));
    vec3 specular = 
        light.color * 
        pow(max(dot(normal, normalize(lightDir + viewDir)), 0.0), u_material.shininess) * 
        vec3(1 - texture(material.rough, texCoords));
    vec3 shadow = calculateShadow(light, depthMap, fragPos, normal);

    return ambient + (1 - shadow) * (diffuse + specular);
}
vec3 calculateLight(SpotLight light, sampler2D depthMap, Material material, vec3 normal, vec3 viewDir, vec2 texCoords, vec3 fragPos)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float distanceLightFragment = length(light.position - fragPos);
    float attenuation = 1.0 / (light.attenuation * distanceLightFragment * distanceLightFragment);

    float theta = dot(lightDir, normalize(-light.direction));
    vec3 ambient = light.color * ambientKoeffitient * attenuation * max(theta, 0);
    if(theta > light.outerConeAngle) {
        float epsilon = light.innerConeAngle - light.outerConeAngle;
        float intensity = clamp((theta - light.outerConeAngle) / epsilon, 0.0, 1.0);

        vec3 diffuse = 
            light.color * 
            intensity *
            attenuation *
            vec3(max(dot(normal, lightDir), 0.0));
        vec3 specular = 
            light.color *
            intensity * 
            attenuation *
            pow(max(dot(normal, normalize(lightDir + viewDir)), 0.0), u_material.shininess) * 
            vec3(1 - texture(material.rough, texCoords));
        vec3 shadow = calculateShadow(light, depthMap, fragPos, normal);

        return ambient + (1 - shadow) * (diffuse + specular);
    } else {
        return ambient;
    }
}

// array of offset direction for sampling
const vec3 gridSamplingDisk[20] = vec3[]
(
   vec3(1, 1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1, 1,  1), 
   vec3(1, 1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
   vec3(1, 1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1, 1,  0),
   vec3(1, 0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1, 0, -1),
   vec3(0, 1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0, 1, -1)
);
vec3 calculateShadow(PointLight light, samplerCube depthMap, vec3 fragPos, vec3 normal)
{
    vec3 fragToLight = fragPos - light.position;
    float currentDepth = length(fragToLight);

    float shadow = 0.0;
    float bias = 0.15;
    int samples = 20;
    float viewDistance = length(u_camPos - fragPos);

    float diskRadius = (1.0 + (viewDistance / light.farPlane)) / 25.0;
    for(int i = 0; i < samples; ++i)
    {
        float closestDepth = texture(depthMap, normalize(fragToLight + gridSamplingDisk[i] * diskRadius)).r;
        closestDepth *= light.farPlane;   // undo mapping [0;1]
        if(currentDepth - bias > closestDepth)
            shadow += 1.0;
    }
    shadow /= float(samples);

    return vec3(shadow);
}
vec3 calculateShadow(DirLight light, sampler2D depthMap, vec3 fragPos, vec3 normal)
{
    vec4 fragPosLightSpace = light.viewProj * vec4(fragPos, 1);
    vec3 projectedCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projectedCoords = projectedCoords * 0.5 + 0.5;
    if(projectedCoords.z > 1.0)
        return vec3(0.0);

    float currentDepth = projectedCoords.z;
    // very primitive PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(depthMap, 0);
    float bias = max(0.05 * (1.0 - dot(normal, normalize(-light.direction))), 0.005);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float closestDepth = texture(depthMap, projectedCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > closestDepth ? 1.0f : 0.0f;
        }    
    }
    shadow /= 9.0;
        
    return vec3(shadow);
}
vec3 calculateShadow(SpotLight light, sampler2D depthMap, vec3 fragPos, vec3 normal)
{
    vec4 fragPosLightSpace = light.viewProj * vec4(fragPos, 1);
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
    float bias = 0.001;
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float closestDepth = texture(depthMap, projectedCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > closestDepth ? 1.0f : 0.0f;
        }    
    }
    shadow /= 9.0;

    return vec3(shadow);
}
