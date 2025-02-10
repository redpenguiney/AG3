#include "gui.hpp"
#include "debug/assert.hpp"
#include <vector>
#include "glm/gtx/string_cast.hpp"
#include "graphics/gengine.hpp"
#include "graphics/mesh.hpp"
#include <algorithm>

#ifdef IS_MODULE
GuiGlobals* _GUI_GLOBALS_ = nullptr;
void GuiGlobals::SetModuleGuiGlobals(GuiGlobals* globals) {
    _GUI_GLOBALS_ = globals;
}
#endif

GuiGlobals& GuiGlobals::Get() {
    #ifdef IS_MODULE
    Assert(_GUI_GLOBALS_ != nullptr);
    return *_GUI_GLOBALS_;
    #else
    static GuiGlobals globals;
    return globals;
    #endif
}

GuiGlobals::GuiGlobals() {}

void Gui::UpdateGuiForNewWindowResolution(glm::uvec2 oldSize, glm::uvec2 newSize) {
    for (auto & ui: GuiGlobals::Get().listOfGuis) {
        if (ui->guiTextInfo.has_value()) {
            ui->UpdateGuiText();
        }
        ui->UpdateGuiTransform();
    }
}

// TODO: O(nGuis) complexity, not huge deal but spatial partioning structure (shudder) might be wise eventually
#pragma warning(disable : 4244)
void Gui::FireInputEvents()
{

    //DebugLogInfo("COUNT ", GuiGlobals::Get().listOfGuis.size());
    for (auto& ui : GuiGlobals::Get().listOfGuis) {

        // if no one cares whether the mouse is on this gui, don't bother calculating it.
        if (!ui->onMouseEnter->HasConnections() && !ui->onMouseExit->HasConnections() && !ui->onInputBegin->HasConnections() && !ui->onInputEnd->HasConnections()) {
            continue;
        }


        // determine if mouse is over the gui.

        // so that we can treat the gui's rotation as 0 and do a simple AABB-point intersection test, we rotate the cursor position by the inverse of the gui's current rotation.
        // Note that we invert mouse position; window treats (0, 0) as top left but gui positioning treats it as bottom left.
        glm::vec2 cursorPos = (glm::vec2(GraphicsEngine::Get().GetWindow().MOUSE_POS));
        cursorPos.y = GraphicsEngine::Get().GetWindow().height - cursorPos.y;
        glm::vec2 relCursorPos = ui->GetPixelPos() - cursorPos;
        relCursorPos = glm::vec2(glm::quat(ui->rotation, glm::vec3(0, 0, 1)) * glm::vec3(relCursorPos, 0.0f));

        int left = - ui->GetPixelSize().x / 2;
        int right = ui->GetPixelSize().x / 2;
        int bottom = - ui->GetPixelSize().y / 2;
        int top = ui->GetPixelSize().y / 2;

        //DebugLogInfo("Rel pos ");

        bool isMouseIntersecting = false;
        if (relCursorPos.x > left && relCursorPos.x < right && relCursorPos.y > bottom && relCursorPos.y < top) {
            //DebugLogInfo("Entered a ui ", ui->guiTextInfo.has_value() ? ui->guiTextInfo->text : "Unnamed");
            isMouseIntersecting = true;
        }

        //if (ui->guiTextInfo.has_value()) {
            //DebugLogInfo("BE ADVISED: ", cursorPos.y, " ", top, " ", bottom)
        //}

        if (ui->mouseHover != isMouseIntersecting) { // then the cursor has either moved onto or off of the gui.
            if (isMouseIntersecting) { // then we started being on the ui.
                ui->onMouseEnter->Fire();
               
            }
            else { // then we stopped being on the ui.
                ui->onMouseExit->Fire();
            }
        }

        // fire input events
        if (isMouseIntersecting && ui->onInputBegin->HasConnections()) {
            for (const InputObject& event : GraphicsEngine::Get().window.PRESS_BEGAN_KEYS) {
                ui->onInputBegin->Fire(event);
            }  
        }
        if (isMouseIntersecting && ui->onInputEnd->HasConnections()) {
            for (const InputObject& event : GraphicsEngine::Get().window.PRESS_ENDED_KEYS) {
                ui->onInputEnd->Fire(event);
            }
        }

        ui->mouseHover = isMouseIntersecting;
    }

    //DebugLogInfo("Checked ", c);
}

void Gui::Init()
{
    GraphicsEngine::Get().GetWindow().onWindowResize->Connect(&UpdateGuiForNewWindowResolution);
    GraphicsEngine::Get().GetWindow().postInputProccessing->Connect(&FireInputEvents);
    GraphicsEngine::Get().preRenderEvent->Connect(&UpdateBillboardGuis);
}


void Gui::UpdateBillboardGuis(float) {
    for (auto & ui: GuiGlobals::Get().listOfBillboardGuis) {
        ui->UpdateGuiTransform();
    }
}

#pragma warning(disable : 26829)
Gui::Gui(bool haveText, std::optional<std::pair<float, std::shared_ptr<Material>>> guiMaterial, std::optional<std::pair<float, std::shared_ptr<Material>>> fontMaterial,  std::optional<BillboardGuiInfo> billboardGuiInfo, bool clippingEnabled):
    material(clippingEnabled ? Material::Copy(guiMaterial.has_value() ? guiMaterial->second : (billboardGuiInfo.has_value() ? GraphicsEngine::Get().defaultBillboardGuiMaterial : GraphicsEngine::Get().defaultGuiMaterial)) : guiMaterial.has_value() ? guiMaterial->second : (billboardGuiInfo.has_value() ? GraphicsEngine::Get().defaultBillboardGuiMaterial : GraphicsEngine::Get().defaultGuiMaterial)),
    ownsMaterial(clippingEnabled),
    onMouseEnter(Event<>::New()),
    onMouseExit(Event<>::New()),
    onInputBegin(Event<InputObject>::New()),
    onInputEnd(Event<InputObject>::New())

{
   
    //DebugLogInfo("Generated gui ", this);
    
    GameobjectCreateParams objectParams({ComponentBitIndex::Transform, ComponentBitIndex::RenderNoFO});

    mouseHover = false;

    
    objectParams.materialId = material->id;
    materialLayer = guiMaterial.has_value() ? guiMaterial->first : -1;
    
    objectParams.meshId = Mesh::Square()->meshId;

    object = GameObject::New(objectParams);

    object->name = "GuiObject";

    guiScaleMode = ScaleXY;
    anchorPoint = {0, 0};

    offsetPos = {0, 0};
    scalePos = {0, 0};

    rotation = 0;
    zLevel = 0;

    offsetSize = {0, 0};
    scaleSize = {0.5, 0.5};

    rgba = {1, 0, 0, 1};

    if (haveText) {
        Assert(fontMaterial.has_value() && fontMaterial->second->Count(Texture::FontMap));

        auto fMat = clippingEnabled ? Material::Copy(fontMaterial->second) : fontMaterial->second;

        GameobjectCreateParams textObjectParams({ComponentBitIndex::Transform, ComponentBitIndex::RenderNoFO });
        textObjectParams.materialId = fMat->id;
        auto textMesh = Mesh::New(TextMeshProvider(MeshCreateParams{ .meshVertexFormat = MeshVertexFormat::DefaultGui() }, fMat), true);
        // auto textMesh = Mesh::Square();

        textObjectParams.meshId = textMesh->meshId;

        guiTextInfo.emplace(GuiTextInfo {
            .rgba = {0, 0, 1, 1},
            .lineHeight = 1,
            .leftMargin = 10,
            .rightMargin = 10,
            .topMargin = 10,
            .bottomMargin = 10,

            .horizontalAlignment = HorizontalAlignMode::Left,
            .verticalAlignment = VerticalAlignMode::Top,
            .text = "Text",
            .object = GameObject::New(textObjectParams),
            .fontMaterial = fMat,
            .fontMaterialLayer = fontMaterial->first
        });

        guiTextInfo->object->name = "GuiText";

    }

    billboardInfo = billboardGuiInfo;

    if (haveText) {
        UpdateGuiText();
    }
    UpdateGuiGraphics();
    UpdateGuiTransform();    
    
    GuiGlobals::Get().listOfGuis.push_back(this);
    if (billboardInfo.has_value()) {
        GuiGlobals::Get().listOfBillboardGuis.push_back(this);
    }
}

Gui::~Gui() {

    // avoid leaking copied material on gui destruction
    if (ownsMaterial) {
        Material::Destroy(material->id);
        if (guiTextInfo) {
            Material::Destroy(guiTextInfo->fontMaterial->id);
        }
    }

    Orphan();

    for (auto& child: children) { // orphan children
        child->parent = nullptr;
    }

    unsigned int i = 0;
    for (auto & ui: GuiGlobals::Get().listOfGuis) {
        if (ui == this) {
            // fast erase
            GuiGlobals::Get().listOfGuis.at(i) = GuiGlobals::Get().listOfGuis.back();
            GuiGlobals::Get().listOfGuis.pop_back();
            
            break;
        }
        i += 1;
    }

    if (billboardInfo) {
        i = 0;
        for (auto & ui: GuiGlobals::Get().listOfBillboardGuis) {
            if (ui == this) {
                // fast erase
                GuiGlobals::Get().listOfBillboardGuis.at(i) = GuiGlobals::Get().listOfBillboardGuis.back();
                GuiGlobals::Get().listOfBillboardGuis.pop_back();
                
                break;
            }
            i += 1;
        }
    }
    

    object->Destroy();
    if (guiTextInfo.has_value()) {
        guiTextInfo->object->Destroy();
    }
}

void Gui::UpdateGuiTransform() {

    // std::cout << "Res = " << glm::to_string(realWindowResolution) << ".\n";

    glm::vec2 size = GetPixelSize();
    // if (billboardInfo.has_value()) {
    //     size *= 0.01;
    // }

    object->RawGet<TransformComponent>()->SetScl(glm::vec3(size.x, size.y, 1));
    
    glm::vec2 centerPosition = GetPixelPos();

    

    glm::vec3 realPosition(centerPosition.x, centerPosition.y, zLevel);

     //if (billboardInfo.has_value() && !billboardInfo->followObject.expired()) { // TODO: make sure expired checks for nullptr?
         //realPosition = billboardInfo->followObject.lock()->Get<TransformComponent>()->Position();
     //}

    object->Get<TransformComponent>()->SetPos(realPosition);
    // object->Get<TransformComponent>().SetPos({0.0, 5.0, 0.0});

    glm::quat rot = glm::angleAxis(rotation + (float)glm::radians(180.0), glm::vec3 {0.0f, 0.0f, 1.0f});
    glm::quat textRot = rot * glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    // if (billboardInfo.has_value()) {
        
    //     if (billboardInfo->rotation.has_value()) {
    //         rot = billboardInfo->rotation.value() * rot;
    //         textRot = billboardInfo->rotation.value() * textRot;
    //     }
    //     else {
    //         rot = glm::inverse(glm::quat(glm::lookAt(GraphicsEngine::Get().GetCurrentCamera().position, glm::dvec3(realPosition), glm::dvec3(0.0, 1.0, 0.0)))) * rot;
    //         textRot = glm::inverse(glm::quat(glm::lookAt(GraphicsEngine::Get().CameraPosition(), glm::dvec3(realPosition), glm::dvec3(0.0, 1.0, 0.0)))) * textRot;
    //     }

    // }

    object->Get<TransformComponent>()->SetRot(rot);

    if (guiTextInfo.has_value()) {
         //guiTextInfo->object->Get<TransformComponent>().SetPos({50.0, 59.0, 0.0});
        guiTextInfo->object->Get<TransformComponent>()->SetPos(realPosition - glm::vec3(0, 0.0, 0.001));
        guiTextInfo->object->Get<TransformComponent>()->SetRot(textRot);
    }

    if (billboardInfo.has_value()) {
        object->Get<RenderComponent>()->SetInstancedVertexAttribute(MeshVertexFormat::ARBITRARY_ATTRIBUTE_1_NAME, zLevel);
        if (guiTextInfo.has_value())
            guiTextInfo->object->Get<RenderComponent>()->SetInstancedVertexAttribute(MeshVertexFormat::ARBITRARY_ATTRIBUTE_1_NAME, float(zLevel - 0.001));
    }

    for (auto& c : children) {
        c->UpdateGuiTransform();
    }

    UpdateScissorTest();
}

Gui::GuiTextInfo& Gui::GetTextInfo() {
    Assert(guiTextInfo.has_value());
    return *guiTextInfo;
}

Gui::BillboardGuiInfo& Gui::GetBillboardInfo() {
    Assert(billboardInfo.has_value());
    return *billboardInfo;
}

void Gui::UpdateGuiGraphics() {
    object->Get<RenderComponent>()->SetColor(rgba);
    object->Get<RenderComponent>()->SetTextureZ(materialLayer);
    
     
    if (guiTextInfo.has_value()) {
        
        guiTextInfo->object->Get<RenderComponent>()->SetColor(guiTextInfo->rgba);
        guiTextInfo->object->Get<RenderComponent>()->SetTextureZ(guiTextInfo->fontMaterialLayer);
    }
}

void Gui::UpdateScissorTest()
{
    if (clipTarget.has_value()) {
        Assert(ownsMaterial);
        bool clipping = !clipTarget->expired();

        // we do scissor test here too, since this function will be called when the clipTarget (an ancestor) moves / changes size too
        material->scissoringEnabled = clipping;
        if (guiTextInfo) {
            guiTextInfo->fontMaterial->scissoringEnabled = clipping;
        }

        if (clipping) {
            auto lockedClipTarget = clipTarget->lock();
            auto s = lockedClipTarget->GetPixelSize() / 2;
            material->scissorCorner1 = lockedClipTarget->GetPixelPos() + glm::vec2(s.x, -s.y);
            material->scissorCorner2 = lockedClipTarget->GetPixelPos() + glm::vec2(-s.x, s.y);

            if (guiTextInfo) {
                guiTextInfo->fontMaterial->scissorCorner1 = lockedClipTarget->GetPixelPos() + glm::vec2(s.x, -s.y);
                guiTextInfo->fontMaterial->scissorCorner2 = lockedClipTarget->GetPixelPos() + glm::vec2(-s.x, s.y);
            }
        }
        else {
            clipTarget = std::nullopt;
        }
    }
}

// TODO: better way?
// const GLfloat TEXT_SCALING_FACTOR = 1024.0f; 

void Gui::UpdateGuiText() {
    Assert(guiTextInfo.has_value());

    

    auto & textMesh = Mesh::Get(guiTextInfo->object->Get<RenderComponent>()->meshId);

    // convert margins into weird unit i made the actual function take
    float absoluteLeftMargin = guiTextInfo->leftMargin - GetPixelSize().x/2.0f;
    float absoluteRightMargin = GetPixelSize().x/2.0f + guiTextInfo->rightMargin;

    Assert(absoluteLeftMargin < absoluteRightMargin);

    float absoluteTopMargin = -GetPixelSize().y/2.0f + guiTextInfo->topMargin;
    float absoluteBottomMargin = GetPixelSize().y/2.0f - guiTextInfo->bottomMargin;

    auto params = TextMeshCreateParams {
        .leftMargin = absoluteLeftMargin,
        .rightMargin = absoluteRightMargin,
        .topMargin = absoluteTopMargin,
        .bottomMargin = absoluteBottomMargin,
        .lineHeightMultiplier = guiTextInfo->lineHeight,
        .horizontalAlignMode = guiTextInfo->horizontalAlignment,
        .verticalAlignMode = guiTextInfo->verticalAlignment
        
        // .scalingFactor = TEXT_SCALING_FACTOR
    };

    TextMeshProvider provider(MeshCreateParams::DefaultGui(), guiTextInfo->fontMaterial);
    provider.textParams = params;
    provider.text = guiTextInfo->text;
    textMesh->Remesh(provider, false);
}

glm::vec2 Gui::GetPixelSize() {
    glm::vec2 parentResolution = GetParentContainerScale();

    if (guiScaleMode == ScaleXX) {
        //parentResolution.x = parentResolution.x;
        parentResolution.y = parentResolution.x;
    }
    else if (guiScaleMode == ScaleXY) {
        //parentResolution.x = parentResolution.x;
        //parentResolution.y = parentResolution.y;
    }
    else if (guiScaleMode == ScaleYY) {
        parentResolution.x = parentResolution.y;
        //parentResolution.y = parentResolution.y;
    }
    else {
        std::cout << "Invalid guiScaleMode. Aborting.\n";
        abort();
    }    

    if (object->Get<TransformComponent>()->GetParent() != nullptr) {
        parentResolution *= glm::vec2 {object->Get<TransformComponent>()->GetParent()->Scale().x, object->Get<TransformComponent>()->GetParent()->Scale().y};
    }

    glm::vec2 size = (scaleSize * parentResolution) + offsetSize;

    return size;
}

glm::vec2 Gui::GetPixelPos()
{
    glm::vec2 containerSize = GetParentContainerScale();
    glm::vec2 size = GetPixelSize();

    glm::vec2 modifiedScalePos = scalePos;
    if (billboardInfo.has_value() && !billboardInfo->followObject.expired()) { // TODO: make sure expired checks for nullptr? Also should we just assert that object isn't expired?
        glm::vec3 projected = GraphicsEngine::Get().GetCurrentCamera().ProjectToScreen(billboardInfo->followObject.lock()->Get<TransformComponent>()->Position(), GraphicsEngine::Get().window.Aspect());
        modifiedScalePos += glm::vec2(projected);
        if (projected.z < 0 || projected.z > 1) { modifiedScalePos.x += 10000; }
        //else DebugLogInfo("Proj ", projected);
    }

    glm::vec2 anchorPointPosition;
    if (!parent) {
        anchorPointPosition = modifiedScalePos * containerSize + offsetPos;
    }
    else if (parent->childBehaviour == GuiChildBehaviour::Relative) {
        anchorPointPosition = parent->GetPixelPos() + modifiedScalePos * containerSize + offsetPos;
    }
    else if (parent->childBehaviour == GuiChildBehaviour::Grid) {
        anchorPointPosition = parent->GetPixelPos() + gridPos + offsetPos;
    }
    else {
        Assert(false);
    }
   
    glm::vec2 centerPosition = anchorPointPosition - (anchorPoint * size);

    // std::cout << "Center pos is " << glm::to_string(centerPosition) << ".\n";
    // std::cout << "Size is " << glm::to_string(size) << ".\n";

    return centerPosition;
}

bool Gui::IsMouseOver()
{
    return mouseHover;
}

const std::vector<std::shared_ptr<Gui>>& Gui::GetChildren()
{
    return children;
}

void Gui::SortChildren()
{
    Assert(gridInfo.semiAxisFillDir == 1 || gridInfo.semiAxisFillDir == -1);

    std::stable_sort(children.begin(), children.end(), [](const std::shared_ptr<Gui>& a, const std::shared_ptr<Gui>& b) { return a->sortValue < b->sortValue; });

    int fillAxis = gridInfo.fillXFirst ? 0 : 1;
    int currentFillLen = 0;
    int currentFillHeight = 0; // 
    glm::vec2 currentPos = gridInfo.gridOffsetPosition + gridInfo.gridScalePosition * GetPixelSize();
    glm::vec2 stride = gridInfo.gridOffsetSize + gridInfo.gridScaleSize * GetParentContainerScale();
    for (auto& ui : children) {
        ui->gridPos = currentPos;
        currentFillLen += gridInfo.maxInPixels ? ui->GetPixelSize()[fillAxis] : 1;
        currentPos[fillAxis] += stride[fillAxis];
        if (gridInfo.addGuiLengths)
            currentPos[fillAxis] += ui->GetPixelSize()[fillAxis];
            currentFillHeight = std::max(currentFillHeight, (int)ui->GetPixelSize()[1 - fillAxis]);
        if (currentFillLen > gridInfo.maxInFillDirection) {
            currentFillLen = 0;
            currentPos[fillAxis] = gridInfo.gridOffsetPosition[fillAxis] + gridInfo.gridScalePosition[fillAxis] * GetPixelSize()[fillAxis];
            currentPos[1 - fillAxis] += stride[1 - fillAxis] + gridInfo.semiAxisFillDir * currentFillHeight;
            currentFillHeight = 0;
        }
    }
}

void Gui::SetParent(Gui* newParent)
{
    if (parent != newParent) {
        Orphan();
    }
    parent = newParent;
    
    parent->children.push_back(shared_from_this());
}

Gui* Gui::GetParent()
{
    return parent;
}

void Gui::Orphan() {
    if (parent) { // remove child
        for (int i = 0; i < parent->children.size(); i++) {
            if (parent->children[i].get() == this) {
                parent->children[i] = parent->children.back();
                parent->children.pop_back();
                break;
            }
        }
    }
    parent = nullptr;
}

glm::vec2 Gui::GetParentContainerScale()
{
    glm::vec2 parentResolution{};

    if (!parent) {
        parentResolution = glm::vec2(GraphicsEngine::Get().window.width, GraphicsEngine::Get().window.height);
    }
    else {
        if (parent->childBehaviour == GuiChildBehaviour::Relative) {
            parentResolution = parent->GetPixelSize();
        }
        else if (parent->childBehaviour == GuiChildBehaviour::Grid) {
            if (parent->parent)
                parentResolution = parent->parent->GetPixelSize();
            else
                parentResolution = glm::vec2(GraphicsEngine::Get().window.width, GraphicsEngine::Get().window.height);
        }
        else {
            Assert(false);
        }
    }

    return parentResolution;
}
