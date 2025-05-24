#version 330 core
out vec4 o_color;

const uint MAX_LIGHTS = 100u;
const float ambientKoeffitient = 0.125;

struct Material
{
    sampler2D diffuse;
    sampler2D normal;
    sampler2D rough;
    float shininess;
};
struct PointLight
{
    vec3 color;
    float attenuation;
    vec3 position;
    float _pad0;
};
struct DirLight
{
    vec3 direction;
    float _pad0;
    vec3 color;
    float _pad1;
};

in VS_OUT {
    vec2 texCoords;
    vec3 fragPos;
    mat3 TBN;
} fs_in;

uniform Material u_material;
uniform vec3 u_camPos;
layout(std140) uniform u_lights {
    uint numPointLights;
    PointLight pointLights[MAX_LIGHTS];
    uint numDirLights;
    DirLight dirLights[MAX_LIGHTS];
};

vec4 calculateLight(PointLight light, Material material, vec3 normal, vec3 viewDir, vec2 texCoords, vec3 fragPos);
vec4 calculateLight(DirLight light, Material material, vec3 normal, vec3 viewDir, vec2 texCoords, vec3 fragPos);

void main() 
{
    vec2 texCoords = fs_in.texCoords;
    vec3 viewDir = normalize(u_camPos - fs_in.fragPos);
    vec3 normal = normalize(fs_in.TBN * normalize(texture(u_material.normal, texCoords).rgb * 2.0 - 1.0));
    normal = fs_in.TBN[2];
    vec3 fragPos = fs_in.fragPos;

    vec3 lightColor = vec3(0);
    for(uint i = 0u; i < numPointLights; ++i) {
        lightColor += calculateLight(pointLights[i], u_material, normal, viewDir, texCoords, fragPos).xyz;
    }
    for(uint i = 0u; i < numDirLights; ++i) {
        lightColor += calculateLight(dirLights[i], u_material, normal, viewDir, texCoords, fragPos).xyz;
    }

    o_color = texture(u_material.diffuse, texCoords) * vec4(lightColor, 1);
    o_color.a = 1;

    if(o_color.a < 1e-5) discard;
    o_color.rgb = pow(o_color.rgb, vec3(1/2.2)); // apply gamma correction
}

vec4 calculateLight(PointLight light, Material material, vec3 normal, vec3 viewDir, vec2 texCoords, vec3 fragPos) 
{
    vec3 lightDir = normalize(light.position - fragPos);
    normal = normalize(normal);
    viewDir = normalize(viewDir);

    float distanceLightFragment = length(light.position - fragPos);
    float attenuation = 1.0 / (light.attenuation * distanceLightFragment * distanceLightFragment);

    vec3 ambient = 
        light.color * ambientKoeffitient * 
        attenuation;
    vec3 diffuse = 
        light.color * 
        attenuation *
        vec3(max(dot(normal, lightDir), 0.0));
    vec3 specular = 
        light.color * 
        attenuation *
        pow(max(dot(normal, normalize(lightDir + viewDir)), 0.0), u_material.shininess) *
        vec3(1 - texture(material.rough, texCoords));
    float shadow = 0;

    return vec4(ambient + (1 - shadow) * (diffuse + specular), 1.0);
}
vec4 calculateLight(DirLight light, Material material, vec3 normal, vec3 viewDir, vec2 texCoords, vec3 fragPos) 
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
    float shadow = 0;

    return vec4(ambient + (1 - shadow) * (diffuse + specular), 1.0);
}

