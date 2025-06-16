#pragma once
#include "../gameobjects/gameobject.hpp"
#include "basic_renderer.hpp"

// Useful for making skybox shaders
static inline std::shared_ptr<ShaderProgram> GetDefaultSkyboxShaderProgram() {
    static auto shader = ShaderProgram::New("../shaders/skybox_vertex.glsl", "../shaders/skybox_fragment.glsl");
    return shader;
}

static inline std::shared_ptr<Material> GetDefaultSkyboxMaterial() {
    auto [layer, mat] = Material::New(MaterialCreateParams{ .shader = GetDefaultSkyboxShaderProgram() });
    mat->drawOrder = 500;

    if (BasicRenderer::used) {
        mat->inputProvider = BasicRenderer::PrepNormalRendering;
    }

    return mat;
}

// material defaults to a logical choice with drawOrder = 500 if not provided
// you can discard return value, skybox will still be there ofc
static inline std::shared_ptr<GameObject> MakeSkybox(std::shared_ptr<Material> skyboxMaterial = nullptr, float layer = 0) {
    if (!skyboxMaterial) {
        static std::shared_ptr<Material> defaultMaterial = GetDefaultSkyboxMaterial();
        skyboxMaterial = defaultMaterial;
    }

    // the skybox's z-coord is hardcoded to 1 so it's not drawn over anything, but depth buffer is all 1 by default so this makes skybox able to be drawn
    skyboxMaterial->depthTestFunc = DepthTestMode::LEqual;
        
    auto skyboxImport = Mesh::MultiFromFile("../models/skybox.obj", MeshCreateParams{ .textureZ = -1.0, .opacity = 1, .expectedCount = 1, .normalizeSize = false }).at(0);
    auto goParams = GameobjectCreateParams({ ComponentBitIndex::Render, ComponentBitIndex::Transform });
    goParams.materialId = skyboxMaterial->id;
    goParams.meshId = skyboxImport.mesh->meshId;
    auto skybox = GameObject::New(goParams);
    skybox->RawGet<RenderComponent>()->SetTextureZ(layer);

    return skybox;
}