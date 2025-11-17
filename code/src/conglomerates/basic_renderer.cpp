#include "basic_renderer.hpp"
#include <gameobjects/gameobject.hpp>



BasicRenderer& BasicRenderer::Setup(std::shared_ptr<ShaderProgram> postProcShader) {
    used = true;
    static BasicRenderer renderer(postProcShader);
    return renderer;
}

std::shared_ptr<Material> GetTMat() {
    auto transparentMaterial = Material::Copy(GraphicsEngine::Get().defaultMaterial);
    transparentMaterial->depthMaskEnabled = false;
    transparentMaterial->drawOrder = BasicRenderer::TRANSPARENT_DRAW_ORDER;
    transparentMaterial->shader = ShaderProgram::New("../shaders/world_vertex.glsl", "../shaders/world_fragment_translucent.glsl");

    //transparentMaterial->depthTestFunc = DepthTestMode::Disabled;
    transparentMaterial->blendingSrcFactor = { BlendFactorMode::One, BlendFactorMode::Zero, };
    transparentMaterial->blendingDstFactor = { BlendFactorMode::One, BlendFactorMode::OneMinusSrcColor };

    return transparentMaterial;
}

std::shared_ptr<Material>& BasicRenderer::GetDefaultTransparentMaterial()
{
    static auto mat = GetTMat();
    return mat;
}

// Makes sure the framebuffer is the right size (in case they resized window or smtg).
static void UpdateFramebuffer() {
    auto& BE = BasicRenderer::Setup();
    auto width = GraphicsEngine::Get().window.width;
    auto height = GraphicsEngine::Get().window.height;
    if (!BE.mainFramebuffer.has_value() || BE.mainFramebuffer->width != width || BE.mainFramebuffer->height != height) {
        TextureCreateParams colorTextureParams({}, Texture::ColorMap);
        colorTextureParams.filteringBehaviour = Texture::LinearTextureFiltering;
        colorTextureParams.mipmapBehaviour = Texture::NoMipmaps;
        colorTextureParams.format = Texture::RGBA_16Float;
        colorTextureParams.wrappingBehaviour = Texture::WrapClampToEdge;

        // accumulation
        TextureCreateParams accumulationTextureParams({}, Texture::ColorMap);
        accumulationTextureParams.filteringBehaviour = Texture::LinearTextureFiltering;
        accumulationTextureParams.mipmapBehaviour = Texture::NoMipmaps;
        accumulationTextureParams.format = Texture::RGBA_16Float;
        accumulationTextureParams.wrappingBehaviour = Texture::WrapClampToEdge;

        // revealage
        TextureCreateParams revealageTextureParams({}, Texture::ColorMap);
        revealageTextureParams.filteringBehaviour = Texture::LinearTextureFiltering;
        revealageTextureParams.mipmapBehaviour = Texture::NoMipmaps;
        revealageTextureParams.format = Texture::Grayscale_8Bit;
        revealageTextureParams.wrappingBehaviour = Texture::WrapClampToEdge;

        BE.mainFramebuffer.emplace(width, height, std::vector{ colorTextureParams, accumulationTextureParams, revealageTextureParams }, true);
    }
}

void BasicRenderer::AddShader(std::shared_ptr<ShaderProgram> shader) {
    for (auto& s : shaders) {
        if (s.lock().get() == shader.get()) return;
    }
    shaders.push_back(shader);

    SetEnvironmentalLighting(currentLighting);
}

void BasicRenderer::PrepPostprocessing(Material* material, std::shared_ptr<ShaderProgram> _) {
    auto& BE = BasicRenderer::Setup();

    UpdateFramebuffer();

    // clearing depth/accum/reveal for purposes of the next frame; only the color is still needed, and next frame will paint it over assuming skybox exists
    BE.mainFramebuffer->Bind(); // TODO: we should try to clear the framebuffer without an unneccesary binding by doing it after the last time the mainframebuffer is drawn to. Gonna need to set up the draw queue system first.
    BE.mainFramebuffer->ClearDepthRenderbuffer();
    BE.mainFramebuffer->Unbind();
    BE.mainFramebuffer->textureAttachments[0].Use();

   

    //DebugLogInfo("POSTPROC");
}


static void PrepOITComposition(Material* material, std::shared_ptr<ShaderProgram> _) {
    auto& BE = BasicRenderer::Setup();

    UpdateFramebuffer();

    
    BE.mainFramebuffer->Bind({0,});
    BE.mainFramebuffer->textureAttachments[1].Use();
    BE.mainFramebuffer->textureAttachments[2].Use();

    //DebugLogInfo("OIT COMPOSITION.");
}

void BasicRenderer::PrepNormalRendering(Material* material, std::shared_ptr<ShaderProgram> _) {
    //DebugLogInfo("Normal mat.");
}

void BasicRenderer::PreRendering(Material*, std::shared_ptr<ShaderProgram>) {
    auto& BE = BasicRenderer::Setup();

    UpdateFramebuffer();
    BE.mainFramebuffer->Bind({ 0, });
    BE.mainFramebuffer->Clear({ {0, 0, 0, 0 }, });
    BE.mainFramebuffer->ClearDepthRenderbuffer();
    //BE.mainFramebuffer->Clear({ {1, 1, 0, 1}, {-1, -1, -1, -1}, {-1, -1, -1, -1}});
}

void BasicRenderer::PreTransparentRendering(Material*, std::shared_ptr<ShaderProgram>) {
    auto& BE = BasicRenderer::Setup();
    BE.mainFramebuffer->Bind({ 1, 2 });
    BE.mainFramebuffer->Clear({ { 0, 0, 0, 0 }, { 1, 1, 1, 1 } });
}

void BasicRenderer::SetEnvironmentalLighting(const EnvironmentalLighting& light) {
    currentLighting = light;

    for (int i = 0; i < shaders.size(); i++) {
        auto& shader = shaders[i];
        if (auto ptr = shader.lock()) {
            ptr->Uniform("envLightDirection", light.dir);
            ptr->Uniform("envLightColor", light.color);
            ptr->Uniform("envLightDiffuse", light.diffuseStrength);
            ptr->Uniform("envLightAmbient", light.ambientStrength);
            ptr->Uniform("envLightSpecular", light.specularStrength);
        }
        else {
            shaders[i] = shaders.back();
            shaders.pop_back();
            i--;
        }
    }
}

BasicRenderer::BasicRenderer(std::shared_ptr<ShaderProgram> postProcShader) {
    GetDefaultTransparentMaterial();
    SetEnvironmentalLighting(currentLighting);

    if (!postProcShader)
        postProcShader = ShaderProgram::New("../shaders/postproc_vertex.glsl", "../shaders/postproc_fragment.glsl", false, false);

    GraphicsEngine::Get().defaultMaterial->inputProvider = ShaderInputProvider(PrepNormalRendering);
    //GraphicsEngine::Get().defaultMaterial->depthTestFunc = DepthTestMode::Disabled;

    shaders.push_back(GraphicsEngine::Get().defaultMaterial->shader);
    shaders.push_back(GetDefaultTransparentMaterial()->shader);

    auto [_, predrawMaterial] = Material::New(MaterialCreateParams{
        .shader = nullptr,
        .depthMask = true,
        .requireUniqueTextureCollection = true,
        .inputProvider = ShaderInputProvider(PreRendering),
        .depthTestFunc = DepthTestMode::LEqual,
        .blendingSrcFactor = { BlendFactorMode::SrcAlpha, },
        .blendingDstFactor = { BlendFactorMode::OneMinusSrcAlpha, },
        .drawOrder = PREDRAW_DRAW_ORDER,
    });
    predrawMaterial->abstract = true;
    auto predrawParams = GameobjectCreateParams({ ComponentBitIndex::Transform, ComponentBitIndex::Render });
    predrawParams.materialId = predrawMaterial->id;
    predrawParams.meshId = Mesh::ScreenQuad()->meshId;
    auto preDraw = GameObject::New(predrawParams);

    auto [____, predrawTransparentMaterial] = Material::New(MaterialCreateParams{
        .shader = nullptr,
        .depthMask = false,
        .requireUniqueTextureCollection = true,
        .inputProvider = ShaderInputProvider(PreTransparentRendering),
        .depthTestFunc = DepthTestMode::LEqual,
        .blendingSrcFactor = { BlendFactorMode::SrcAlpha, },
        .blendingDstFactor = { BlendFactorMode::OneMinusSrcAlpha, },
        .drawOrder = PREDRAW_TRANSPARENT_DRAW_ORDER,
        });
    predrawTransparentMaterial->abstract = true;
    auto predrawTransparentParams = GameobjectCreateParams({ ComponentBitIndex::Transform, ComponentBitIndex::Render });
    predrawTransparentParams.materialId = predrawTransparentMaterial->id;
    predrawTransparentParams.meshId = Mesh::ScreenQuad()->meshId;
    auto preDrawTransparent = GameObject::New(predrawTransparentParams);

    auto [__, oitCompositorMaterial] = Material::New(MaterialCreateParams{
        .shader = ShaderProgram::New("../shaders/postproc_vertex.glsl", "../shaders/transparent_compositor_frag.glsl", false, false),
        .depthMask = false,
        .requireUniqueTextureCollection = true,
        .inputProvider = ShaderInputProvider(PrepOITComposition),
        .depthTestFunc = DepthTestMode::Disabled,
        .blendingSrcFactor = { BlendFactorMode::SrcAlpha, },
        .blendingDstFactor = { BlendFactorMode::OneMinusSrcAlpha, },
        .drawOrder = TRANSPARENT_COMPOSITION_DRAW_ORDER,
        
    });
    auto oitcParams = GameobjectCreateParams({ ComponentBitIndex::Transform, ComponentBitIndex::Render });
    oitcParams.materialId = oitCompositorMaterial->id;
    oitcParams.meshId = Mesh::ScreenQuad()->meshId;
    auto oitCompositor = GameObject::New(oitcParams);

    auto [___, ppsMaterial] = Material::New(MaterialCreateParams{
        .shader = postProcShader,
        .depthMask = false,
        .requireUniqueTextureCollection = true,
        .inputProvider = ShaderInputProvider(PrepPostprocessing),
        .depthTestFunc = DepthTestMode::Disabled,
        .drawOrder = POSTPROC_DRAW_ORDER,
    });
    auto ppsqParams = GameobjectCreateParams({ ComponentBitIndex::Transform, ComponentBitIndex::Render });
    ppsqParams.meshId = Mesh::ScreenQuad()->meshId;
    ppsqParams.materialId = ppsMaterial->id;
    auto postProcScreenQuad = GameObject::New(ppsqParams);

}