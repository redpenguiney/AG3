#include "basic_renderer.hpp"
#include <gameobjects/gameobject.hpp>



BasicRenderer& BasicRenderer::Setup(std::shared_ptr<ShaderProgram> postProcShader) {
    static BasicRenderer renderer(postProcShader);
    return renderer;
}

// XYZ, UV
const std::vector<GLfloat> screenQuadVertices = {
    -1.0, -1.0, 0.0,   0.0, 0.0,
    -1.0,  1.0, 0.0,   0.0, 1.0,
     1.0, -1.0, 0.0,   1.0, 0.0,
     1.0,  1.0, 0.0,   1.0, 1.0,
};

MeshVertexFormat screenQuadVertexFormat = MeshVertexFormat({
    .position = {{
        .nFloats = 3,
        .instanced = false
    }},
    .textureUV = {{
        .nFloats = 2,
        .instanced = false
    }},
    .modelMatrix = {{
        .nFloats = 16,
        .instanced = true
    }},
    .normalMatrix = {{
        .nFloats = 9,
        .instanced = true
    }}
});

const std::vector<GLuint> screenQuadIndices = {
    0, 2, 1,
    1, 2, 3
};

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

static void PrepPostprocessing(Material* material, std::shared_ptr<ShaderProgram> _) {
    auto& BE = BasicRenderer::Setup();

    UpdateFramebuffer();
    
    // clearing for purposes of the next frame

    //BE.mainFramebuffer->Clear({ {0, 0, 0, 0 }, {0, 0, 0, 0}, { 1, 1, 1, 1 } });
    BE.mainFramebuffer->Unbind();
    BE.mainFramebuffer->textureAttachments[0].Use();

    DebugLogInfo("POSTPROC");
}


static void PrepOITComposition(Material* material, std::shared_ptr<ShaderProgram> _) {
    auto& BE = BasicRenderer::Setup();

    UpdateFramebuffer();

    BE.mainFramebuffer->Bind();
    BE.mainFramebuffer->textureAttachments[1].Use();
    BE.mainFramebuffer->textureAttachments[2].Use();

    DebugLogInfo("OIT COMPOSITION.");
}

static void PrepNormalRendering(Material* material, std::shared_ptr<ShaderProgram> _) {
    auto& BE = BasicRenderer::Setup();

    UpdateFramebuffer();
    BE.mainFramebuffer->Bind();

    DebugLogInfo("Normal mat.");
    
}

BasicRenderer::BasicRenderer(std::shared_ptr<ShaderProgram> postProcShader) {
    if (!postProcShader)
        postProcShader = ShaderProgram::New("../shaders/postproc_vertex.glsl", "../shaders/postproc_fragment.glsl", false, false);

    auto screenQuadMesh = Mesh::New(RawMeshProvider(screenQuadVertices, screenQuadIndices, MeshCreateParams{ .meshVertexFormat = screenQuadVertexFormat, .expectedCount = 2, .normalizeSize = false }));

    GraphicsEngine::Get().defaultMaterial->inputProvider = PrepNormalRendering;

    auto [_, oitCompositorMaterial] = Material::New(MaterialCreateParams{
        .shader = ShaderProgram::New("../shaders/postproc_vertex.glsl", "../shaders/transparent_compositor_frag.glsl", false, false),
        .depthMask = false,
        .requireUniqueTextureCollection = true,
        .inputProvider = ShaderInputProvider(PrepOITComposition),
        .depthTestFunc = DepthTestMode::Disabled,
        .blendingSrcFactor = { BlendFactorMode::SrcAlpha, }, // 2 other attachment handling???
        .blendingDstFactor = { BlendFactorMode::OneMinusSrcAlpha, },
        .drawOrder = TRANSPARENT_COMPOSITION_DRAW_ORDER,
        
    });
    auto oitcParams = GameobjectCreateParams({ ComponentBitIndex::Transform, ComponentBitIndex::Render });
    oitcParams.materialId = oitCompositorMaterial->id;
    oitcParams.meshId = screenQuadMesh->meshId;
    auto oitCompositor = GameObject::New(oitcParams);

    auto [__, ppsMaterial] = Material::New(MaterialCreateParams{
        .shader = postProcShader,
        .depthMask = false,
        .requireUniqueTextureCollection = true,
        .inputProvider = ShaderInputProvider(PrepPostprocessing),
        .depthTestFunc = DepthTestMode::Disabled,
        .drawOrder = POSTPROC_DRAW_ORDER,
    });
    auto ppsqParams = GameobjectCreateParams({ ComponentBitIndex::Transform, ComponentBitIndex::Render });
    ppsqParams.meshId = screenQuadMesh->meshId;
    ppsqParams.materialId = ppsMaterial->id;
    auto postProcScreenQuad = GameObject::New(ppsqParams);

}