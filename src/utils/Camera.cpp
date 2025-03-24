#include "Camera.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <iostream>

Camera::Camera(glm::vec3 const &pos, glm::vec3 const &rotation, ProjectionType projectionType) noexcept : position(pos), rotation(rotation), fov(45), projectionType(projectionType) {}

Camera::~Camera() noexcept {}

void Camera::update(double deltatime) noexcept
{
    front = glm::normalize(glm::vec3(
        cos(glm::radians(rotation.x)) * cos(glm::radians(rotation.y)),
        sin(glm::radians(rotation.y)),
        sin(glm::radians(rotation.x)) * cos(glm::radians(rotation.y))
    ));
    right = glm::normalize(glm::cross(front, glm::vec3(0, 1, 0)));
    up    = glm::normalize(glm::cross(right, front));
}

glm::mat4 Camera::getViewMatrix() const noexcept
{
    return glm::lookAt(position, position + front, up);
}

glm::mat4 Camera::getProjectionMatrix() const noexcept
{
    auto params = std::make_tuple(width, height, fov, near, far, projectionType);
    if(params != lastparams) {
        if(projectionType == PERSPECTIVE) {
            projectionMat = glm::mat4{glm::perspective(glm::radians(fov), (float) width / height, near, far)};
        } else if(projectionType == ORTHO) {
            float aspect = (float) width / height;
            projectionMat = glm::ortho<float>(-aspect, aspect, -1, 1, near, far);
        } else {
            std::cout << "unknown projection type\n";
        }
        lastparams = params;
    }
    return projectionMat;
}
