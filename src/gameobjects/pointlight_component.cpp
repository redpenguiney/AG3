#pragma once
#include "pointlight_component.hpp"
#include <memory>

PointLightComponent::PointLightComponent(TransformComponent const* transformComponent): transform(transformComponent) {
    POINT_LIGHT_COMPONENTS.insert(this);
}

PointLightComponent::~PointLightComponent() {
    POINT_LIGHT_COMPONENTS.erase(this);
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