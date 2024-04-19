#include "gui.hpp"
#include <cassert>
#include <vector>
#include "../../external_headers/GLM/gtx/string_cast.hpp"

std::vector<Gui*> listOfGuis;
void Gui::UpdateGuiForNewWindowResolution() {
    std::cout << "UPDATEing " << listOfGuis.size() << ".\n";
    for (auto & ui: listOfGuis) {
        std::cout << "UPDATE\n";
        if (ui->guiTextInfo.has_value()) {
            ui->UpdateGuiText();
        }
        ui->UpdateGuiTransform();
    }
}

std::vector<Gui*> listOfBillboardGuis;
void Gui::UpdateBillboardGuis() {
    for (auto & ui: listOfBillboardGuis) {
        ui->UpdateGuiTransform();
    }
}

Gui::Gui(bool haveText, std::optional<std::pair<float, std::shared_ptr<Material>>> fontMaterial, std::optional<std::pair<float, std::shared_ptr<Material>>> guiMaterial, std::optional<BillboardGuiInfo> billboardGuiInfo, std::shared_ptr<ShaderProgram> guiShader) {
    GameobjectCreateParams objectParams({ComponentRegistry::TransformComponentBitIndex, ComponentRegistry::RenderComponentNoFOBitIndex});

    objectParams.materialId = (guiMaterial.has_value() ? guiMaterial->second->id : 0);
    objectParams.meshId = Mesh::Square()->meshId;
    objectParams.shaderId = guiShader->shaderProgramId;
    object = ComponentRegistry::NewGameObject(objectParams);

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
        assert(fontMaterial.has_value() && fontMaterial->second->HasFontMap());

        GameobjectCreateParams textObjectParams({ComponentRegistry::TransformComponentBitIndex, ComponentRegistry::RenderComponentNoFOBitIndex});
        textObjectParams.materialId = fontMaterial->second->id;
        textObjectParams.shaderId = guiShader->shaderProgramId;
        auto textMesh = Mesh::Text();
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
            .object = ComponentRegistry::NewGameObject(textObjectParams),
            .fontMaterial = fontMaterial->second,
            .fontMaterialLayer = fontMaterial->first
        });

        

    }

    billboardInfo = billboardGuiInfo;

    if (haveText) {
        UpdateGuiText();
    }
    UpdateGuiGraphics();
    UpdateGuiTransform();    
    
    listOfGuis.push_back(this);
    if (billboardInfo.has_value()) {
        listOfBillboardGuis.push_back(this);
    }
}

Gui::~Gui() {
    unsigned int i = 0;
    for (auto & ui: listOfGuis) {
        if (ui == this) {
            // fast erase
            listOfGuis.at(i) = listOfGuis.back();
            listOfGuis.pop_back();
            
            break;
        }
        i += 1;
    }

    if (billboardInfo) {
        i = 0;
        for (auto & ui: listOfBillboardGuis) {
            if (ui == this) {
                // fast erase
                listOfBillboardGuis.at(i) = listOfBillboardGuis.back();
                listOfBillboardGuis.pop_back();
                
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
    glm::vec2 realWindowResolution = {GraphicsEngine::Get().window.width, GraphicsEngine::Get().window.height};
    // std::cout << "Res = " << glm::to_string(realWindowResolution) << ".\n";

    glm::vec2 size = GetPixelSize();

    object->transformComponent->SetScl(glm::vec3(size.x, size.y, 1));
    
    object->transformComponent->SetRot(glm::angleAxis(rotation, glm::vec3 {0.0f, 0.0f, 1.0f}));

    glm::vec2 anchorPointPosition = scalePos * realWindowResolution + offsetPos;
    glm::vec2 centerPosition = anchorPointPosition - (anchorPoint * size);
    // std::cout << "Center pos is " << glm::to_string(centerPosition) << ".\n";
    // std::cout << "Size is " << glm::to_string(size) << ".\n";

    if (billboardInfo.has_value() && !billboardInfo->followObject.expired()) { // TODO: make sure expired checks for nullptr?
        
    }

    object->transformComponent->SetPos(glm::vec3(centerPosition.x, centerPosition.y, zLevel));
    if (guiTextInfo.has_value()) {
        guiTextInfo->object->transformComponent->SetPos(glm::vec3(centerPosition.x, centerPosition.y, zLevel + 0.01));
        guiTextInfo->object->transformComponent->SetRot(glm::angleAxis(rotation + (float)glm::radians(180.0), glm::vec3 {0.0f, 0.0f, 1.0f}) * glm::angleAxis(glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
        // guiTextInfo->object->transformComponent->SetScl(glm::vec3(size.x, size.y, 1));
    }
}

Gui::GuiTextInfo& Gui::GetTextInfo() {
    assert(guiTextInfo.has_value());
    return *guiTextInfo;
}

Gui::BillboardGuiInfo& Gui::GetBillboardInfo() {
    assert(billboardInfo.has_value());
    return *billboardInfo;
}

void Gui::UpdateGuiGraphics() {
    object->renderComponent->SetColor(rgba);
    object->renderComponent->SetTextureZ(materialLayer.value_or(-1.0));
     
     if (guiTextInfo.has_value()) {
        guiTextInfo->object->renderComponent->SetColor(guiTextInfo->rgba);
        guiTextInfo->object->renderComponent->SetTextureZ(guiTextInfo->fontMaterialLayer);
     }
}

// TODO: better way?
// const GLfloat TEXT_SCALING_FACTOR = 1024.0f; 

void Gui::UpdateGuiText() {
    assert(guiTextInfo.has_value());

    

    auto & textMesh = Mesh::Get(guiTextInfo->object->renderComponent->meshId);
    auto [vers, inds] = textMesh->StartModifying();

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
    TextMeshFromText(guiTextInfo->text, guiTextInfo->fontMaterial->fontMapConstAccess.value(), params, textMesh->vertexFormat, vers, inds);
    textMesh->StopModifying(false);
}

glm::vec2 Gui::GetPixelSize() {
    glm::vec2 windowResolution;
    
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

    if (object->transformComponent->GetParent() != nullptr) {
        windowResolution *= glm::vec2 {object->transformComponent->GetParent()->Scale().x, object->transformComponent->GetParent()->Scale().y};
    }

    glm::vec2 size = (scaleSize * windowResolution) + offsetSize;

    return size;
}