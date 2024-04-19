#pragma once
#include "../gameobjects/component_registry.hpp"
#include "../gameobjects/transform_component.hpp"
#include "../graphics/engine.hpp"
#include "../../external_headers/GLM/vec2.hpp"
#include <memory>
#include <optional>
#include <utility>

// TODO: parenting, 3d 

// Basically a wrapper for the transform and render components that handles stuff for you when doing gui.
// Doesn't need to be super fast.
// Everything this does, you could just do yourself with rendercomponents + transformcomponents if you want, all this does is call functions on/set values of render/transform components when UpdateGuiTransform()/UpdateGuiGraphics() is called.
class Gui {
    public:
    // window.cpp calls this when the window resolution changes to handle stuff.
    static void UpdateGuiForNewWindowResolution();
    static void UpdateBillboardGuis();

    struct GuiTextInfo {

        glm::vec4 rgba;

        // TODO: I DONT THINK THIS ACTUALLY DOES ANYTHING YET
        // unsigned int textHeight;

        // multiplier; 1 is single spaced, 2 is MLA double spaced, etc.
        GLfloat lineHeight;

        // in pixels
        GLfloat leftMargin, rightMargin, topMargin, bottomMargin;

        HorizontalAlignMode horizontalAlignment;
        VerticalAlignMode verticalAlignment;

        std::string text;

        std::shared_ptr<GameObject> object;

        std::shared_ptr<Material> fontMaterial;

        float fontMaterialLayer; 
    };

    struct BillboardGuiInfo {
        // Makes the gui get smaller when it's far away if true.
        bool scaleWithDistance;

        // if nullopt, gui will always face camera and obey rotation.
        std::optional<glm::quat> rotation;

        // Billboard position will be projected onto followObject's position every frame if this is true. (so the gui will always appear on top of followObject).
        // If the gui's position isn't the origin, it is treated as an offset.
        std::weak_ptr<GameObject> followObject;

        // TODO: ACTUALLY KINDA COMPLICATED WHEN YOU HAVE MULTIPLE GUIS INVOLVED
        // if true, other gameobjects in front of this gui will cover it up
        // bool occludable;
    };
    
    std::shared_ptr<GameObject> object;

    Gui(bool haveText = false, std::optional<std::pair<float, std::shared_ptr<Material>>> fontMaterial = std::nullopt, std::optional<std::pair<float, std::shared_ptr<Material>>> guiMaterial = std::nullopt, std::optional<BillboardGuiInfo> billboardInfo = std::nullopt, std::shared_ptr<ShaderProgram> guiShader = GraphicsEngine::Get().defaultGuiShaderProgram);

    ~Gui();

    // don't copy or move ples
    Gui(const Gui&) = delete;
    Gui(const Gui&&) = delete;

    GuiTextInfo& GetTextInfo();
    BillboardGuiInfo& GetBillboardInfo();

    // around center of the gui
    float rotation;

    // lower number = rendered on top
    float zLevel;

    // The point (in object space) that offset + scale pos are setting the position of.
    glm::vec2 anchorPoint;

    // Offset position in pixels
    glm::vec2 offsetPos;

    // % of the screen on each axis. (0, 0) is the center of the screen, screen is in interval [-1, 1].
    glm::vec2 scalePos;

    // Offset size in pixels
    glm::vec2 offsetSize;

    // % of the screen on each axis. (0, 0) is the center of the screen, screen is in interval [-1, 1].
    glm::vec2 scaleSize;

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

    // get size of the gui in pixels
    glm::vec2 GetPixelSize();

    private:
    // If not nullopt, the gui has text
    std::optional<GuiTextInfo> guiTextInfo;

    // If not nullopt, will make the gui basically 3d
    std::optional<BillboardGuiInfo> billboardInfo;
    
};