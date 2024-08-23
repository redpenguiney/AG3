#pragma once
#include "base_component.hpp"
#include <glm/vec3.hpp>

// Spotlight direction is determined by object's transform's forward vector.
class SpotLightComponent : public BaseComponent {
public:
    SpotLightComponent();
    ~SpotLightComponent();
    //void Destroy();
    //void Init();

    // get distance from light at which it isn't visible
    float Range();

    // set distance from light at which it isn't visible
    void SetRange(float range);

    void SetColor(glm::vec3 color);
    glm::vec3 Color();

    void SetInnerAngle(float); // in degrees
    void SetOuterAngle(float); // in degrees
    float InnerAngle(); // in degrees
    float OuterAngle(); // in degrees

private:
    float lightRange;
    glm::vec3 lightColor;
    // in degrees
    float innerAngle;
    // in degrees
    float outerAngle;
};