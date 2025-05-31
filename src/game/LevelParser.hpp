#pragma once
#include <filesystem>
#include <fstream>
#include "utils/ECS.hpp"
#include "utils/Model.hpp"
#include "utils/Text.hpp"

namespace game
{
    struct Scene
    {
        std::set<ecs::Entity_t> containedEntities;
        std::filesystem::path filePath;
    };
    class LevelParser
    {
    private:
        std::string m_errorStr = "";
        std::map<std::filesystem::path, model::Model> m_modelCache;
        std::map<std::filesystem::path, opengl::Texture> m_textureCache;
        std::map<std::pair<std::filesystem::path, std::filesystem::path>, text::Font> m_fontCache;
        ecs::Entity_t createModel(std::filesystem::path const &filepath, bool flipWindingOrder, bool flipTextures);
        text::Font &createFont(std::filesystem::path atlas, std::filesystem::path metadata);
        void addTexture(ecs::Entity_t const &modelEntity, std::filesystem::path const &path, std::string const &type, bool flipTextures);
    public:
        LevelParser() = default;
        ~LevelParser();
        Scene parseScene(std::filesystem::path const &filepath);
        inline std::string const &getErrorString() const { return m_errorStr; }
        inline void clearError() { m_errorStr = ""; }
    };
    inline LevelParser &getLevelParser() {
        static LevelParser *parser = new LevelParser;
        return *parser;
    }
} // namespace game

