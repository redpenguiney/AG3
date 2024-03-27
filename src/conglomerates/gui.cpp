#include "gui.hpp"
#include <cassert>
#include <vector>
#include "../../external_headers/GLM/gtx/string_cast.hpp"

std::vector<Gui*> listOfGuis;
void Gui::UpdateGuiForNewWindowResolution() {
    for (auto & ui: listOfGuis) {
        ui->UpdateGuiTransform();
    }
}

Gui::Gui(bool haveText, std::optional<std::pair<float, std::shared_ptr<Material>>> fontMaterial, std::optional<std::pair<float, std::shared_ptr<Material>>> guiMaterial, std::shared_ptr<ShaderProgram> guiShader) {
    GameobjectCreateParams objectParams({ComponentRegistry::TransformComponentBitIndex, ComponentRegistry::RenderComponentNoFOBitIndex});

    std::cout << "Gui has value = " << guiMaterial.has_value() << " font is " << fontMaterial.has_value() << ".\n";
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

    rgba = {1, 0, 0, 0};

    if (haveText) {
        assert(fontMaterial.has_value() && fontMaterial->second->HasFontMap());

        GameobjectCreateParams textObjectParams({ComponentRegistry::TransformComponentBitIndex, ComponentRegistry::RenderComponentNoFOBitIndex});
        textObjectParams.materialId = fontMaterial->second->id;
        textObjectParams.shaderId = guiShader->shaderProgramId;
        auto textMesh = Mesh::FromText("Text", fontMaterial->second->fontMapConstAccess.value());
        // auto textMesh = Mesh::Square();
        std::cout << "Text mesh vertices: ";
        for (auto & v: textMesh->vertices) {
            std::cout << v << ", ";
        } 
        std::cout << ".\n";
        textObjectParams.meshId = textMesh->meshId;

        guiTextInfo.emplace(GuiTextInfo {
            .rgba = {0, 0, 1, 1},
            .textHeight = 12,
            .text = "Text",
            .object = ComponentRegistry::NewGameObject(textObjectParams),
            .fontMaterial = fontMaterial->second,
            .fontMaterialLayer = fontMaterial->first
        });

        std::cout << "Object in gui has matid" << guiTextInfo->object->renderComponent->materialId << ".\n"; 
        std::cout << "Object in back has matid" << object->renderComponent->materialId << ".\n"; 
    }

    if (haveText) {
        UpdateGuiText();
    }
    UpdateGuiGraphics();
    UpdateGuiTransform();    
    
    listOfGuis.push_back(this);
}

Gui::~Gui() {
    unsigned int i = 0;
    for (auto & ui: listOfGuis) {
        if (ui == this) {
            // fast erase
            listOfGuis.at(i) = listOfGuis.back();
            listOfGuis.pop_back();
        }
        i += 1;
    }
}

void Gui::UpdateGuiTransform() {
    glm::vec2 windowResolution;
    glm::vec2 realWindowResolution = {GraphicsEngine::Get().window.width, GraphicsEngine::Get().window.height};
    
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

    glm::vec2 size = (scaleSize / realWindowResolution * windowResolution) + (offsetSize/realWindowResolution);
    object->transformComponent->SetScl(glm::vec3(size.x, size.y, 1));
    
    object->transformComponent->SetRot(glm::angleAxis(rotation, glm::vec3 {0.0f, 0.0f, 1.0f}));

    glm::vec2 anchorPointPosition = scalePos + (offsetPos/realWindowResolution);
    glm::vec2 centerPosition = anchorPointPosition - (anchorPoint * size);
    // std::cout << "Center pos is " << glm::to_string(centerPosition) << ".\n";

    object->transformComponent->SetPos(glm::vec3(centerPosition.x, centerPosition.y, zLevel));
    if (guiTextInfo.has_value()) {
        guiTextInfo->object->transformComponent->SetPos(glm::vec3(centerPosition.x, centerPosition.y, zLevel - 0.01));
        guiTextInfo->object->transformComponent->SetRot(glm::angleAxis(rotation, glm::vec3 {0.0f, 0.0f, 1.0f}));
        guiTextInfo->object->transformComponent->SetScl(glm::vec3(size.x, size.y, 1));
    }
}

Gui::GuiTextInfo& Gui::GetTextInfo() {
    assert(guiTextInfo.has_value());
    return *guiTextInfo;
}

void Gui::UpdateGuiGraphics() {
    object->renderComponent->SetColor(rgba);
    object->renderComponent->SetTextureZ(materialLayer.value_or(-1.0));

    assert(guiTextInfo->rgba.a == 1.0);
     
     if (guiTextInfo.has_value()) {
        guiTextInfo->object->renderComponent->SetColor(guiTextInfo->rgba);
        guiTextInfo->object->renderComponent->SetTextureZ(guiTextInfo->fontMaterialLayer);
     }
}

void Gui::UpdateGuiText() {
    assert(guiTextInfo.has_value());

    // auto & textMesh = Mesh::Get(guiTextInfo->object->renderComponent->meshId);
    // auto [vers, inds] = textMesh->StartModifying();
    // TextMeshFromText(guiTextInfo->text, guiTextInfo->fontMaterial->fontMapConstAccess.value(), textMesh->vertexFormat, vers, inds);
    // textMesh->StopModifying(false);
}