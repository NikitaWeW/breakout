#include <iostream>

#include "Text.hpp"
#include "ttf2mesh.h"

void text::Font::drawText(std::string const &text, glm::vec2 const &position, float size, glm::vec4 const &color, glm::mat4 const &projectionMatrix)
{
    struct GlyphRenderData {
        glm::vec2 quadPosition;
        glm::vec2 quadSize;
        glm::vec2 glyphOffset;
        glm::vec2 glyphSize;
    };
    std::vector<GlyphRenderData> renderData;
    {
        glm::vec2 currentPosition = {position.x, position.y};
        for(auto it = text.begin(); it != text.end(); ++it) { // fuck that shit
            if(*it == L' ') {
                currentPosition.x += size * spaceSize;
                continue;
            } else if(*it == L'\n') {
                currentPosition.x = position.x;
                currentPosition.y -= newLineSize * size;
                continue;
            }
            auto dataIter = atlas.glyphs.find(*it);
            if(dataIter == atlas.glyphs.end()) {
                std::cout << "failed to find glyph for char \'" << (wchar_t) *it << "\'!\n";
            }
            GlyphData const &data = dataIter->second;

            GlyphRenderData currentRenderData{};
            currentRenderData.quadPosition = { currentPosition.x, currentPosition.y + data.verticalOffset * size };
            currentRenderData.quadSize = { data.size.x * size, data.size.y * size };
            currentRenderData.glyphOffset = data.offset;
            currentRenderData.glyphSize = data.size;
            
            renderData.push_back(currentRenderData);
    
            currentPosition.x += data.size.x * size + spacing;
        }
    }

    textShader.bind();
    glUniform4fv(textShader.getUniform("u_color"), 1, &color.r);
    glUniform1i (textShader.getUniform("u_atlas"), 0);
    glUniformMatrix4fv(textShader.getUniform("u_projMat"), 1, GL_FALSE, &projectionMatrix[0][0]);
    atlas.texture.bind(0);
    opengl::VertexBuffer renderDataBuffer{renderData.size() * sizeof(GlyphRenderData), renderData.data()};
    opengl::VertexArray quadVAO{renderDataBuffer, opengl::InterleavedInstancingVertexBufferLayout{ 
        {2, GL_FLOAT, 1},
        {2, GL_FLOAT, 1},
        {2, GL_FLOAT, 1},
        {2, GL_FLOAT, 1}
    }};
    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, renderData.size());

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glUseProgram(0);
}

text::Font::Font(std::filesystem::path const &filepath, std::vector<wchar_t> const &chars, int atlasSize)
{
    // TODO: export and cache atlas using stb image write
    // TODO: nuke this shit and use msdfgen or stb_truetype

    textShader = opengl::ShaderProgram{"shaders/drawText", true};
    atlasShader = opengl::ShaderProgram{"shaders/fontAtlas", true};
    blurShader = opengl::ShaderProgram{"shaders/blur", true};
    
    // ========================================================

    atlas.size = atlasSize;
    atlas.texture = opengl::TextureMS{GL_LINEAR};
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RED, atlasSize, atlasSize, GL_TRUE);

    unsigned numRows = glm::ceil(glm::sqrt(chars.size()));
    unsigned numCols = glm::ceil((float)chars.size() / numRows);
    unsigned cellWidth = glm::ceil((float) atlasSize / numRows);
    unsigned cellHeight = glm::ceil((float) atlasSize / numCols);

    glm::vec2 currentPos = {0, 0};
    
    opengl::Framebuffer atlasFBO;
    atlasFBO.bind();
    atlasFBO.attach(atlas.texture, GL_COLOR_ATTACHMENT0);
    assert(atlasFBO.isComplete());

    ttf_t *font;
    ttf_load_from_file(filepath.string().c_str(), &font, false);
    assert(font);
    unsigned largestHeightInCurrentRow = 0;
    for(uint16_t currentChar : chars) {
        // get the glyph mesh
        int index = ttf_find_glyph(font, currentChar);
        if(index < 0) {
            std::cout << "WARNING: failed to find \'" << currentChar << "\' glyph!\n";
            continue;
        }
        ttf_glyph_t *glyph = &font->glyphs[index];
        ttf_mesh_t *mesh;
        if(ttf_glyph2mesh(glyph, &mesh, TTF_QUALITY_NORMAL, TTF_FEATURES_DFLT) != TTF_DONE) {
            std::cout << "WARNING: failed to convert glyph \'" << (char) currentChar << "\' to mesh!\n";
            continue;
        }

        // draw the mesh to the atlas. that's the stupidest aproach ever.
        glBindVertexArray(0);
        atlasFBO.bind();
        atlasShader.bind();

        // setup cell dimensions
        float glyphWidth = glyph->xbounds[1] - glyph->xbounds[0];
        float glyphHeight = glyph->ybounds[1] - glyph->ybounds[0];
        unsigned glyphCellWidth = glm::floor(glm::min<float>(cellWidth, glyphWidth * cellWidth));
        unsigned glyphCellHeight = glm::floor(glm::min<float>(cellHeight, glyphHeight * cellWidth));
        largestHeightInCurrentRow = glm::max(largestHeightInCurrentRow, glyphCellHeight);
        glViewport(currentPos.x, currentPos.y, glyphCellWidth, glyphCellHeight); 

        // draw the (stupid) mesh 
        opengl::VertexBuffer meshVBO{mesh->nvert * sizeof(mesh->vert[0]), mesh->vert};
        opengl::IndexBuffer meshIBO{mesh->nfaces * sizeof(mesh->faces[0]), mesh->faces};
        glVertexAttribPointer(0, 2, GL_FLOAT, false, 2 * sizeof(float), reinterpret_cast<void const *>(0));
        glEnableVertexAttribArray(0);
        glUniform2f(atlasShader.getUniform("u_glyphMin"), glyph->xbounds[0], glyph->ybounds[0]);
        glUniform2f(atlasShader.getUniform("u_glyphMax"), glyph->xbounds[1], glyph->ybounds[1]);
        glDrawElements(GL_TRIANGLES, mesh->nfaces * 3, GL_UNSIGNED_INT, nullptr);

        // make an entry in the glyph lookup map
        atlas.glyphs[currentChar] = {
            currentPos / (float) atlasSize,
            {
                (float) glyphCellWidth / atlasSize, 
                (float) largestHeightInCurrentRow / atlasSize
            },
            glyph->ybounds[0] / 10
        };

        // process current position
        currentPos.x += glyphCellWidth + 1;
        if(currentPos.x + cellWidth > atlasSize) {
            currentPos.x = 0;
            currentPos.y += largestHeightInCurrentRow + 1;
            assert(currentPos.y + largestHeightInCurrentRow < atlasSize);
        }

        ttf_free_mesh(mesh);
    }
    ttf_free(font);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
