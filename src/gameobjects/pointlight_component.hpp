#pragma once
#include <memory>
#include <unordered_set>
#include "base_component.hpp"
#include "../../external_headers/GLM//vec3.hpp"

class GraphicsEngine;

class PointLightComponent: public BaseComponent<PointLightComponent> {
    public:
    PointLightComponent();
    ~PointLightComponent();
    void Destroy();
    void Init();

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
