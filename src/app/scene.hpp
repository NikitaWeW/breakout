#pragma once
#include "engine/DSA/ECS.hpp"
#include "engine/Engine.hpp"
#include "nlohmann/json-schema.hpp"
#include <unordered_set>

struct Scene
{
    std::string path; /// The path to the json file the scene was loaded from
    std::string name; /// The name of the scene
    bool valid = false; /// Indicates whether the scene was loaded correctly
    nlohmann::json data; /// The json file the scene was loaded from
    std::vector<engine::Entity> entities; /// A list of entties describing the scene.
    std::unordered_set<engine::Entity> models; /// A set of (cached) models.
};

class SceneLoader : public engine::Handle<struct SceneLoaderImpl>
{
public:
    /// @brief Construct an invalid scene loader.
    SceneLoader() = default;
    /// @brief Construct a valid scene loader.
    SceneLoader(engine::Registry &reg, std::string_view schemaPath = "res/scenes/schema.json");

    /// @brief Load the scene json file.
    Scene load(std::string_view path);
};

void createScene(engine::Registry &reg);
void updateScene(engine::Registry &reg, float deltatime);


inline std::string printTexture(engine::Entity e_texture)
{
    std::stringstream ss;
    auto const &texture = e_texture.get<engine::Texture>();
    ss 
        << 'e' << e_texture.entity() << ", \"" 
        << texture.path << "\", \t" 
        << texture.data.getWidth() << "x" << texture.data.getHeight() << ", \t" 
        << (texture.srgb ? "srgb" : "not srgb");
    return ss.str();
}
inline void printModelData(engine::Entity e_model)
{
    ENGINE_ASSERT(e_model.has<engine::Model>());
    engine::Model const &model = e_model.get<engine::Model>();
    ENGINE_INFO("");
    ENGINE_INFO("Model: e{}: \"{}\"", e_model.entity(), model.path);
    ENGINE_INFO("Skeleton: ");
    ENGINE_INFO("  Bone map size / number of bones: {}", model.skeleton.boneMap.size());
    if(model.skeleton.boneMap.size() <= 30)
        for(auto const &[name, id] : model.skeleton.boneMap)
            ENGINE_INFO("    [\"{}\": {}]", name, id);

    ENGINE_INFO("Animations: {}", model.animations.size());
    for(auto const &animation : model.animations)
    {
        ENGINE_INFO("-----------------");
        ENGINE_INFO("Animation: \"{}\"", animation.name);
        ENGINE_INFO("  Duration: {} ticks, tps: {}", animation.durationTicks, animation.ticksPerSecond);
        ENGINE_INFO("  Bones size: {}", animation.bones.size());
    }

    ENGINE_INFO("Meshes: {}", model.meshes.size());
    for(auto const &mesh : model.meshes)
    {
        ENGINE_INFO("-----------------");

        ENGINE_INFO("Geometry:");
        ENGINE_INFO("  Triangles: {}", mesh.geometry.indices.size() / 3);
        ENGINE_INFO("  Indices:   {}", mesh.geometry.indices.size());
        ENGINE_INFO("  Positions: {}", mesh.geometry.positions.size());
        ENGINE_INFO("  TexCoords: {}", mesh.geometry.texCoords.size());
        ENGINE_INFO("  Normals:   {}", mesh.geometry.normals.size());
        ENGINE_INFO("  Tangents:  {}", mesh.geometry.tangents.size());
        ENGINE_INFO("  BoneIDs:   {}", mesh.geometry.boneIDs.size());
        ENGINE_INFO("  Weights:   {}", mesh.geometry.weights.size());
        
        ENGINE_INFO("Material:");
        ENGINE_INFO("Textures:");
        ENGINE_INFO("  Albedo:       {}", printTexture(engine::Entity{e_model.reg(), mesh.material.textures.albedo}));
        ENGINE_INFO("  Metallic:     {}", printTexture(engine::Entity{e_model.reg(), mesh.material.textures.metallic}));
        ENGINE_INFO("  Roughness:    {}", printTexture(engine::Entity{e_model.reg(), mesh.material.textures.roughness}));
        ENGINE_INFO("  Ambient:      {}", printTexture(engine::Entity{e_model.reg(), mesh.material.textures.ambient}));
        ENGINE_INFO("  Normal:       {}", printTexture(engine::Entity{e_model.reg(), mesh.material.textures.normal}));
        ENGINE_INFO("  Displacement: {}", printTexture(engine::Entity{e_model.reg(), mesh.material.textures.displacement}));
        ENGINE_INFO("  Alpha:        {}", printTexture(engine::Entity{e_model.reg(), mesh.material.textures.alpha}));
        ENGINE_INFO("Properties:");
        ENGINE_INFO("  Ambient:       {}", fmt::streamed(mesh.material.properties.ambient));
        ENGINE_INFO("  Albedo:        {}", fmt::streamed(mesh.material.properties.albedo));
        ENGINE_INFO("  Specular:      {}", fmt::streamed(mesh.material.properties.specular));
        ENGINE_INFO("  Emission:      {}", fmt::streamed(mesh.material.properties.emission));
        ENGINE_INFO("  Shininess:     {}", mesh.material.properties.shininess);
        ENGINE_INFO("  Metallic:      {}", mesh.material.properties.metallic);
        ENGINE_INFO("  IOR:           {}", mesh.material.properties.ior);
    }
}
