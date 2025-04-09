#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "Renderer.hpp"
#include "game/Physics.hpp"

void game::Renderer::update(double deltatime)
{
    for(auto &cameraEntity : m_entities) {
        if(!ecs::entityHasComponent<Camera>(cameraEntity)) continue;
        Camera &camera = ecs::get<Camera>(cameraEntity);
        glfwGetWindowSize(window, &camera.width, &camera.height);
        
        // float aspect = (float) camera.width / camera.height;
        // camera.projMat = glm::ortho<float>(-aspect, aspect, -1, 1, near, far);
        camera.projMat = glm::perspective<float>(glm::radians(camera.fov), (float) camera.width / camera.height, camera.znear, camera.zfar);

        camera.viewMat = glm::mat4{1.0f};
        if(ecs::entityHasComponent<Position>(cameraEntity)) {
            camera.viewMat *= glm::translate(camera.viewMat, -ecs::get<Position>(cameraEntity).position);
        }
        if(ecs::entityHasComponent<RotationQuaternion>(cameraEntity)) {
            camera.viewMat *= glm::mat4_cast(ecs::get<RotationQuaternion>(cameraEntity).quat);
        }
        
        glViewport(0, 0, camera.width, camera.height);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        for(auto &entity : m_entities) { // TODO: batch it or something?
            if(!ecs::entityHasComponent<Drawable>(entity)) continue;
            Drawable &drawable = ecs::get<Drawable>(entity);
            drawable.shader->bind();
    
            glm::mat4 modelMat{1.0f};

            if(ecs::entityHasComponent<ModelMatrix>(entity))
            {
                modelMat = ecs::get<ModelMatrix>(entity).modelMatrix;
            }
            if(ecs::entityHasComponent<Position>(entity)) {
                modelMat = glm::translate(modelMat, ecs::get<Position>(entity).position);
            }
            if(ecs::entityHasComponent<Rotation>(entity)) {
                modelMat = glm::rotate<float>(modelMat, 1.0f, ecs::get<Rotation>(entity).rotation);
            }
            if(ecs::entityHasComponent<Scale>(entity)) {
                modelMat = glm::scale(modelMat, ecs::get<Scale>(entity).scale);
            }
            if(ecs::entityHasComponent<opengl::Texture>(entity)) {
                ecs::get<opengl::Texture>(entity).bind(0);
            } else {
                m_notfound.bind(0);
            }
            if(ecs::entityHasComponent<Color>(entity)) {
                glUniform4fv(drawable.shader->getUniform("u_color"), 1, &ecs::get<Color>(entity).color.r);
            } else {
                glUniform4f(drawable.shader->getUniform("u_color"), 1, 1, 1, 1);
            }
            
            drawable.va.bind();
            glUniform1i(drawable.shader->getUniform("u_texture"), 0);
            glUniformMatrix4fv(drawable.shader->getUniform("u_modelMat"), 1, GL_FALSE, &modelMat[0][0]);
            glUniformMatrix4fv(drawable.shader->getUniform("u_viewMat"), 1, GL_FALSE, &camera.viewMat[0][0]);
            glUniformMatrix4fv(drawable.shader->getUniform("u_projectionMat"), 1, GL_FALSE, &camera.projMat[0][0]);
            if(drawable.ib.has_value()) {
                glDrawElements(drawable.mode, drawable.count, GL_UNSIGNED_INT, nullptr);
            } else {
                glDrawArrays(drawable.mode, 0, drawable.count);
            }
        }
    }
}