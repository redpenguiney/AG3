#pragma once
#include "base_component.hpp"
#include "transform_component.hpp"
// #include "../graphics/engine.hpp"
#include "../../external_headers/GLM/vec2.hpp"

// Basically a wrapper for the transform component that handles it for you when doing gui.
// Doesn't need to be super fast.
class GuiComponent: public BaseComponent<GuiComponent> {
    public:
    // Sets position, but for guis. SetPos works for this too, but this lets you use offset + scale + anchor point.
    // Offset is in pixels of the screen, scale is % of the screen, together they determine the position of the anchor point (which is in local space)
    // (0, 0) is the center of the screen, screen is in interval [-0.5, 0.5].
    void SetGuiPos(float scaleX, float scaleY, float offsetX, float offsetY, float z = 0, float anchorX = 0, float anchorY = 0, GuiScaleMode = XY);

    private:
    // pointers to the components corresponding to this GuiComponent.
    TransformComponent* transform;
    // GraphicsEngine::RenderComponent* render;

    glm::vec2 anchorPoint;
    glm::vec2 scale;
    glm::vec2 offset;
};