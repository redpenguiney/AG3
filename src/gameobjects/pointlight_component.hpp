#pragma once
#include "transform_component.cpp"
#include <memory>
#include <unordered_set>

class GraphicsEngine;

class PointLightComponent {
    public:
    PointLightComponent(TransformComponent const* transformComponent);
    ~PointLightComponent();

    // get distance from light at which it isn't visible
    float Range();

    // set distance from light at which it isn't visible
    void SetRange(float range);

    void SetColor(glm::vec3 color);
    glm::vec3 Color();

    // indirection here is ok because if you have enough lights for that to lag you have bigger problems.
    // don't change this pointer please
    TransformComponent const* transform;

    private:
    float lightRange;
    glm::vec3 lightColor;

    // GE needs to iterate through LIGHT_COMPONENTS
    friend class GraphicsEngine;    

    // component pool is really overkill for lights but we have it for all the other components so why not
    inline static std::unordered_set<PointLightComponent*> POINT_LIGHT_COMPONENTS;
};
