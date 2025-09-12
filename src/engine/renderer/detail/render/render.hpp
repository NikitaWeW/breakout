#pragma once

#include "engine/renderer/renderer.hpp"
#include "engine/renderer/detail/ogl.hpp"
#include "engine/renderer/detail/mesh.hpp"
#include "glm/gtc/type_ptr.hpp"

namespace engine::renderer::detail
{
    struct RendererData
    {
        ogl::Framebuffer oitFBO;
        ogl::Texture oitAccumTexture;
        ogl::Texture oitRevelageTexture;

        ogl::Framebuffer mainFBO;
        ogl::Texture mainFBOColor;
        ogl::Renderbuffer mainFBORBO;

        ogl::Program plainColorShader;

        glm::uvec2 prevCamSize{0};
    }; 

    void renderMain(ecs::registry &reg, detail::RendererData &data, renderer::RendererContext &context);
} // namespace engine::renderer::detail
