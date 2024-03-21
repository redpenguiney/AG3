#include "gui.hpp"
#include <cassert>
#include <memory>
#include <vector>

Gui::Gui(bool haveText, std::optional<std::pair<float, std::shared_ptr<Material>>> fontMaterial, std::optional<std::pair<float, std::shared_ptr<Material>>> guiMaterial, std::shared_ptr<ShaderProgram> guiShader) {
    
    GameobjectCreateParams objectParams({ComponentRegistry::TransformComponentBitIndex, ComponentRegistry::RenderComponentBitIndex});
    objectParams.materialId = (guiMaterial.has_value() ? 0: guiMaterial->second->id);
    objectParams.meshId = Mesh::Square()->meshId;
    objectParams.shaderId = guiShader->shaderProgramId;

    object = ComponentRegistry::NewGameObject(objectParams);

    guiScaleMode = ScaleXY;
    anchorPoint = {0, 0};

    offsetPos = {0, 0};
    scalePos = {0, 0};

    rotation = 0;

    offsetSize = {0, 0};
    scaleSize = {0, 0};

    rgba = {1, 1, 1, 1};

    if (haveText) {
        assert(fontMaterial.has_value() && fontMaterial->second->HasFontMap());

        GameobjectCreateParams textObjectParams({ComponentRegistry::TransformComponentBitIndex, ComponentRegistry::RenderComponentBitIndex});
        textObjectParams.materialId = 0;
        textObjectParams.shaderId = guiShader->shaderProgramId;
        textObjectParams.meshId = Mesh::FromText("TEXT", *fontMaterial->second->fontMapConstAccess)->meshId;

        guiTextInfo.emplace(GuiTextInfo {
            .rgba = {0, 0, 0, 1},
            .textHeight = 12,
            .text = "Text",
            .object = ComponentRegistry::NewGameObject(textObjectParams),
            .fontMaterial = fontMaterial->second,
            .fontMaterialLayer = fontMaterial->first
        });
    }

    UpdateGuiTransform();
    UpdateGuiGraphics();
    UpdateGuiText();
}

void Gui::UpdateGuiGraphics() {

}

void Gui::UpdateGuiText() {
    assert(guiTextInfo.has_value());

    auto & textMesh = Mesh::Get(guiTextInfo->object->renderComponent->meshId);
    auto [vers, inds] = textMesh->StartModifying();
    TextMeshFromText(guiTextInfo->text, guiTextInfo->fontMaterial->fontMapConstAccess.value(), textMesh->vertexFormat, vers, inds);
    textMesh->StopModifying(true);
    // guiTextInfo->object->transformComponent->SetScl(textMesh->originalSize * 0.01f);
    ttt += 'a';
}