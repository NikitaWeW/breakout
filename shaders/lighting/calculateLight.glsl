#extension GL_ARB_shading_language_include: enable // So validators dont freak out

#ifndef CALCULATE_LIGHT_GLSL
#define CALCULATE_LIGHT_GLSL

const float ambientCoefficient = .05;

#include "lights.glsl"

#define BOOL_VEC2(x) ((x) ? vec2(0,1) : vec2(1,0))

vec4 sampleAtlas(sampler2D atlas, vec2 texcoords, uvec2 pos, uint size)
{
    texcoords = clamp(texcoords, 0, 1);
    // return vec4(vec2(texcoords * float(size) + pos) / textureSize(atlas, 0), 0,1);
    return texelFetch(atlas, ivec2(texcoords * float(size) + pos), 0);
}

int getCubeFaceIndex(vec3 dir)
{
    vec3 adir = abs(dir);
    if (adir.x > adir.y && adir.x > adir.z)
        return (dir.x > 0) ? 0 : 1;
    else if (adir.y > adir.z)
        return (dir.y > 0) ? 2 : 3;
    return (dir.z > 0) ? 4 : 5;
}

/// @return vec3(uv.x, uv.y, faceIndex)
vec3 dirToUV(vec3 dir) 
{
    dir = normalize(dir);
    vec3 adir = abs(dir);

    int face = getCubeFaceIndex(dir);
    vec2 uv;

    switch (face)
    {
    case 0: uv = vec2(-dir.z, dir.y) / adir.x; break; // +X
    case 1: uv = vec2( dir.z, dir.y) / adir.x; break; // -X
    case 2: uv = vec2( dir.x, dir.z) / adir.y; break; // +Y
    case 3: uv = vec2( dir.x,-dir.z) / adir.y; break; // -Y
    case 4: uv = vec2(-dir.x, dir.y) / adir.z; break; // +Z
    case 5: uv = vec2( dir.x, dir.y) / adir.z; break; // -Z
    }

    uv = uv * .5 + .5;
    return vec3(uv, face);
}
vec4 sampleAtlas(sampler2D atlas, vec3 dir, uvec2 pos, uint size)
{
    vec3 uvLayer = dirToUV(dir);
    // return vec4(dir, 1);
    // return vec4(vec3(uvLayer.z / 5), 1);
    uvec2 base = pos + ivec2(uvLayer.z * size, 0);

    return sampleAtlas(atlas, uvLayer.xy, base, size);
}

// array of offset directions for sampling
const vec3 gridSamplingDisk[20] = vec3[]
(
    vec3(1, 1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1, 1,  1), 
    vec3(1, 1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
    vec3(1, 1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1, 1,  0),
    vec3(1, 0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1, 0, -1),
    vec3(0, 1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0, 1, -1) 
);

float linearizeDepth(float depth, float near, float far) { return (2.0 * near * far) / (far + near - (depth * 2.0 - 1.0) * (far - near)); }

vec3 calculateShadow(PointLight light, vec3 normal, vec3 viewDir, vec3 fragPos, sampler2D atlas)
{
    vec3 lightToFrag = fragPos - light.position;
    float currentDepth = length(lightToFrag) / light.farPlane;
    // return vec3(currentDepth);

    float shadow = 0.0;
    // float bias = 1e-2 * (1.0 - dot(normal, normalize(-lightToFrag)));
    // float bias = mix(10, 20, 0.5 - abs(dot(viewDir, normal))) * max(abs(dFdx(currentDepth)), abs(dFdy(currentDepth)));
    float bias = 1e-2;
    float diskRadius = (1.0 + currentDepth) * 0.005;

    for(int i = 0; i < gridSamplingDisk.length(); ++i)
    {
        float closestDepth = sampleAtlas(atlas, normalize(lightToFrag) + normalize(gridSamplingDisk[i]) * diskRadius, light.depthMapPos, light.depthMapSize).r;
        shadow += float(currentDepth - bias > closestDepth);
        // return vec3(closestDepth);
    }
    shadow /= float(gridSamplingDisk.length());

    return vec3(shadow);
}
vec3 calculateShadow(DirLight light, vec3 normal, vec3 viewDir, vec3 fragPos, sampler2D atlas)
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

    // very primitive PCF
    float shadow = 0.0;
    vec2 texelSize = vec2(1.0 / light.depthMapSize);
    // float bias = mix(1e-1, 1e-6, mix(0.5, 0.9, abs(dot(normal, light.direction))));
    // float bias = 0.001 * max(abs(dFdx(projectedCoords.z)), abs(dFdy(projectedCoords.z)));
    float bias = 0.001 * (1.0 - dot(normal, light.direction));
    // float bias = 0.0001;
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float closestDepth = sampleAtlas(atlas, projectedCoords.xy + vec2(x, y) * texelSize, light.depthMapPos, light.depthMapSize).r;

            shadow += float(currentDepth - bias > closestDepth);
        }
    }
    shadow /= 9.0;
        
    return vec3(shadow);
}
vec3 calculateShadow(SpotLight light, vec3 normal, vec3 viewDir, vec3 fragPos, sampler2D atlas)
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
// return vec3(projectedCoords.xy, 0);

    float currentDepth = projectedCoords.z;
// return vec3(currentDepth);
    float shadow = 0.0;
    vec2 texelSize = vec2(1.0 / light.depthMapSize) * 2;
    // float bias = mix(0, 30, 1 - abs(dot(viewDir, normal))) * max(abs(dFdx(projectedCoords.z)), abs(dFdy(projectedCoords.z)));
    float bias = 1e-4 * (1.0 - dot(normal, light.direction));
    // float bias = 0;
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float closestDepth = sampleAtlas(atlas, projectedCoords.xy + vec2(x, y) * texelSize, light.depthMapPos, light.depthMapSize).r;
// return vec3(closestDepth);
            shadow += float(currentDepth - bias > closestDepth);
        }    
    }
    shadow /= 9.0;

    return vec3(shadow);
}

// TODO: PBR
vec3 calculateLight(PointLight light, vec3 normal, vec3 viewDir, vec2 texCoords, vec3 fragPos, sampler2D atlas) 
{
    vec3 lightDir = normalize(light.position - fragPos);
    normal = normalize(normal);

    float distanceLightFragment = length(light.position - fragPos);
    float attenuation = 1 / (distanceLightFragment * distanceLightFragment);

    // vec3 ambient = light.color * ambientCoefficient * attenuation;
    vec3 ambient = vec3(0);
    vec3 diffuse = 
        light.color * 
        attenuation *
        vec3(max(dot(normal, lightDir), 0.0));
    vec3 specular = vec3(0); // dunno it just doesn't work and gives nans
    vec3 shadow = calculateShadow(light, normal, viewDir, fragPos, atlas);
// return shadow;

    return vec3(ambient + (1 - shadow) * (diffuse + specular));
}
vec3 calculateLight(DirLight light, vec3 normal, vec3 viewDir, vec2 texCoords, vec3 fragPos, sampler2D atlas) 
{
    vec3 ambient = light.color * ambientCoefficient;
    vec3 diffuse = 
        light.color * 
        vec3(max(dot(normal, -light.direction), 0.0));
    vec3 specular = vec3(0); // dunno it just doesn't work and gives nans
    vec3 shadow = calculateShadow(light, normal, viewDir, fragPos, atlas);
// return shadow;

    return ambient + (1 - shadow) * (diffuse + specular);
}
vec3 calculateLight(SpotLight light, vec3 normal, vec3 viewDir, vec2 texCoords, vec3 fragPos, sampler2D atlas)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float distanceLightFragment = length(light.position - fragPos);
    float attenuation = 1 / (distanceLightFragment * distanceLightFragment);

    float theta = dot(lightDir, normalize(-light.direction));
    // return vec3(theta);
    vec3 ambient = light.color * ambientCoefficient * attenuation;
    if(theta > light.outerConeAngle) {
        float epsilon = light.innerConeAngle - light.outerConeAngle;
        float intensity = (theta - light.outerConeAngle) / max(epsilon, 1e-6);

        vec3 diffuse = 
            light.color * 
            intensity *
            attenuation *
            vec3(max(dot(normal, lightDir), 0.0));
        vec3 specular = vec3(0); // dunno it just doesn't work and gives nans
        vec3 shadow = calculateShadow(light, normal, viewDir, fragPos, atlas);
// return shadow;
        // return vec3(1);
        return ambient + (1 - shadow) * (diffuse + specular);
    } else {
        // return vec3(0);
        return ambient;
    }
}

#endif // #ifndef CALCULATE_LIGHT_GLSL