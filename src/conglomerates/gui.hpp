#pragma once
#include "../gameobjects/component_registry.hpp"
#include "../gameobjects/transform_component.hpp"
#include "../graphics/engine.hpp"
#include "../../external_headers/GLM/vec2.hpp"
#include <memory>
#include <optional>
#include <utility>

// Basically a wrapper for the transform and render components that handles stuff for you when doing gui.
// Doesn't need to be super fast.
// Everything this does, you could just do yourself with rendercomponents + transformcomponents if you want, all this does is call functions on/set values of render/transform components when UpdateGuiTransform()/UpdateGuiGraphics() is called.
class Gui {
    public:
    struct GuiTextInfo {
        glm::vec4 rgba;
        unsigned int textHeight;
        std::string text;
        std::shared_ptr<GameObject> object;
        std::shared_ptr<Material> fontMaterial;
        float fontMaterialLayer; 
    };

    std::shared_ptr<GameObject> object;

    Gui(bool haveText = false, std::optional<std::pair<float, std::shared_ptr<Material>>> fontMaterial = std::nullopt, std::optional<std::pair<float, std::shared_ptr<Material>>> guiMaterial = std::nullopt, std::shared_ptr<ShaderProgram> guiShader = GraphicsEngine::Get().defaultGuiShaderProgram);

    GuiTextInfo& getTextInfo();

    // around center of the gui
    float rotation;

    // The point (in object space) that offset + scale pos are setting the position of.
    glm::vec2 anchorPoint;

    // Offset position in pixels
    glm::vec2 offsetPos;


    // % of the screen on each axis. (0, 0) is the center of the screen, screen is in interval [-0.5, 0.5].
    glm::vec2 scalePos;

    // % of the screen on each axis. (0, 0) is the center of the screen, screen is in interval [-0.5, 0.5].
    glm::vec2 scaleSize;

    // Offset size in pixels
    glm::vec2 offsetSize;

    enum {
        ScaleXX,
        ScaleXY,
        ScaleYY
    } guiScaleMode; // which screen dimensions the scale portion of position/scale uses for each axis.
    
    glm::vec4 rgba; // color and transparency
    std::optional<std::shared_ptr<Material>> material;
    std::optional<unsigned int> materialLayer; 

    // Call after modifying any position/rotation/scale related variables to actually apply those changes to the gui's transform.
    void UpdateGuiTransform();

    // Call after changing font, text, or text-formatting-related stuff
    void UpdateGuiText(); 

    // Call after modifying any graphics related (not text-related and not pos/rot/scl) variables to actually apply those changes to the gui's transform.
    void UpdateGuiGraphics();

    private:
    std::optional<GuiTextInfo> guiTextInfo;
    
};