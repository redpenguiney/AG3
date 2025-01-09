#pragma once
#include "gameobjects/gameobject.hpp"
#include "gameobjects/transform_component.hpp"
#include "graphics/gengine.hpp"
#include "glm/vec2.hpp"
#include <memory>
#include <optional>
#include <utility>

class Gui;

// These were originally private static variables, but that didn't work well with the introduction of modules/dlls so now it's here (TODO JK DELETE).
// Only for internal use by gui.cpp. The public Get() method is for transferring these globals to modules.
class GuiGlobals {
    public:
    static GuiGlobals& Get();

    // When modules (shared libraries) get their copy of this code, they need to use a special version of GuiGlobals::Get().
    // This is so that both the module and the main executable have access to the same globals. 
    // The executable will provide each shared_library with a pointer to these gui globals.
    #ifdef IS_MODULE
    static void SetModuleGuiGlobals(GuiGlobals* globals);
    #endif

    private:
    
    std::vector<Gui*> listOfGuis;
    std::vector<Gui*> listOfBillboardGuis;
    friend class Gui;

    GuiGlobals();

};

enum class GuiChildBehaviour {
    Relative, // positions and scaling are done in the space of the parent
    
    // positions are overriden according to a grid determined by the parent; scaling is done in the grandparent's space.
    // SortChildren() must be called at least once to apply grid.
    // Offset pos is still applied.
    Grid 
};

// Basically a wrapper for the transform and render components that handles stuff for you when doing gui.
// Doesn't need to be super fast.
// Everything this does, you could just do yourself with rendercomponents + transformcomponents if you want, all this does is call functions on/set values of render/transform components when UpdateGuiTransform()/UpdateGuiGraphics() is called.
class Gui: public std::enable_shared_from_this<Gui> {
public:
    

    // called in main.cpp, connects some events
    static void Init();

    struct GuiTextInfo {

        glm::vec4 rgba;

        // TODO: I DONT THINK THIS ACTUALLY DOES ANYTHING YET
        // unsigned int textHeight;

        // multiplier; 1 is single spaced, 2 is MLA double spaced, etc.
        GLfloat lineHeight;

        // margin is offset in pixels in screen space from sides of gui. 10 for left and right margins would shift text by 10 pixels to right
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

    struct GridGuiInfo {
        glm::vec2 gridScalePosition; // relative position of first child in terms of object
        glm::vec2 gridOffsetPosition; // relative position of first child in pixels
        glm::vec2 gridScaleSize; // spacing between anchor points of each child in terms of object's parent (child's grandparent)
        glm::vec2 gridOffsetSize; // spacing between anchor points of each child in pixels

        // Determines when to wrap grid around based on maximum amount of objects in the grid on the fill axis. 
        // If maxInPixels == true, it will compare total size of objects in pixels along fill axis to this value.
        int maxInFillDirection = 16384;
        bool maxInPixels = false;
        bool fillXFirst = true; // if true, grid is filled in rows; if false, grid is filled in columns
        bool addGuiLengths = false; // makes the spacing between guis consider the size of each gui if true
    };

    // only used when childBehaviour == Grid; call UpdateGuiTransform() when modified to affect children.
    // Set this for that PARENT of the objects you want aligned to a grid.
    GridGuiInfo gridInfo;
    
    std::shared_ptr<GameObject> object;

    // if haveText == true, then fontMaterial must be a material with a fontmap attached.
    // SHOULD be used with std::make_shared/shared_ptr, but if you don't its fine as long as you don't use SetParent().
    // guiMaterial will default to the one in GraphicsEngine if nullopt.
    Gui(bool haveText, std::optional<std::pair<float, std::shared_ptr<Material>>> guiMaterial, std::optional<std::pair<float, std::shared_ptr<Material>>> fontMaterial = std::nullopt, std::optional<BillboardGuiInfo> billboardInfo = std::nullopt);

    ~Gui();

    // don't copy or move ples
    Gui(const Gui&) = delete;
    Gui(const Gui&&) = delete;

    GuiTextInfo& GetTextInfo();
    BillboardGuiInfo& GetBillboardInfo();

    // determines how its children inherit this gui's position
    GuiChildBehaviour childBehaviour = GuiChildBehaviour::Relative;

    // around center of the gui
    float rotation = 0;

    // lower number = rendered on top
    float zLevel = 0;

    // used for scrolling. If not nullptr, only the part of gui whose area overlaps clipTarget will be rendered. 
    // of course, it's still up to you to actually move the gui up and down depending on scroll wheel or whatever.
    // will NOT work if clipTarget is not an ancestor of the gui.
    std::optional<std::weak_ptr<Gui>> clipTarget;


    // The point (in object space) that offset + scale pos are setting the position of.
    glm::vec2 anchorPoint;

    // Offset position in pixels
    glm::vec2 offsetPos;

    // % of the screen (or parent, depending on its parent's GuiChildBehaviour) on each axis. (0.5, 0.5) is the center of the screen, screen is in interval [0, 1].
    glm::vec2 scalePos;

    // Offset size in pixels
    glm::vec2 offsetSize;

    // % of the screen (or parent, depending on its parent's GuiChildBehaviour) on each axis. (1, 1) would cover the whole screen if centered.
    glm::vec2 scaleSize;

    enum {
        ScaleXX,
        ScaleXY,
        ScaleYY
    } guiScaleMode; // which screen dimensions the scale portion of position/scale uses for each axis.
    
    glm::vec4 rgba; // color and opacity/alpha
    std::shared_ptr<Material> material;
    unsigned int materialLayer; 

    // Call after modifying any position/rotation/scale related variables (including changes to child behaviour and grid size) to actually apply those changes to the gui's transform (and that of its children).
    void UpdateGuiTransform();

    // Call after changing font, text, or text-formatting-related stuff
    void UpdateGuiText(); 

    // Call after modifying any graphics related (not text-related and not pos/rot/scl) variables to actually apply those changes to the gui's appearance.
    void UpdateGuiGraphics();

    // get size of the gui in pixels
    glm::vec2 GetPixelSize();

    // get position of the gui's center (NOT OF THE ANCHOR POINT) in pixels
    glm::vec2 GetPixelPos();

    // Won't fire if the gui entered and exited in the same frame.
    std::shared_ptr<Event<>> onMouseEnter;
    std::shared_ptr<Event<>> onMouseExit;

    // Like window input events, but only fire when the mouse is over the gui.
    std::shared_ptr<Event<InputObject>> onInputBegin;
    std::shared_ptr<Event<InputObject>> onInputEnd;

    // returns mouseHover; true if mouse is over the gui.
    bool IsMouseOver();

    const std::vector<std::shared_ptr<Gui>>& GetChildren();

    // (stably) sorts children vector by their sortValue and sets each child's gridPos based on the grid layout. Follow with UpdateGuiTransform().
    // The results are undefined if gridInfo.maxFillInDirection is too restrictive, but it will not crash.
    void SortChildren();

    // lower values show up first in grid.
    int sortValue = 0;

    // sets the gui's parent to newParent. May be nullptr.
    // NOT SUPPORTED if the gui is not stored in a shared_ptr.
    void SetParent(Gui* newParent);

    // will return nullptr if no parent
    Gui* GetParent();

    // sets current parent to nullptr.
    void Orphan();

private:
    // returns parent size, grandparent size, or window size in pixels depending on gui ancestry and GuiChildBehaviour of its parent
    glm::vec2 GetParentContainerScale();

    glm::vec2 gridPos = { 0, 0 };

    // nonowning pointer, may be null
    Gui* parent = nullptr;

    // children are orphaned but not destroyed on parent destruction unless this is the only reference to those children 
    std::vector<std::shared_ptr<Gui>> children;

    // true if mouse was on this gui last frame.
    bool mouseHover;

    static void UpdateGuiForNewWindowResolution(glm::uvec2 oldSize, glm::uvec2 newSize);
    static void FireInputEvents();
    static void UpdateBillboardGuis(float);

    // If not nullopt, the gui has text
    std::optional<GuiTextInfo> guiTextInfo;

    // If not nullopt, will make the gui basically 3d
    std::optional<BillboardGuiInfo> billboardInfo;
};