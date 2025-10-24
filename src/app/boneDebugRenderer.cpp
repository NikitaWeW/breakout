#include "boneDebugRenderer.hpp"
#include "glm/gtc/type_ptr.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/matrix_decompose.hpp"

using namespace engine;

static glm::vec3 extractPositionFromMat4(glm::mat4 const &M) {
    return glm::vec3(M[3]);
};
static glm::vec3 extractScaleFromMat4(glm::mat4 const &M) {
    glm::vec3 sx = glm::vec3(M[0]);
    glm::vec3 sy = glm::vec3(M[1]);
    glm::vec3 sz = glm::vec3(M[2]);
    return glm::vec3(glm::length(sx), glm::length(sy), glm::length(sz));
};

void BoneDebugRenderer::renderBones(ecs::registry &reg, engine::detail::RendererData const &data, ecs::entity const &e_instance)
{
    detail::Model const &model = reg.get<detail::Model>(reg.get<detail::ProcessedModel>(reg.get<engine::Instance>(e_instance).e_model).data);
    if(model.skeleton.bindTransform.empty())
        return;
    auto const &current = reg.get<CurrentAnimation>(e_instance);

    ENGINE_ASSERT_MSG(reg.view<BoneModel>().size() == 1, "forgot to call BoneDebugRenderer::setup?");
    ecs::entity boneModelEntity = reg.view<BoneModel>().at(0);

    constexpr glm::vec3 boneScaleFactor(0.2f);

    glm::mat4 instanceModel = reg.get<engine::ModelMatrix>(e_instance).value;
    glm::vec3 instance_scale, instance_translation, instance_skew;
    glm::vec4 instance_perspective;
    glm::quat instance_rotation;
    glm::decompose(instanceModel, instance_scale, instance_rotation, instance_translation, instance_skew, instance_perspective);

    for (size_t i = 0; i < current.boneMatrices.size(); ++i) {
        glm::mat4 boneGlobal = current.boneMatrices[i];
        glm::mat4 bindInv    = glm::inverse(model.skeleton.bindTransform[i]);

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

        reg.get<engine::ModelMatrix>(boneModelEntity).value = finalTransform;
        renderMainInstance(reg, data, boneModelEntity);
    }
}

void BoneDebugRenderer::renderMain(ecs::registry &reg, engine::detail::RendererData &data)
{
    auto const &camera = reg.get<engine::Camera>(m_context.e_camera);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, camera.size.x, camera.size.y);
    glClearColor(0.1, 0.1, 0.1, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glUseProgram(data.plainColorShader.id);
    glUniformMatrix4fv(ogl::getUniform(data.plainColorShader, "u_viewMat"), 1, false, glm::value_ptr(camera.viewMat));
    glUniformMatrix4fv(ogl::getUniform(data.plainColorShader, "u_projMat"), 1, false, glm::value_ptr(camera.projMat));

    glEnable(GL_CULL_FACE);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glCullFace(GL_BACK);

    for(ecs::entity e_instance : reg.view<engine::Instance>())
    {
        glEnable(GL_DEPTH_TEST);
        renderMainInstance(reg, data, e_instance);
        glDisable(GL_DEPTH_TEST);
        renderBones(reg, data, e_instance);
    }
}

BoneDebugRenderer::BoneDebugRenderer(Context const &context, engine::Model const &boneModel) : engine::EngineRenderer(context), m_boneModel(boneModel) {}

void BoneDebugRenderer::setup(ecs::registry &reg)
{
    auto e = reg.create<>();
    reg.emplace<engine::Instance>(e, e);
    reg.emplace<BoneModel>(e);
    reg.emplace<engine::Model>(e, m_boneModel);
    reg.emplace<engine::ModelMatrix>(e);

    EngineRenderer::setup(reg);
}
