#include "graphics/gengine.hpp"
#include <algorithm>
#include "debug/assert.hpp"
#include <tuple>
#include <execution>
#include <vector>
#include "gameobjects/pointlight_component.hpp"
#include "gameobjects/spotlight_component.hpp"
#include "gameobjects/component_registry.hpp"
#include "GLM/gtx/string_cast.hpp"
#include "gameobjects/animation_component.hpp"
#include "graphics/gengine.hpp"
#include "graphics/mesh.hpp"

#ifdef IS_MODULE
GraphicsEngine* _GRAPHICS_ENGINE_ = nullptr;
void GraphicsEngine::SetModuleGraphicsEngine(GraphicsEngine* eng) {
    _GRAPHICS_ENGINE_ = eng;
}
#endif

GraphicsEngine& GraphicsEngine::Get() {
    #ifdef IS_MODULE
    Assert(_GRAPHICS_ENGINE_ != nullptr);
    return *_GRAPHICS_ENGINE_;
    #else
    static GraphicsEngine engine;
    return engine;
    #endif
}

// thing we send to gpu to tell it about a light
struct PointLightInfo {
    glm::vec4 colorAndRange; // w-coord is range, xyz is rgb
    glm::vec4 relPos; // w-coord is padding; openGL wants everything on a vec4 alignment
};

struct SpotLightInfo {
    glm::vec4 colorAndRange; // w-coord is range, xyz is rgb
    glm::vec4 relPosAndInnerAngle; // w-coord is cos(inner angle); openGL wants everything on a vec4 alignment
    glm::vec4 directionAndOuterAngle; // w-coord is cos(outer angle)
};

// XYZ, UV
const std::vector<GLfloat> screenQuadVertices = {
    -1.0, -1.0, 0.0,   0.0, 0.0,
    -1.0,  1.0, 0.0,   0.0, 1.0,
     1.0, -1.0, 0.0,   1.0, 0.0,
     1.0,  1.0, 0.0,   1.0, 1.0,
};

MeshVertexFormat screenQuadVertexFormat = MeshVertexFormat({
    .position = {{
        .offset = 0,
        .nFloats = 3,
        .instanced = false
    }},
    .textureUV = {{
        .offset = 3 * sizeof(GLfloat),
        .nFloats = 2,
        .instanced = false
    }}
});

const std::vector<GLuint> screenQuadIndices = {
    0, 2, 1,
    1, 2, 3
};

GraphicsEngine::GraphicsEngine():
pointLightDataBuffer(GL_SHADER_STORAGE_BUFFER, 1, (sizeof(PointLightInfo) * 1024) + sizeof(glm::vec4)),
spotLightDataBuffer(GL_SHADER_STORAGE_BUFFER, 1, (sizeof(SpotLightInfo) * 1024) + sizeof(glm::vec4)),
screenQuad(Mesh::New(RawMeshProvider(screenQuadVertices, screenQuadIndices, MeshCreateParams{ .meshVertexFormat = screenQuadVertexFormat, .opacity = 1, .normalizeSize = false}), false))
{

    debugFreecamEnabled = false;
    debugFreecamPitch = 0.0;
    debugFreecamYaw = 0.0;

    

    defaultShaderProgram = ShaderProgram::New("../shaders/world_vertex.glsl", "../shaders/world_fragment.glsl", {}, true, true, false);
    defaultGuiShaderProgram = ShaderProgram::New("../shaders/gui_vertex.glsl", "../shaders/gui_fragment.glsl", {}, false, false, true);
    defaultBillboardGuiShaderProgram = ShaderProgram::New("../shaders/gui_billboard_vertex.glsl", "../shaders/gui_fragment.glsl", {}, true, false, true);
    skyboxShaderProgram = ShaderProgram::New("../shaders/skybox_vertex.glsl", "../shaders/skybox_fragment.glsl");
    postProcessingShaderProgram = ShaderProgram::New("../shaders/postproc_vertex.glsl", "../shaders/postproc_fragment.glsl");
    crummyDebugShader = ShaderProgram::New("../shaders/debug_axis_vertex.glsl", "../shaders/debug_simple_fragment.glsl", {}, false, false, true);

    auto [skyboxBoxmesh, skyboxMat, skyboxTz, _] = Mesh::MultiFromFile("../models/skybox.obj", MeshCreateParams{.textureZ = -1.0, .opacity = 1, .expectedCount = 1, .normalizeSize = false}).at(0);
    skybox = new RenderableMesh(skyboxBoxmesh);
    skyboxMaterial = skyboxMat; // ok if this is nullptr
    skyboxMaterialLayer = skyboxTz;
    // std::cout << "SKYBOX has indices: ";
    // for (auto & v: skybox_boxmesh->indices) {
    //     std::cout << v << ", ";
    // }
    // std::cout << "\n";
    // std::cout << "SKYBOX has vertices: ";
    // for (auto & v: skybox_boxmesh->vertices) {
    //     std::cout << v << ", ";
    // }
    // std::cout << "\n";

    defaultShaderProgram->Uniform("envLightDirection", glm::normalize(glm::vec3(1, 1, 1)));
    defaultShaderProgram->Uniform("envLightColor", glm::vec3(0.7, 0.7, 1));
    defaultShaderProgram->Uniform("envLightDiffuse", 0.2f);
    defaultShaderProgram->Uniform("envLightAmbient", 0.05f);
    defaultShaderProgram->Uniform("envLightSpecular", 0.0f);

    auto pair = Material::New({TextureCreateParams({"../textures/error_texture.bmp"}, Texture::TextureUsage::ColorMap)}, Texture::TextureType::Texture2D);
    errorMaterial = pair.second;
    errorMaterialTextureZ = pair.first;

    glDepthFunc(GL_LEQUAL); // the skybox's z-coord is hardcoded to 1 so it's not drawn over anything, but depth buffer is all 1 by default so this makes skybox able to be drawn

}

GraphicsEngine::~GraphicsEngine() {
    for (auto & [shaderId, map1] : meshpools) {
        for (auto & [materialId, map2] : map1) {
            for (auto & [poolId, pool] : map2) {
                delete pool;
            } 
        } 
    }

    delete skybox;
}

void GraphicsEngine::SetWireframeEnabled(bool)
{
    wireframeDrawing = !wireframeDrawing; // can't directly enable wireframe because that would draw the screen postproc quad with wireframe (bad)
}

Camera& GraphicsEngine::GetCurrentCamera() {
    return debugFreecamEnabled ? debugFreecamCamera : camera; 
}

Camera& GraphicsEngine::GetDebugFreecamCamera() {
    return debugFreecamCamera;
}

Camera& GraphicsEngine::GetMainCamera() {
    return camera;
}

void GraphicsEngine::SetSkyboxShaderProgram(std::shared_ptr<ShaderProgram> program) {
    skyboxShaderProgram = program;
}

void GraphicsEngine::SetSkyboxMaterial(std::shared_ptr<Material> material) {
    skyboxMaterial = material;
}

void GraphicsEngine::SetPostProcessingShaderProgram(std::shared_ptr<ShaderProgram> program) {
    postProcessingShaderProgram = program;
}

void GraphicsEngine::SetDefaultShaderProgram(std::shared_ptr<ShaderProgram> program) {
    defaultShaderProgram = program;
}

void GraphicsEngine::SetDefaultGuiShaderProgram(std::shared_ptr<ShaderProgram> program) {
    defaultGuiShaderProgram = program;
}

void GraphicsEngine::SetDefaultBillboardGuiShaderProgram(std::shared_ptr<ShaderProgram> program) {
    defaultBillboardGuiShaderProgram = program;
}

Window& GraphicsEngine::GetWindow() {
    return window;
}


bool GraphicsEngine::IsTextureInUse(unsigned int textureId) { 
    for (auto & [shaderId, map] : meshpools) {
        if (map.count(textureId)) {
            return true;
        }
    }
    return false;
}

bool GraphicsEngine::IsShaderProgramInUse(unsigned int shaderId) {
    return meshpools.count(shaderId);
}

bool GraphicsEngine::ShouldClose() {
    return window.ShouldClose();
}

void GraphicsEngine::UpdateMainFramebuffer() {
    if (!mainFramebuffer.has_value() || mainFramebuffer->width != window.width || mainFramebuffer->height != window.height) {
        TextureCreateParams colorTextureParams({}, Texture::ColorMap);
        colorTextureParams.filteringBehaviour = Texture::LinearTextureFiltering;
        colorTextureParams.format = Texture::RGBA_16Float;
        colorTextureParams.wrappingBehaviour = Texture::WrapClampToEdge;
        mainFramebuffer.emplace(window.width, window.height, std::vector {colorTextureParams}, true);
    }
}

void GraphicsEngine::DebugAxis() {

    const static std::vector<float> vertices = { 0.0,  0.0,  0.0, 1.0, 0.0, 0.0,
                                   0.2,  0.0,  0.0,    1.0, 0.0, 0.0,
                                  0.0, 0.0, 0.0,      0.0, 1.0, 0.0,
                                  0.0, 0.2, 0.0,  0.0, 1.0, 0.0,
                                  0.0, 0.0, 0.0,   0.0, 0.0, 1.0,
                                  0.0, 0.0, 0.2, 0.0, 0.0, 1.0 };

    /*const static std::vector<unsigned int> indices = { 0, 1, 2, 3, 4, 5 };

    MeshCreateParams params;
    params.meshVertexFormat.emplace(MeshVertexFormat::FormatVertexAttributes{
        .position = VertexAttribute {
            .nFloats = 3,
        },
        .color = VertexAttribute {
            .nFloats = 3
        }
    });
    static auto axisMesh = Mesh::New(RawMeshProvider(vertices, indices, params));
    static RenderableMesh axisRenderer(axisMesh);*/
    crummyDebugShader->Use();
    crummyDebugShader->Uniform("stretch", (float)window.height / (float)window.width);
    // TODO: i literally made the renderablemesh class for this sort of thing, use it
    // xyz, rgb
    
    
    glm::vec3 cpos = GetCurrentCamera().position;
    glm::mat4x4 instancedVertexAttributes = { 1, 0, 0, 0,
                                                    0, 1, 0, 0,
                                                    0, 0, 1, 0,
                                                    cpos.x, cpos.y, cpos.z, 1}; // per object data. format is 4x4 model mat 
    instancedVertexAttributes = glm::identity<glm::mat4x4>();
    // instancedVertexAttributes = (camera.GetProj((float)window.width/(float)window.height));
    glm::mat4x4 cameraMatrix = instancedVertexAttributes * GetCurrentCamera().GetCamera();
    //instancedVertexAttributes = glm::scale(cameraMatrix, glm::vec3(1.0, 1.0, 1.0));
    //instancedVertexAttributes = glm::identity<glm::mat4x4>();
    GLuint vao, vbo, ivbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ivbo);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STREAM_DRAW);    
    glBindBuffer(GL_ARRAY_BUFFER, ivbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::mat4x4), &cameraMatrix, GL_STREAM_DRAW);
 
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    glEnableVertexAttribArray(0); // position
    glEnableVertexAttribArray(1); // color

    glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(glm::vec3) * 2, (void*)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(glm::vec3) * 2, (void*)sizeof(glm::vec3));

    glVertexAttribDivisor(0, 0); // tell openGL that vertex position and color are per-vertex
    glVertexAttribDivisor(1, 0);

    glBindBuffer(GL_ARRAY_BUFFER, ivbo);

    glEnableVertexAttribArray(2); // model matrix (needs 4 vec4 attributes)
    glEnableVertexAttribArray(2+1);
    glEnableVertexAttribArray(2+2);
    glEnableVertexAttribArray(2+3);
    

    glVertexAttribPointer(2, 4, GL_FLOAT, false, sizeof(glm::mat4x4) + sizeof(glm::vec4), (void*)0);
    glVertexAttribPointer(2+1, 4, GL_FLOAT, false, sizeof(glm::mat4x4) + sizeof(glm::vec4), (void*)(sizeof(glm::vec4) * 1));
    glVertexAttribPointer(2+2, 4, GL_FLOAT, false, sizeof(glm::mat4x4) + sizeof(glm::vec4), (void*)(sizeof(glm::vec4) * 2));
    glVertexAttribPointer(2+3, 4, GL_FLOAT, false, sizeof(glm::mat4x4) + sizeof(glm::vec4), (void*)(sizeof(glm::vec4) * 3));
    

    glVertexAttribDivisor(2, 1); // tell openGL that these are instanced vertex attributes (meaning per object, not per vertex)
    glVertexAttribDivisor(2+1, 1);
    glVertexAttribDivisor(2+2, 1);
    glVertexAttribDivisor(2+3, 1);
    

    
    glDrawArraysInstanced(GL_LINES, 0, 6, 1);

    glDeleteBuffers(1, &vbo);  
    glDeleteBuffers(1, &ivbo);  
    
    glDeleteVertexArrays(1, &vao);
}

void GraphicsEngine::RenderScene(float dt) {

    preRenderEvent.Fire(dt);
    BaseEvent::FlushEventQueue(); // we want preRenderEvent to be fired NOW, not later, so if they make objects or whatever it gets rendered this frame.

    // glEnable(GL_FRAMEBUFFER_SRGB); // gamma correction; google it. TODO: when we start using postprocessing/framebuffers, turn this off except for final image output

    // std::cout << "\tAdding cached meshes.\n";

    // Update various things

    // std::cout << "\tAdding cached meshes.\n";
    AddCachedMeshes();
    // std::cout << "\tUpdating RCs.\n";
    UpdateRenderComponents(dt);    
  
    
    // std::cout << "\tUpdating lights.\n";
    UpdateLights();

    UpdateMeshpools(); // NOTE: this does need to be at the end or the beginning, not the middle, i forget why
    pointLightDataBuffer.Update();
    spotLightDataBuffer.Update();

    //glFinish();
    //CalculateLightingClusters();

    if (debugFreecamEnabled) {
        UpdateDebugFreecam();
    }
    

    // std::cout << "\tSetting uniforms.\n";
    // Update shaders with the camera's new rotation/projection
    glm::mat4x4 cameraMatrix = GetCurrentCamera().GetCamera();
    glm::mat4x4 cameraMatrixNoFloatingOrigin = glm::translate(cameraMatrix, (debugFreecamEnabled) ? (glm::vec3) -debugFreecamCamera.position : (glm::vec3) -GetCurrentCamera().position);
    glm::mat4x4 projectionMatrix = camera.GetProj((float)window.width/(float)window.height); 
    ShaderProgram::SetCameraUniforms(projectionMatrix * cameraMatrix, projectionMatrix * cameraMatrixNoFloatingOrigin, glm::ortho(0.0f, float(window.width), 0.0f, float(window.height), -1.0f, 1.0f));

    // Draw the world onto a framebuffer so we can draw the contents of that framebuffer onto the screen with a postprocessing shader.
    UpdateMainFramebuffer();
    mainFramebuffer->Bind();

    

    // tell opengl how to do transparency
    glEnable(GL_BLEND); 
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 

    

    DrawWorld(true);
    DrawSkybox(); // Draw skybox afterwards to encourage early z-test
    

    // Go back to drawing on the window.
    Framebuffer::Unbind();
    glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    // Draw contents of main framebuffer on screen quad, using the postprocessing shader.
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // obviously the screen quad should not be drawn with wireframe

    postProcessingShaderProgram->Use();
    mainFramebuffer->textureAttachments.at(0).Use();
    screenQuad.Draw();

    DrawWorld(false);


    // Debugging stuff
    // TODO: actual settings to toggle debug stuff
     //SpatialAccelerationStructure::Get().DebugVisualize();
    DebugAxis();

    

    glFlush(); // Tell OpenGL we're done drawing.

    postRenderEvent.Fire(dt);
    BaseEvent::FlushEventQueue();
}

void GraphicsEngine::DrawSkybox() {
    if (skyboxShaderProgram == nullptr || skyboxMaterial == nullptr) {return;} // make sure there is a skybox
    glDisable(GL_CULL_FACE);
    // glDisable(GL_DEPTH_TEST);
    skyboxShaderProgram->Use();
    skyboxMaterial->Use();
    skybox->Draw();
    
    glClear(GL_DEPTH_BUFFER_BIT); // make sure skybox isn't drawn over everything else
}

void GraphicsEngine::UpdateLights() {

    glm::dvec3 cameraPos = GetCurrentCamera().position;

    // Point lights
    // Get components of all gameobjects that have a transform and point light component
    auto pcomponents = ComponentRegistry::Get().GetSystemComponents<PointLightComponent, TransformComponent>();

    // set properties of point lights on gpu
    const unsigned int POINT_LIGHT_OFFSET = sizeof(glm::vec4); // although the first value is just one uint (# of lights), we need vec4 alignment so yeah
    unsigned int pointLightCount = 0;
    unsigned int i = 0;

    for (auto & tuple : pcomponents) {

        PointLightComponent& pointLight = *std::get<0>(tuple);
        TransformComponent& transform = *std::get<1>(tuple);

        if (pointLight.live) {
            glm::vec3 relCamPos = transform.Position() - cameraPos;
            DebugLogInfo("rel pos = ", glm::to_string(relCamPos));
            auto info = PointLightInfo {
                .colorAndRange = glm::vec4(pointLight.Color().x, pointLight.Color().y, pointLight.Color().z, pointLight.Range()),
                .relPos = glm::vec4(relCamPos.x, relCamPos.y, relCamPos.z, 0)
            };
            //std::printf("byte offset %llu\n", POINT_LIGHT_OFFSET + (lightCount * sizeof(PointLightInfo)));
            (*(PointLightInfo*)(pointLightDataBuffer.Data() + POINT_LIGHT_OFFSET + (i * sizeof(PointLightInfo)))) = info;

            pointLightCount++;
        }
        i++;
    }

    // say how many point lights there are
    *(GLuint*)(pointLightDataBuffer.Data()) = pointLightCount;

    // Spot lights
    // Get components of all gameobjects that have a transform and point light component
    auto scomponents = ComponentRegistry::Get().GetSystemComponents<SpotLightComponent, TransformComponent>();

    // set properties of point lights on gpu
    const unsigned int SPOT_LIGHT_OFFSET = sizeof(glm::vec4); // although the first value is just one uint (# of lights), we need vec4 alignment so yeah
    unsigned int spotLightCount = 0;
    i = 0;

    for (auto& tuple : scomponents) {

        SpotLightComponent& spotLight = *std::get<0>(tuple);
        TransformComponent& transform = *std::get<1>(tuple);

        if (spotLight.live) {
            glm::vec3 relCamPos = transform.Position() - cameraPos;
            // std::printf("rel pos = %f %f %f %f\n", relCamPos.x, relCamPos.y, relCamPos.z, pointLight.Range());
            glm::vec3 spotlightDirection = transform.Rotation() * glm::vec3(0, 0, 1);
            auto info = SpotLightInfo{
                .colorAndRange = glm::vec4(spotLight.Color().x, spotLight.Color().y, spotLight.Color().z, spotLight.Range()),
                .relPosAndInnerAngle = glm::vec4(relCamPos.x, relCamPos.y, relCamPos.z, cos(glm::radians(spotLight.InnerAngle()))),
                .directionAndOuterAngle = glm::vec4(spotlightDirection, cos(glm::radians(spotLight.OuterAngle()))),
            };
            //std::printf("byte offset %llu\n", POINT_LIGHT_OFFSET + (lightCount * sizeof(PointLightInfo)));
            (*(SpotLightInfo*)(spotLightDataBuffer.Data() + SPOT_LIGHT_OFFSET + (i * sizeof(SpotLightInfo)))) = info;

            spotLightCount++;
        }
        i++;
    }

    // say how many spot lights there are
    *(GLuint*)(spotLightDataBuffer.Data()) = spotLightCount;
}

// void GraphicsEngine::SetColor(const RenderComponent& comp, const glm::vec4& rgba) {
//     Assert(comp.meshpoolId != -1);
//     meshpools[comp.shaderProgramId][comp.materialId][comp.meshpoolId]->SetColor(comp.meshpoolSlot, comp.meshpoolInstance, rgba);
// }

void GraphicsEngine::SetModelMatrix(const RenderComponent& comp, const glm::mat4x4& model) {
    Assert(comp.meshpoolId != -1);
    meshpools[comp.shaderProgramId][comp.materialId][comp.meshpoolId]->SetModelMatrix(comp.meshpoolSlot, comp.meshpoolInstance, model);
}

void GraphicsEngine::SetNormalMatrix(const RenderComponent& comp, const glm::mat3x3& normal) {
    Assert(comp.meshpoolId != -1);
    meshpools[comp.shaderProgramId][comp.materialId][comp.meshpoolId]->SetNormalMatrix(comp.meshpoolSlot, comp.meshpoolInstance, normal);
}

void GraphicsEngine::SetBoneState(const RenderComponent& comp, unsigned int nBones, glm::mat4x4* offsets) {
    Assert(comp.meshpoolId != -1);
    meshpools[comp.shaderProgramId][comp.materialId][comp.meshpoolId]->SetBoneState(comp.meshpoolSlot, comp.meshpoolInstance, nBones, offsets);
}

// void GraphicsEngine::SetTextureZ(const RenderComponent& comp, const float textureZ) {
//     Assert(comp.meshpoolId != -1);
//     meshpools[comp.shaderProgramId][comp.materialId][comp.meshpoolId]->SetTextureZ(comp.meshpoolSlot, comp.meshpoolInstance, textureZ);
// }

// void GraphicsEngine::SetArbitrary1(const RenderComponent& comp, const float arb) {
//     Assert(comp.meshpoolId != -1);
//     meshpools[comp.shaderProgramId][comp.materialId][comp.meshpoolId]->SetArbitrary1(comp.meshpoolSlot, comp.meshpoolInstance, arb);
// }

void GraphicsEngine::UpdateMeshpools() {
    for (auto & [shaderId, map1] : meshpools) {
        for (auto & [textureId, map2] : map1) {
            for (auto & [poolId, pool] : map2) {
                pool->Update();
            } 
        } 
    }
}

void GraphicsEngine::DrawWorld(bool postProc)
{
    glClear(postProc ? GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT : GL_DEPTH_BUFFER_BIT); // clear screen, but if we've already drawn stuff then only clear depth buffer

    glEnable(GL_DEPTH_TEST); // stuff near the camera should be drawn over stuff far from the camera
    glEnable(GL_CULL_FACE); // backface culling

    if (wireframeDrawing) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // Draw world stuff.
    for (auto& [shaderId, map1] : meshpools) {
        auto& shader = ShaderProgram::Get(shaderId);
        if (shader->ignorePostProc == postProc) {
            continue;
        }
        shader->Use();
        pointLightDataBuffer.BindBase(0);
        spotLightDataBuffer.BindBase(1);
        for (auto& [materialId, map2] : map1) {
            if (materialId == 0) { // 0 means no material
                Material::Unbind();

                shader->Uniform("specularMappingEnabled", false);
                shader->Uniform("fontMappingEnabled", false);
                shader->Uniform("normalMappingEnabled", false);
                shader->Uniform("parallaxMappingEnabled", false);
                shader->Uniform("colorMappingEnabled", false);
            }
            else {

                auto& material = Material::Get(materialId);
                material->Use();

                // if (materialId == 4) {
                //     std::cout << "BINDING THING WITH FONTMAP.\n";
                // }

                shader->Uniform("specularMappingEnabled", material->HasSpecularMap());
                shader->Uniform("fontMappingEnabled", material->HasFontMap());
                shader->Uniform("normalMappingEnabled", material->HasNormalMap());
                shader->Uniform("parallaxMappingEnabled", material->HasDisplacementMap());
                shader->Uniform("colorMappingEnabled", material->HasColorMap());
            }

            for (auto& [poolId, pool] : map2) {
                pool->Draw();
            }
        }
    }
}

// recursive function used in processing of animation components in UpdateRenderComponents()
void HandleBone(const Animation& anim, const Bone& bone, unsigned int count, glm::mat4x4* boneTransforms, float* transformPriorities) {

}

// w/ simple for loop: 8ms
// w/ parallelized std::for_each: about 9ms??? unclear if it was actually parallel

// TODO: idk if there's any practical way around this but data locality is probably messed up by jumping around inside the meshpool vbos
void GraphicsEngine::UpdateRenderComponents(float dt) {
    // auto start = Time();
    
    auto cameraPos = GetCurrentCamera().position;


    std::vector<unsigned int> indicesToRemove;

    // SETTING INSTANCED VERTEX ATTRIBUTES (color, textureZ, etc.)
        // but not model/normal matrix; those are done in the next part bc every render component gets a different one, every frame.
        // TODO: unless floating origin in which case this system would be good for them
    // vec4
    unsigned int i = 0;
    for (auto & [renderComp, attributeName, value, timesRemainingToUpdate]: Instanced4ComponentVertexAttributeUpdates) {
        
        if (renderComp->meshpoolId != -1) { // we can't set these values until the render component gets a mesh pool
            meshpools[renderComp->shaderProgramId][renderComp->materialId][renderComp->meshpoolId]->SetInstancedVertexAttribute<4>(renderComp->meshpoolSlot, renderComp->meshpoolInstance, attributeName, value);
            timesRemainingToUpdate -= 1;
            if (timesRemainingToUpdate == 0) {
                indicesToRemove.push_back(i);
            }
            i++;
        }
        
        
    }

    for (auto it = indicesToRemove.rbegin(); it != indicesToRemove.rend(); it++) {
        Instanced4ComponentVertexAttributeUpdates[*it] = Instanced4ComponentVertexAttributeUpdates.back();
        Instanced4ComponentVertexAttributeUpdates.pop_back();
    }  
    indicesToRemove.clear();

    // vec3
    i = 0;
    for (auto & [renderComp, attributeName, value, timesRemainingToUpdate]: Instanced3ComponentVertexAttributeUpdates) {
        if (renderComp->meshpoolId != -1) { // we can't set these values until the render component gets a mesh pool
            meshpools[renderComp->shaderProgramId][renderComp->materialId][renderComp->meshpoolId]->SetInstancedVertexAttribute<3>(renderComp->meshpoolSlot, renderComp->meshpoolInstance, attributeName, value);
            timesRemainingToUpdate -= 1;
            if (timesRemainingToUpdate == 0) {
                indicesToRemove.push_back(i);
            }
        }
        
        

        i++;
    }

    for (auto it = indicesToRemove.rbegin(); it != indicesToRemove.rend(); it++) {
        Instanced3ComponentVertexAttributeUpdates[*it] = Instanced3ComponentVertexAttributeUpdates.back();
        Instanced3ComponentVertexAttributeUpdates.pop_back();
    }  
    indicesToRemove.clear();

    // vec2
    i = 0;
    for (auto & [renderComp, attributeName, value, timesRemainingToUpdate]: Instanced2ComponentVertexAttributeUpdates) {
        if (renderComp->meshpoolId != -1) { // we can't set these values until the render component gets a mesh pool
            meshpools[renderComp->shaderProgramId][renderComp->materialId][renderComp->meshpoolId]->SetInstancedVertexAttribute<2>(renderComp->meshpoolSlot, renderComp->meshpoolInstance, attributeName, value);
            timesRemainingToUpdate -= 1;
            if (timesRemainingToUpdate == 0) {
                indicesToRemove.push_back(i);
            }
        }
        

        i++;
    }

    for (auto it = indicesToRemove.rbegin(); it != indicesToRemove.rend(); it++) {
        Instanced2ComponentVertexAttributeUpdates[*it] = Instanced2ComponentVertexAttributeUpdates.back();
        Instanced2ComponentVertexAttributeUpdates.pop_back();
    }  
    indicesToRemove.clear();

    // vec1
    i = 0;
    for (auto & [renderComp, attributeName, value, timesRemainingToUpdate]: Instanced1ComponentVertexAttributeUpdates) {
        if (renderComp->meshpoolId != -1) { // we can't set these values until the render component gets a mesh pool
            meshpools[renderComp->shaderProgramId][renderComp->materialId][renderComp->meshpoolId]->SetInstancedVertexAttribute<1>(renderComp->meshpoolSlot, renderComp->meshpoolInstance, attributeName, value);
            timesRemainingToUpdate -= 1;
            if (timesRemainingToUpdate == 0) {
                indicesToRemove.push_back(i);
            }
        }
        

        i++;
    }

    for (auto it = indicesToRemove.rbegin(); it != indicesToRemove.rend(); it++) {
        Instanced1ComponentVertexAttributeUpdates[*it] = Instanced1ComponentVertexAttributeUpdates.back();
        Instanced1ComponentVertexAttributeUpdates.pop_back();
    }  
    
    // Get components of all gameobjects that have a transform and render component
    auto components = ComponentRegistry::Get().GetSystemComponents<RenderComponent, TransformComponent>();
    for (auto& [rc, tc] : components) {
    //std::for_each(std::execution::par, components.begin(), components.end(), [this, &cameraPos](std::tuple<RenderComponent*, TransformComponent*>& tuple) {
        //auto & renderComp = *std::get<0>(tuple);
        //auto & transformComp = *std::get<1>(tuple);
        auto& renderComp = *rc;
        auto& transformComp = *tc;
        if (renderComp.live) { 
            // std::cout << "Component at " << &renderComp << " is live \n";
            // if (renderComp.componentPoolId != i) {
            //     //std::cout << "Warning: comp at " << renderComp << " has id " << renderComp->componentPoolId << ", i=" << i << ". ABORT\n";
            //     abort();
            // }
            // TODO: we only need to call SetNormalMatrix() when the object is rotated
            
            // tell shaders where the object is, rot/scl
            SetNormalMatrix(renderComp, transformComp.GetNormalMatrix()); 
            SetModelMatrix(renderComp, transformComp.GetGraphicsModelMatrix(cameraPos));

            
        }
    };

    // Get components of all gameobjects that have a transform and no floating origin render component
    auto components2 = ComponentRegistry::Get().GetSystemComponents<RenderComponentNoFO, TransformComponent>();
    for (auto& [rcnfo, tc] : components2) {
        auto& renderCompNoFO = *rcnfo;
        auto & transformComp = *tc;
        if (renderCompNoFO.live) {
            
            //std::cout << "Component " << j <<  " at " << renderComp << " is live \n";
            // if (renderComp.componentPoolId != i) {
            //     //std::cout << "Warning: comp at " << renderComp << " has id " << renderComp->componentPoolId << ", i=" << i << ". ABORT\n";
            //     abort();
            // }
            // TODO: we only need to call SetNormalMatrix() when the object is rotated
            SetNormalMatrix(renderCompNoFO, transformComp.GetNormalMatrix()); 
            SetModelMatrix(renderCompNoFO, transformComp.GetGraphicsModelMatrix({0, 0, 0}));
            
            // if (renderCompNoFO.textureZChanged > 0) {renderCompNoFO.textureZChanged -= 1; SetTextureZ(renderCompNoFO, renderCompNoFO.textureZ);}
            // if (renderCompNoFO.colorChanged > 0) {renderCompNoFO.colorChanged -= 1; SetColor(renderCompNoFO, renderCompNoFO.color);}
        }
    }
    //std::for_each(std::execution::, components2.begin(), components2.end(), [this](std::tuple<RenderComponentNoFO*, TransformComponent*>& tuple) {
    //    auto & renderCompNoFO = *std::get<0>(tuple);
    //    auto & transformComp = *std::get<1>(tuple);
    //    if (renderCompNoFO.live) {
    //        
    //        //std::cout << "Component " << j <<  " at " << renderComp << " is live \n";
    //        // if (renderComp.componentPoolId != i) {
    //        //     //std::cout << "Warning: comp at " << renderComp << " has id " << renderComp->componentPoolId << ", i=" << i << ". ABORT\n";
    //        //     abort();
    //        // }
    //        // TODO: we only need to call SetNormalMatrix() when the object is rotated
    //        SetNormalMatrix(renderCompNoFO, transformComp.GetNormalMatrix()); 
    //        SetModelMatrix(renderCompNoFO, transformComp.GetGraphicsModelMatrix({0, 0, 0}));
    //        
    //        // if (renderCompNoFO.textureZChanged > 0) {renderCompNoFO.textureZChanged -= 1; SetTextureZ(renderCompNoFO, renderCompNoFO.textureZ);}
    //        // if (renderCompNoFO.colorChanged > 0) {renderCompNoFO.colorChanged -= 1; SetColor(renderCompNoFO, renderCompNoFO.color);}
    //    }
    //});

    // animations
    auto components3 = ComponentRegistry::Get().GetSystemComponents<AnimationComponent>();
    for (auto & [animComp]: components3) {
        if (!animComp->live) {continue;}

    //     // TODO: this will redundantly send identity matrices for every bone when something isn't getting animated. Difficult to fix bc triple buffering, prob not big deal anyways.
    //     // TOOD: FRUSTRUM culling would actually be big brain here.

        Assert(animComp->mesh->bones.has_value());
        std::vector<glm::mat4x4> boneOffsets;
        std::vector<float> bonePriorities;
        boneOffsets.resize(animComp->mesh->bones->size());
        bonePriorities.resize(animComp->mesh->bones->size()); // priority of the animation controlling this bone, so we know which animations override which
        for (unsigned int index = 0; index < animComp->mesh->bones->size(); index++) {
            boneOffsets[index] = glm::identity<glm::mat4x4>();
            bonePriorities[index] = -INFINITY;
        }

        for (unsigned int index = 0; index < animComp->currentlyPlaying.size(); index++) {
            auto & animation = animComp->currentlyPlaying.at(index);
            
            // DebugLogInfo("Pos = ", animation.playbackPosition, " len ", animation.anim->duration, " looped ", animation.looped);
            animation.playbackPosition += dt;
            while (animation.looped && animation.playbackPosition > animation.anim->duration) { // loop animations
                animation.playbackPosition -= animation.anim->duration;
            }
            
            if ((animation.playbackPosition < animation.anim->duration) || (animation.looped == true)) { // then the animation is still playing.
                

                for (auto & bone: animComp->mesh->bones.value()) {
                    Assert(bone.id < animComp->mesh->bones->size());
                    if (animation.anim->priority > bonePriorities[bone.id]) {
                        Assert(animation.playbackPosition < animation.anim->duration);
                        // DebugLogInfo("At bone ", bone.id, " placing ", glm::to_string(animation.anim->BoneTransformAtTime(bone.id, animation.playbackPosition)));

                        auto transform = animation.anim->BoneTransformAtTime(bone.id, animation.playbackPosition);
                        if (transform.has_value()) {
                            boneOffsets[bone.id] = animation.anim->BoneTransformAtTime(bone.id, animation.playbackPosition).value(); //todo: interpolating with looped
                        }
                        
                    }
                }
            }
            else { // then the animation is done.
                animComp->currentlyPlaying.erase(animComp->currentlyPlaying.begin() + index);
            }
        }

        //for (auto& offset : boneOffsets) {
        //DebugLogInfo("Putting bone at ", glm::to_string(boneOffsets.at(0)));
        //}
        SetBoneState(*animComp->renderComponent, animComp->mesh->bones->size(), boneOffsets.data());
    }

    

    // LogElapsed(start, "Rendercomp update elapsed ");
}

void GraphicsEngine::SetDebugFreecamAcceleration(double acceleration) {
    debugFreecamAcceleration = acceleration;
}

void GraphicsEngine::SetDebugFreecamPitch(double pitch) {
    debugFreecamPitch = pitch;
}

void GraphicsEngine::SetDebugFreecamYaw(double yaw) {
    debugFreecamYaw = yaw;
}

void GraphicsEngine::SetDebugFreecamEnabled(bool enabled) {
    debugFreecamEnabled = enabled;
}

void GraphicsEngine::UpdateDebugFreecam() {
    Assert(debugFreecamEnabled);

    // camera acceleration
    if (window.PRESSED_KEYS.contains(InputObject::W) || window.PRESSED_KEYS.contains(InputObject::S) || window.PRESSED_KEYS.contains(InputObject::A) || window.PRESSED_KEYS.contains(InputObject::D) || window.PRESSED_KEYS.contains(InputObject::Q) || window.PRESSED_KEYS.contains(InputObject::E)) {
        debugFreecamSpeed += debugFreecamAcceleration;
    }
    else {
        debugFreecamSpeed = debugFreecamAcceleration;
    }

    // camera rotation
    debugFreecamPitch += 0.5 * window.MOUSE_DELTA.y;
    debugFreecamYaw += 0.5 * window.MOUSE_DELTA.x;
    if (debugFreecamYaw > 180) {
        debugFreecamYaw -= 360;
    } else if (debugFreecamYaw < -180) {
        debugFreecamYaw += 360;
    } 
    debugFreecamPitch = std::max(-90.0, std::min(debugFreecamPitch, 90.0));

    glm::dquat rotation = glm::rotate(glm::rotate(glm::identity<glm::dmat4x4>(), glm::radians(debugFreecamPitch), glm::dvec3(1, 0, 0)), glm::radians(debugFreecamYaw), glm::dvec3(0, 1, 0));
    debugFreecamCamera.rotation = rotation;

    // camera movement
    auto look = LookVector(glm::radians(debugFreecamPitch), glm::radians(debugFreecamYaw));
    auto right = LookVector(0, glm::radians(debugFreecamYaw + 90));
    auto up = glm::cross(look, right);
    glm::dvec3 rightMovement = right * debugFreecamSpeed * (double)(window.PRESSED_KEYS.contains(InputObject::D) - window.PRESSED_KEYS.contains(InputObject::A));
    glm::dvec3 upMovement = up * debugFreecamSpeed * (double)(window.PRESSED_KEYS.contains(InputObject::Q) - window.PRESSED_KEYS.contains(InputObject::E));
    glm::dvec3 forwardMovement = look * debugFreecamSpeed * (double)(window.PRESSED_KEYS.contains(InputObject::W) - window.PRESSED_KEYS.contains(InputObject::S));
    debugFreecamCamera.position += rightMovement + forwardMovement + upMovement;
}

void GraphicsEngine::AddCachedMeshes() {
    if (renderComponentsToAdd.size() > 0) {
        // std::cout << "There are " << renderComponentsToAdd.size() << " to add.\n";
    }
    
    for (auto & [shaderId, map1] : renderComponentsToAdd) {
        for (auto & [materialId, map2] : map1) {
            for (auto & [meshId, components] : map2) {

                
                // std::cout << "\tInfo: " << meshId << " " << textureId << " " << shaderId << " size " << renderComponentsToAdd.size() << "\n";

                std::shared_ptr<Mesh>& m = Mesh::Get(meshId);
                const unsigned int verticesNBytes = m->vertices.size() * sizeof(GLfloat);
                const unsigned int indicesNBytes = m->indices.size() * sizeof(GLuint);

                // pick best pool for mesh
                // TODO: O(n) complexity might be an issue
                int bestPoolScore = INT_MAX;
                int bestPoolId = -1;
                for (auto & [poolId, pool] : meshpools[shaderId][materialId]) {
                    int score = pool->ScoreMeshFit(verticesNBytes, indicesNBytes, m->vertexFormat);
                    if (score == -1) {continue;} // this continues the inner loop which is what we want
                    if (score < bestPoolScore) {
                        bestPoolScore = score;
                        bestPoolId = poolId;
                    }
                }

                if (bestPoolId == -1) { // if we didn't find a suitable pool just make one
                    // DebugLogInfo("must create pool for new mesh. ", " size ", m->vertices.size() * m->vertexFormat.GetNonInstancedVertexSize()/sizeof(GLfloat));
                    bestPoolId = lastPoolId++;

                    int nVertices = m->vertices.size() * sizeof(GLfloat) / m->vertexFormat.GetNonInstancedVertexSize();
                    meshpools[shaderId][materialId][bestPoolId] = new Meshpool(m->vertexFormat, nVertices == 0 ? 1 : nVertices); // make the meshpool have a minimum of 1 vertex to avoid problems with empty meshes. 
                }
                // std::cout << "\tmesh pool id is " << bestPoolId << "\n"; 

                auto objectPositions = meshpools.at(shaderId).at(materialId).at(bestPoolId)->AddObject(meshId, components.size());
                // std::cout << "\tAdded.\n";
                for (unsigned int i = 0; i < components.size(); i++) { // todo: use iterator here???
                    components.at(i)->meshpoolId = bestPoolId;
                    components[i]->meshpoolSlot = objectPositions[i].first;
                    components[i]->meshpoolInstance = objectPositions[i].second;
                    //std::cout  << "Initalized mesh location " << (meshLocations[i]) << ".\n";

                    if (m->dynamic) {
                        dynamicMeshUsers[meshId].push_back(components[i]);
                    }
                }

                
            } 
        }
    }
    // TODO: instead of clearing the entire thing do something else
    // TODO: why did i write the above comment? clearing is fine???
    renderComponentsToAdd.clear();
}

void GraphicsEngine::AddObject(unsigned int shaderId, unsigned int materialId, unsigned int meshId, RenderComponent* component) {
    renderComponentsToAdd[shaderId][materialId][meshId].push_back(component);
}

