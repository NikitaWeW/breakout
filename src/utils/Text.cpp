#include <iostream>
#include <fstream>
#include "json.hpp"
#include "Text.hpp"

constexpr float magicValue = 6; // ??

text::Font::Font(std::filesystem::path const &atlas, std::filesystem::path const &metadata)
{
    // TODO: better atlas storage?
    // TODO: kerning
    m_textShader = opengl::ShaderProgram{"shaders/drawText", true};
    // ========
    
    m_atlas.texture = opengl::Texture{atlas, true, false, "msdf-atlas"};
    
    using json = nlohmann::json;
    std::ifstream metadataFileStream{metadata};
    if(!metadataFileStream) {
        std::cout << "failed to open \"" << metadata << "\" metadata file!\n";
        throw std::runtime_error{"unable to open file"};
    }

    json jsonMetadata = json::parse(metadataFileStream);
    
    json jsonAtlas = jsonMetadata.at("atlas");
    json jsonMetrics = jsonMetadata.at("metrics");
    json jsonGlyphs = jsonMetadata.at("glyphs");

    glm::vec2 atlasDimensions{jsonAtlas.at("width").get<float>(), jsonAtlas.at("height").get<float>()};

    m_newLineSize = jsonMetrics.at("lineHeight").get<float>() / magicValue;
    m_spaceSize = 0.05f;

    assert(atlasDimensions.x == atlasDimensions.y);
    float size = jsonAtlas.at("size").get<float>();
    float range = jsonAtlas.at("distanceRange").get<float>();
    m_pixelRange = atlasDimensions.x / size * range;

    m_atlas.glyphs = {};
    for(json &jsonGlyph : jsonGlyphs) {
        auto unicode = static_cast<wchar_t>(jsonGlyph.at("unicode").get<int>());
        if(unicode == 32) {
            m_spaceSize = jsonGlyph.at("advance").get<float>() / magicValue;
            continue;
        }
        json jsonAtlasBounds = jsonGlyph.at("atlasBounds");
        json jsonPlaneBounds = jsonGlyph.at("planeBounds");

        auto glyphData = GlyphData{
            .offset = glm::vec2{jsonAtlasBounds.at("left").get<float>(), jsonAtlasBounds[jsonAtlas.at("yOrigin").get<std::string>()].get<float>()} / atlasDimensions,
            .size = glm::abs(glm::vec2{jsonAtlasBounds.at("left").get<float>() - jsonAtlasBounds.at("right").get<float>(), jsonAtlasBounds.at("top").get<float>() - jsonAtlasBounds.at("bottom").get<float>()}) / atlasDimensions,
            .verticalOffset = jsonPlaneBounds.at("bottom").get<float>() / magicValue,
            .advance = jsonGlyph.at("advance").get<float>() / magicValue
        };

        m_atlas.glyphs.try_emplace(unicode, glyphData);
    }
}

void text::Font::drawText(std::string const &text, glm::vec2 const &position, float size, glm::vec4 const &fgColor, glm::vec4 const &bgColor, glm::mat4 const &projectionMatrix)
{ // FIXME: text only works without any objects in the scene
    struct GlyphRenderData {
        glm::vec2 quadPosition;
        glm::vec2 quadSize;
        glm::vec2 glyphOffset;
        glm::vec2 glyphSize;
    };
    std::vector<GlyphRenderData> renderData;
    {
        glm::vec2 currentPosition = {position.x, position.y};
        for(auto const &character : text) { // fuck that shit
            if(character == L' ') {
                currentPosition.x += size * m_spaceSize;
                continue;
            } else if(character == L'\n') {
                currentPosition.x = position.x;
                currentPosition.y -= m_newLineSize * size;
                continue;
            }
            auto dataIter = m_atlas.glyphs.find(character);
            if(dataIter == m_atlas.glyphs.end()) {
                std::cout << "failed to find glyph for char \'" << character << "\'!\n";
            }
            GlyphData const &data = dataIter->second;

            GlyphRenderData currentRenderData{};
            currentRenderData.quadPosition = { currentPosition.x, currentPosition.y + data.verticalOffset * size };
            currentRenderData.quadSize = { data.size.x * size, data.size.y * size };
            currentRenderData.glyphOffset = data.offset;
            currentRenderData.glyphSize = data.size;
            
            renderData.push_back(currentRenderData);
    
            currentPosition.x += data.size.x * size + m_spacing;
        }
    }

    m_textShader.bind();
    glDisable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glUniform4fv(m_textShader.getUniform("u_fgColor"), 1, &fgColor.r);
    glUniform4fv(m_textShader.getUniform("u_bgColor"), 1, &bgColor.r);
    glUniform1i (m_textShader.getUniform("u_atlas"), 0);
    glUniform1f (m_textShader.getUniform("u_screenPxRange"), m_pixelRange);
    glUniformMatrix4fv(m_textShader.getUniform("u_projMat"), 1, GL_FALSE, &projectionMatrix[0][0]);
    m_atlas.texture.bind(0);
    opengl::VertexBuffer renderDataBuffer{renderData.size() * sizeof(GlyphRenderData), renderData.data()};
    opengl::VertexArray quadVAO{renderDataBuffer, opengl::InterleavedInstancingVertexBufferLayout{ 
        {2, GL_FLOAT, 1},
        {2, GL_FLOAT, 1},
        {2, GL_FLOAT, 1},
        {2, GL_FLOAT, 1}
    }};
    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, static_cast<int>(renderData.size()));

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glUseProgram(0);
    glEnable(GL_DEPTH_TEST);
}
