#pragma once
#include <filesystem>
#include <fstream>
#include "utils/Model.hpp"
#include "utils/Text.hpp"
#include "json.hpp"

namespace game
{
    struct Scene
    {
        std::set<ecs::Entity_t> containedEntities;
        std::filesystem::path filePath;
    };

    class ILevelEntityCreator
    {
    public:
        virtual ~ILevelEntityCreator() = default;
        virtual void create(nlohmann::json const &j, game::Scene &scene) = 0;
    };
    class LevelParser
    {
    private:

        std::map<std::string, std::unique_ptr<ILevelEntityCreator>> m_creators;
        inline void registerCreator(std::string_view name, std::unique_ptr<ILevelEntityCreator> creator) {
            m_creators.emplace(name, std::move(creator));
        }
    public:
        LevelParser();
        ~LevelParser();
        Scene parseScene(std::filesystem::path const &filepath);
    };
    inline LevelParser &getLevelParser() {
        // need to deallocate textures and stuff before ogl context destruction
        static LevelParser *parser = new LevelParser;
        return *parser;
    }
} // namespace game

