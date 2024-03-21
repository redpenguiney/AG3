#include "gui.hpp"
#include <cassert>
#include <memory>
#include <vector>

Gui::Gui(bool haveText, std::optional<std::pair<float, std::shared_ptr<Material>>> fontMaterial, std::optional<std::pair<float, std::shared_ptr<Material>>> guiMaterial, std::shared_ptr<ShaderProgram> guiShader) {
    
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

    GameobjectCreateParams objectParams({ComponentRegistry::TransformComponentBitIndex, ComponentRegistry::RenderComponentBitIndex});

    std::vector<GLfloat> squareVerts = {
        -1.0, -1.0, 0.0,   0.0, 0.0, 
         1.0, -1.0, 0.0,   1.0, 0.0,
         1.0,  1.0, 0.0,   1.0, 1.0,
        -1.0,  1.0, 0.0,   0.0, 1.0,
         };

    objectParams.materialId = (guiMaterial.has_value() ? 0: guiMaterial->second->id);
    objectParams.meshId = nononoyoucantdothisMesh::FromVertices(squareVerts, {0, 1, 2, 1, 2, 3}, false, MeshVertexFormat::DefaultGui())->meshId;
    object = ComponentRegistry::NewGameObject(objectParams);
}