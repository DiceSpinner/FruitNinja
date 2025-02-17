#ifndef CAMERA_H
#define CAMERA_H
#include <vector>
#include <glm/glm.hpp>
#include "../core/component.hpp"

class Camera : public Component {
private:
    float near;
    float far;
    glm::mat4 ortho{ 1 };
    glm::mat4 perspective{ 1 };
public:
    static Camera* main;
    static std::vector<Camera*>* cameras;

    bool isOrtho;

    Camera(std::unordered_map<std::type_index, std::vector<std::unique_ptr<Component>>>& components, Transform& transform, Object* object,  float near, float far);
    void SetPerspective(float near, float far);

    glm::mat4 Ortho() const;
    glm::mat4 Perspective() const;
    glm::mat4 Projection() const;
    glm::mat4 View() const;
    float ComputerNormalizedZ(float viewZ) const;
    void OnEnabled() override;
    void OnDisabled() override;
};

#endif