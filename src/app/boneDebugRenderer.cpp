/*
#include "boneDebugRenderer.hpp"
#include "glm/gtc/type_ptr.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/matrix_decompose.hpp"

using namespace engine;
namespace ogl = renderer::ogl;

void BoneDebugRenderer::renderBones(ecs::registry &reg, renderer::RendererData const &data, ecs::entity const &e_instance)
{
    renderer::Model const &model = reg.get<renderer::Model>(reg.get<renderer::ProcessedModel>(reg.get<Instance>(e_instance).e_model).data);
    if(model.skeleton.bindTransform.empty())
        return;
    auto const &current = reg.get<CurrentAnimation>(e_instance);

    ENGINE_ASSERT_MSG(reg.view<BoneModel>().size() == 1, "forgot to call BoneDebugRenderer::setup?");
    ecs::entity boneModelEntity = reg.view<BoneModel>().at(0);
    reg.emplace<Instance>(boneModelEntity, boneModelEntity);

    constexpr glm::vec3 boneScaleFactor(0.2f);

    glm::mat4 instanceModel = reg.get<ModelMatrix>(e_instance);
    glm::vec3 instance_scale, instance_translation, instance_skew;
    glm::vec4 instance_perspective;
    glm::quat instance_rotation;
    glm::decompose(instanceModel, instance_scale, instance_rotation, instance_translation, instance_skew, instance_perspective);

    for (size_t i = 0; i < current.boneMatrices.size(); ++i) {
        glm::mat4 boneGlobal = current.boneMatrices[i];
        glm::mat4 bindInv    = glm::inverse(model.skeleton.bindTransform[i]);
        // boneGlobal = glm::mat4(1.0f);
        // bindInv    = glm::mat4(1.0f);

        glm::mat4 boneTransform = boneGlobal * bindInv;

        glm::vec3 scale, translation, skew;
        glm::vec4 perspective;
        glm::quat rotation;
        glm::decompose(boneTransform, scale, rotation, translation, skew, perspective);

        glm::mat4 finalTransform = 
            glm::translate(glm::mat4(1.0f), instance_translation) * 
            glm::mat4_cast(instance_rotation) *
            glm::translate(glm::mat4(1.0f), translation * instance_scale) * 
            glm::mat4_cast(rotation) *
            glm::scale(glm::mat4(1.0f), boneScaleFactor)
        ;

        static_cast<glm::mat4 &>(reg.get<ModelMatrix>(boneModelEntity)) = finalTransform;
        renderMainInstance(reg, data, boneModelEntity);
    }
    reg.remove<Instance>(boneModelEntity);
}

void BoneDebugRenderer::renderMain(ecs::registry &reg, renderer::RendererData &data)
{
    auto const &camera = reg.get<Camera>(m_context.e_camera);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, camera.size.x, camera.size.y);
    glClearColor(0.1, 0.1, 0.1, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glUseProgram(data.plainColorShader.id);
    glm::mat4 viewMat = reg.has<engine::ModelMatrix>(m_context.e_camera) ? reg.get<engine::ModelMatrix>(m_context.e_camera) : glm::mat4{1.0f};
    glUniformMatrix4fv(ogl::getUniform(data.plainColorShader, "u_viewMat"), 1, false, glm::value_ptr(viewMat));
    glUniformMatrix4fv(ogl::getUniform(data.plainColorShader, "u_projMat"), 1, false, glm::value_ptr(camera.projMat));

    glEnable(GL_CULL_FACE);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glCullFace(GL_BACK);

    for(ecs::entity e_instance : reg.view<Instance>())
    {
        glEnable(GL_DEPTH_TEST);
        renderMainInstance(reg, data, e_instance);
        glDisable(GL_DEPTH_TEST);
        renderBones(reg, data, e_instance);
    }
}

BoneDebugRenderer::BoneDebugRenderer(Context const &context, Model const &boneModel) : EngineRenderer(context), m_boneModel(boneModel) {}

void BoneDebugRenderer::setup(ecs::registry &reg)
{
    if(reg.view<BoneModel>().empty())
    {
        auto e = reg.create<>();
        reg.emplace<BoneModel>(e);
        reg.emplace<Model>(e, m_boneModel);
        reg.emplace<ModelMatrix>(e);
    }

    EngineRenderer::setup(reg);
}
*/
