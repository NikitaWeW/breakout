#pragma once
#include "glm/glm.hpp"
#include <map>
#include <tuple>

class Camera 
{
public:
    enum ProjectionType { ORTHO, PERSPECTIVE, UNKNOWN };
private:
    glm::vec3 right = glm::vec3{1, 0, 0};
    glm::vec3 up = glm::vec3{0, 1, 0};
    glm::vec3 front = glm::vec3{0, 0, -1};
    mutable glm::mat4 projectionMat = glm::mat4{1.0f};
    mutable std::tuple<int, int, float, float, float, ProjectionType> lastparams = {-1, -1, -1, -1, -1, UNKNOWN};
public:
    ProjectionType projectionType;
    glm::vec3 position = glm::vec3{0, 0, 0};
    glm::vec3 rotation = glm::vec3{0, 0, 0};
    float far = 100;
    float near = 0.01f;
    float fov = 45;
    int width = 0; 
    int height = 0;
public:
    Camera(glm::vec3 const &pos = glm::vec3(0), glm::vec3 const &rotation = glm::vec3(0), ProjectionType projectionType = PERSPECTIVE) noexcept;
    virtual ~Camera() noexcept;
    virtual void update(double deltatime) noexcept;
    virtual glm::mat4 getViewMatrix() const noexcept;
    virtual glm::mat4 getProjectionMatrix() const noexcept;
    virtual inline glm::vec3 getFront() const noexcept { return front; }
    virtual inline glm::vec3 getRight() const noexcept { return right; }
    virtual inline glm::vec3 getUp() const noexcept { return up; }
};