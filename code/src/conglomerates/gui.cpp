#include "gui.hpp"
#include "debug/assert.hpp"
#include <vector>
#include "glm/gtx/string_cast.hpp"
#include "graphics/gengine.hpp"
#include "graphics/mesh.hpp"

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
void Gui::FireInputEvents()
{
    for (auto& ui : GuiGlobals::Get().listOfGuis) {

        // if no one cares whether the mouse is on this gui, don't bother calculating it.
        if (!ui->onMouseEnter->HasConnections() && !ui->onMouseExit->HasConnections() && !ui->onInputBegin->HasConnections() && !ui->onInputEnd->HasConnections()) {
            continue;
        }

        // determine if mouse is over the gui.

        // so that we can treat the gui's rotation as 0 and do a simple AABB-point intersection test, we rotate the cursor position by the inverse of the gui's current rotation.
        glm::vec2 relCursorPos = ui->GetPixelPos() - glm::vec2(GraphicsEngine::Get().GetWindow().MOUSE_POS);
        glm::uvec2 cursorPos = glm::vec2(ui->GetPixelPos()) + glm::vec2(glm::quat(ui->rotation, glm::vec3(0, 0, 1)) * glm::vec3(relCursorPos, 0.0f));

        unsigned int left = ui->GetPixelPos().x - ui->GetPixelSize().x / 2;
        unsigned int right = ui->GetPixelPos().x + ui->GetPixelSize().x / 2;
        unsigned int bottom = ui->GetPixelPos().y - ui->GetPixelSize().y / 2;
        unsigned int top = ui->GetPixelPos().y + ui->GetPixelSize().y / 2;

        bool isMouseIntersecting = false;
        if (cursorPos.x > left && cursorPos.x < right && cursorPos.y > bottom && cursorPos.y < top) {
            isMouseIntersecting = true;
        }


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

Gui::Gui(bool haveText, std::optional<std::pair<float, std::shared_ptr<Material>>> fontMaterial, std::optional<std::pair<float, std::shared_ptr<Material>>> guiMaterial, std::optional<BillboardGuiInfo> billboardGuiInfo, std::shared_ptr<ShaderProgram> guiShader):
    onMouseEnter(Event<>::New()),
    onMouseExit(Event<>::New()),
    onInputBegin(Event<InputObject>::New()),
    onInputEnd(Event<InputObject>::New())
{
    //DebugLogInfo("Generated gui ", this);
    
    GameobjectCreateParams objectParams({ComponentBitIndex::Transform, billboardGuiInfo.has_value() ? ComponentBitIndex::Render : ComponentBitIndex::RenderNoFO});

    mouseHover = false;

    objectParams.materialId = (guiMaterial.has_value() ? guiMaterial->second->id : 0);
    objectParams.meshId = Mesh::Square()->meshId;
    objectParams.shaderId = guiShader->shaderProgramId;

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
        Assert(fontMaterial.has_value() && fontMaterial->second->textures->Count(Texture::FontMap));

        GameobjectCreateParams textObjectParams({ComponentBitIndex::Transform, billboardGuiInfo.has_value() ? ComponentBitIndex::Render : ComponentBitIndex::RenderNoFO });
        textObjectParams.materialId = fontMaterial->second->id;
        textObjectParams.shaderId = guiShader->shaderProgramId;
        auto textMesh = Mesh::New(TextMeshProvider(MeshCreateParams{ .meshVertexFormat = MeshVertexFormat::DefaultGui() }, fontMaterial->second), true);
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
            .fontMaterial = fontMaterial->second,
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

    //DebugLogInfo("Destroyed gui ", this);
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

    // if (billboardInfo.has_value() && !billboardInfo->followObject.expired()) { // TODO: make sure expired checks for nullptr?
    //     realPosition = billboardInfo->followObject.lock()->Get<TransformComponent>().Position();
    // }

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
        guiTextInfo->object->Get<TransformComponent>()->SetPos(realPosition + glm::vec3(0, 0.0, 0.1));
        guiTextInfo->object->Get<TransformComponent>()->SetRot(textRot);
    }


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
    object->Get<RenderComponent>()->SetTextureZ(materialLayer.value_or(-1.0));
    object->Get<RenderComponent>()->SetInstancedVertexAttribute(MeshVertexFormat::ARBITRARY_ATTRIBUTE_1_NAME, zLevel);
     
    if (guiTextInfo.has_value()) {
        guiTextInfo->object->Get<RenderComponent>()->SetInstancedVertexAttribute(MeshVertexFormat::ARBITRARY_ATTRIBUTE_1_NAME, float(zLevel - 0.01));
        guiTextInfo->object->Get<RenderComponent>()->SetColor(guiTextInfo->rgba);
        guiTextInfo->object->Get<RenderComponent>()->SetTextureZ(guiTextInfo->fontMaterialLayer);
    }
}

// TODO: better way?
// const GLfloat TEXT_SCALING_FACTOR = 1024.0f; 

void Gui::UpdateGuiText() {
    Assert(guiTextInfo.has_value());

    

    auto & textMesh = Mesh::Get(guiTextInfo->object->Get<RenderComponent>()->meshId);

    // convert margins into weird unit i made the actual function take
    float absoluteLeftMargin = guiTextInfo->leftMargin - GetPixelSize().x/2.0f;
    float absoluteRightMargin = GetPixelSize().x/2.0f - guiTextInfo->rightMargin;

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

    TextMeshProvider provider(MeshCreateParams::Default(), guiTextInfo->fontMaterial);
    provider.textParams = params;
    provider.text = guiTextInfo->text;
    textMesh->Remesh(provider, false);
}

glm::vec2 Gui::GetPixelSize() {
    glm::vec2 windowResolution {};
    
    if (guiScaleMode == ScaleXX) {
        windowResolution.x = GraphicsEngine::Get().window.width;
        windowResolution.y = GraphicsEngine::Get().window.width;
    }
    else if (guiScaleMode == ScaleXY) {
        windowResolution.x = GraphicsEngine::Get().window.width;
        windowResolution.y = GraphicsEngine::Get().window.height;
    }
    else if (guiScaleMode == ScaleYY) {
        windowResolution.x = GraphicsEngine::Get().window.height;
        windowResolution.y = GraphicsEngine::Get().window.height;
    }
    else {
        std::cout << "Invalid guiScaleMode. Aborting.\n";
        abort();
    }    

    if (object->Get<TransformComponent>()->GetParent() != nullptr) {
        windowResolution *= glm::vec2 {object->Get<TransformComponent>()->GetParent()->Scale().x, object->Get<TransformComponent>()->GetParent()->Scale().y};
    }

    glm::vec2 size = (scaleSize * windowResolution) + offsetSize;

    return size;
}

glm::vec2 Gui::GetPixelPos()
{
    glm::vec2 realWindowResolution = { GraphicsEngine::Get().window.width, GraphicsEngine::Get().window.height };
    glm::vec2 size = GetPixelSize();

    glm::vec2 modifiedScalePos = scalePos;
    if (billboardInfo.has_value() && !billboardInfo->followObject.expired()) { // TODO: make sure expired checks for nullptr?
        glm::vec3 projected = GraphicsEngine::Get().GetCurrentCamera().ProjectToScreen(billboardInfo->followObject.lock()->Get<TransformComponent>()->Position(), GraphicsEngine::Get().window.Aspect());
        modifiedScalePos += glm::vec2(projected);
        if (projected.z < 0 || projected.z > 1) { modifiedScalePos.x += 10000; }
    }

    glm::vec2 anchorPointPosition = modifiedScalePos * realWindowResolution + offsetPos;
    glm::vec2 centerPosition = anchorPointPosition - (anchorPoint * size);
    // std::cout << "Center pos is " << glm::to_string(centerPosition) << ".\n";
    // std::cout << "Size is " << glm::to_string(size) << ".\n";

    return centerPosition;
}

bool Gui::IsMouseOver()
{
    return mouseHover;
}
