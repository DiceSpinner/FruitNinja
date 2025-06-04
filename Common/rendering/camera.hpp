#ifndef CAMERA_H
#define CAMERA_H
#include <vector>
#include "render_context.hpp"
#include "infrastructure/object.hpp"

class Camera : public Component {
private:
    float nearClipPlane;
    float farClipPlane;
    glm::mat4 ortho{ 1 };
    glm::mat4 perspective{ 1 };
    float width;
public:
    static Camera* main;
    static std::vector<Camera*>* cameras;

    bool isOrtho;

    Camera(
        std::unordered_map<std::type_index, std::vector<std::unique_ptr<Component>>>& collection, Transform& transform, Object* object, float nearClipPlane, float farClipPlane
    );

    void SetPerspective(float nearClipPlane, float farClipPlane);
    void SetOrthoWidth(float width);
	float Width() const;  
    glm::mat4 Ortho() const;
    glm::mat4 Perspective() const;
    glm::mat4 Projection() const;
    glm::mat4 View() const;
    float ComputerNormalizedZ(float viewZ) const;
    void OnEnabled() override;
    void OnDisabled() override;
};

#endif