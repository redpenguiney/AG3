#include "pointlight_component.hpp"
#include "component_pool.hpp"
#include <memory>

PointLightComponent::PointLightComponent() {}
PointLightComponent::~PointLightComponent() {}

void PointLightComponent::Destroy() {

}

void PointLightComponent::Init() {
    lightColor = {1, 1, 1};
    lightRange = 100;
} 

void PointLightComponent::SetColor(glm::vec3 color) {
    lightColor = color;
}

glm::vec3 PointLightComponent::Color() {
    return lightColor;
}

void PointLightComponent::SetRange(float range) {
    lightRange = range;
}

float PointLightComponent::Range() {
    return lightRange;
}

