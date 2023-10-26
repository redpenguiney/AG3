#pragma once
#include "transform_component.cpp"
#include <memory>
#include <unordered_set>

class GraphicsEngine;

class PointLightComponent: BaseComponent<PointLightComponent> {
    public:
    PointLightComponent(TransformComponent const* transformComponent);
    ~PointLightComponent();

    // get distance from light at which it isn't visible
    float Range();

    // set distance from light at which it isn't visible
    void SetRange(float range);

    void SetColor(glm::vec3 color);
    glm::vec3 Color();

    private:
    float lightRange;
    glm::vec3 lightColor;  
};
