#pragma once
#include <filesystem>
#include <fstream>
#include "utils/ECS.hpp"
#include "utils/Model.hpp"

namespace game
{
    class LevelParser
    {
    private:
        std::string m_errorStr = "";
        std::map<std::filesystem::path, model::Model> m_modelCache;
        std::map<std::filesystem::path, opengl::Texture> m_textureCache;
        ecs::Entity_t createModel(std::filesystem::path const &filepath, bool flipWindingOrder, bool flipTextures);
        void addTexture(ecs::Entity_t const &modelEntity, std::filesystem::path const &path, std::string const &type, bool flipTextures);
    public:
        LevelParser() = default;
        ~LevelParser();
        std::optional<std::vector<ecs::Entity_t>> parceScene(std::filesystem::path const &filepath);
        inline std::string const &getErrorString() const { return m_errorStr; }
        inline void clearError() { m_errorStr = ""; }
    };
    
} // namespace game

