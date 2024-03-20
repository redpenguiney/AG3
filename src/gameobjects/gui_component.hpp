#pragma once
#include "base_component.hpp"
#include "transform_component.hpp"
// #include "../graphics/engine.hpp"
#include "../../external_headers/GLM/vec2.hpp"
#include <optional>

// Basically a wrapper for the transform and render components that handles stuff for you when doing gui.
// Doesn't need to be super fast.
// Everything this does, you could just do yourself with rendercomponents + transformcomponents if you want, all this does is call functions on/set values of render/transform components when UpdateGuiTransform()/UpdateGuiGraphics() is called.
class GuiComponent: public BaseComponent<GuiComponent> {
    public:

    float rotation;

    // The point (in object space) that offset + scale pos are setting the position of.
    glm::vec2 anchorPoint;

    // % of the screen on each axis. (0, 0) is the center of the screen, screen is in interval [-0.5, 0.5].
    glm::vec2 Pos;

    // Offset position in pixels
    glm::vec2 offsetPos;

    // % of the screen on each axis. (0, 0) is the center of the screen, screen is in interval [-0.5, 0.5].
    glm::vec2 scaleSize;

    // Offset size in pixels
    glm::vec2 offsetSize;

    enum {
        XX,
        XY,
        YY
    } guiScaleMode; // which screen dimensions the scale portion of position/scale uses for each axis.
    
    glm::vec4 rgba; // color and transparency

    struct GuiTextInfo {
        glm::vec4 textRgba;
        std::string textMessage;
        
    };
    std::optional<GuiTextInfo> guiTextInfo;

    // Call after modifying any position/rotation/scale related variables to actually apply those changes to the gui's transform.
    void UpdateGuiTransform();

    // Call after modifying any graphics related (not pos/rot/scl) variables to actually apply those changes to the gui's transform.
    void UpdateGuiGraphics();

    private:
    // pointers to the components corresponding to this GuiComponent.
    TransformComponent* transform;

};