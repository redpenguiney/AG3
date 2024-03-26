#include "engine.hpp"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <pstl/glue_execution_defs.h>
#include <tuple>
#include <execution>
#include <vector>
#include "../gameobjects/pointlight_component.hpp"
#include "../gameobjects/component_registry.hpp"

GraphicsEngine& GraphicsEngine::Get() {
    static GraphicsEngine engine;
    return engine;
}

// thing we send to gpu to tell it about a light
struct PointLightInfo {
    glm::vec4 colorAndRange; // w-coord is range, xyz is rgb
    glm::vec4 relPos; // w-coord is padding; openGL wants everything on a vec4 alignment
};

// XYZ, UV
const std::vector<GLfloat> screenQuadVertices = {
    -1.0, -1.0, 0.0,   0.0, 0.0,
    -1.0,  1.0, 0.0,   0.0, 1.0,
     1.0, -1.0, 0.0,   1.0, 0.0,
     1.0,  1.0, 0.0,   1.0, 1.0,
};

constexpr MeshVertexFormat screenQuadVertexFormat = MeshVertexFormat {
    .attributes = {
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
    }
};

const std::vector<GLuint> screenQuadIndices = {
    0, 1, 2,
    1, 2, 3
};

GraphicsEngine::GraphicsEngine(): 
pointLightDataBuffer(GL_SHADER_STORAGE_BUFFER, 1, (sizeof(PointLightInfo) * 1024) + sizeof(glm::vec3)),
screenQuad(Mesh::FromVertices(screenQuadVertices, screenQuadIndices, false, screenQuadVertexFormat, 1, false))
{
    debugFreecamEnabled = false;
    debugFreecamPos = {0.0, 0.0, 0.0};
    debugFreecamPitch = 0.0;
    debugFreecamYaw = 0.0;

    skyboxMaterial = nullptr;

    defaultShaderProgram = ShaderProgram::New("../shaders/world_vertex.glsl", "../shaders/world_fragment.glsl");
    defaultGuiShaderProgram = ShaderProgram::New("../shaders/gui_vertex.glsl", "../shaders/gui_fragment.glsl", {}, false);
    skyboxShaderProgram = ShaderProgram::New("../shaders/skybox_vertex.glsl", "../shaders/skybox_fragment.glsl");
    postProcessingShaderProgram = ShaderProgram::New("../shaders/postproc_vertex.glsl", "../shaders/postproc_fragment.glsl");

    auto skybox_boxmesh = Mesh::FromFile("../models/skybox.obj", MeshVertexFormat::Default(), -1.0, 1.0, 1, false);
    skybox = new RenderableMesh(skybox_boxmesh);
    // std::cout << "SKYBOX has indices: ";
    // for (auto & v: skybox_boxmesh->indices) {
    //     std::cout << v << ", ";
    // }
    // std::cout << "\n";
    // std::cout << "SKYBOX has vertices: ";
    // for (auto & v: skybox_boxmesh->vertices) {
    //     std::cout << v << ", ";
    // }
    std::cout << "\n";
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
    std::cout << "Deleted.\n";
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
    static auto crummyDebugShader =  ShaderProgram::New("../shaders/debug_axis_vertex.glsl", "../shaders/debug_simple_fragment.glsl", {}, false, true, false);
    crummyDebugShader->Use();
    // TODO: i literally made the renderablemesh class for this sort of thing, use it
    // xyz, rgb
    const static std::vector<float> vertices = {0.0,  0.0,  0.0, 1.0, 0.0, 0.0,
                                   0.2,  0.0,  0.0,    1.0, 0.0, 0.0,
                                  0.0, 0.0, 0.0,      0.0, 1.0, 0.0,
                                  0.0, 0.2, 0.0,  0.0, 1.0, 0.0,
                                  0.0, 0.0, 0.0,   0.0, 0.0, 1.0,
                                  0.0, 0.0, 0.2, 0.0, 0.0, 1.0};
    
    glm::vec3 cpos = debugFreecamEnabled ? debugFreecamPos: camera.position;
    glm::mat4x4 instancedVertexAttributes = {1, 0, 0, 0, 
                                                    0, 1, 0, 0,
                                                    0, 0, 1, 0,
                                                    cpos.x, cpos.y, cpos.z, 1}; // per object data. format is 4x4 model mat 
    // instancedVertexAttributes = (camera.GetProj((float)window.width/(float)window.height));
    glm::mat4x4 cameraMatrix = (debugFreecamEnabled) ? UpdateDebugFreecam() : camera.GetCamera();
    instancedVertexAttributes = glm::translate(cameraMatrix, (debugFreecamEnabled) ? (glm::vec3) -debugFreecamPos : (glm::vec3) -camera.position);
    GLuint vao, vbo, ivbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ivbo);

    // auto frCam = glm::inverse((debugFreecamEnabled) ? UpdateDebugFreecam() : camera.GetCamera());
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

void GraphicsEngine::RenderScene() {
    // glEnable(GL_FRAMEBUFFER_SRGB); // gamma correction; google it. TODO: when we start using postprocessing/framebuffers, turn this off except for final image output

    // std::cout << "\tAdding cached meshes.\n";

    // Update various things

    // std::cout << "\tAdding cached meshes.\n";
    AddCachedMeshes();
    // std::cout << "\tUpdating RCs.\n";
    UpdateRenderComponents();      
    
    // std::cout << "\tUpdating lights.\n";
    UpdateLights();
    //CalculateLightingClusters();

    // std::cout << "\tSetting uniforms.\n";
    // Update shaders with the camera's new rotation/projection
    glm::mat4x4 cameraMatrix = (debugFreecamEnabled) ? UpdateDebugFreecam() : camera.GetCamera();
    glm::mat4x4 cameraMatrixNoFloatingOrigin = glm::translate(cameraMatrix, (debugFreecamEnabled) ? (glm::vec3) -debugFreecamPos : (glm::vec3) -camera.position);
    glm::mat4x4 projectionMatrix = camera.GetProj((float)window.width/(float)window.height); 
    ShaderProgram::SetCameraUniforms(projectionMatrix * cameraMatrix, projectionMatrix * cameraMatrixNoFloatingOrigin);

    // Draw the world onto a framebuffer so we can draw the contents of that framebuffer onto the screen with a postprocessing shader.
    UpdateMainFramebuffer();
    mainFramebuffer->Bind();

    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT); // clear screen

    
    glEnable(GL_DEPTH_TEST); // stuff near the camera should be drawn over stuff far from the camera
    glEnable(GL_CULL_FACE); // backface culling

    // tell opengl how to do transparency
    glEnable(GL_BLEND); 
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  

    // Draw world stuff.
    for (auto & [shaderId, map1] : meshpools) {
        auto& shader = ShaderProgram::Get(shaderId);
        shader->Use();
        pointLightDataBuffer.Bind();
        pointLightDataBuffer.BindBase(1);
        pointLightDataBuffer.Bind();
        for (auto & [materialId, map2] : map1) {
            if (materialId == 0) { // 0 means no material
                Material::Unbind();
            }
            else {
                auto& material = Material::Get(materialId);
                material->Use();
                shader->Uniform("specularMappingEnabled", material->HasSpecularMap());
                shader->Uniform("fontMappingEnabled", material->HasFontMap());
                shader->Uniform("normalMappingEnabled", material->HasNormalMap());
                shader->Uniform("parallaxMappingEnabled", material->HasDisplacementMap());
                shader->Uniform("colorMappingEnabled", material->HasColorMap());

            }
            
            for (auto & [poolId, pool] : map2) {
                pool->Draw();
            } 
        } 
    }
    
    UpdateMeshpools(); // NOTE: this does need to be at the end or the beginning, not the middle, i forget why
    // Draw skybox afterwards to encourage early z-test
    DrawSkybox();

    // Go back to drawing on the window.
    Framebuffer::Unbind();
    glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    // Draw contents of main framebuffer on screen quad, using the postprocessing shader.
    postProcessingShaderProgram->Use();
    mainFramebuffer->textureAttachments.at(0).Use();
    screenQuad.Draw();

    // Debugging stuff
    // TODO: actual settings to toggle debug stuff
    
    // SpatialAccelerationStructure::Get().DebugVisualize();
    DebugAxis();

    glFlush(); // Tell OpenGL we're done drawing.
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
    // Get components of all gameobjects that have a transform and point light component
    auto components = ComponentRegistry::GetSystemComponents<PointLightComponent, TransformComponent>();

    

    // set properties of point lights on gpu
    const unsigned int POINT_LIGHT_OFFSET = sizeof(glm::vec4); // although the first value is just one uint (# of lights), we need vec4 alignment so yeah
    unsigned int lightCount = 0;
    unsigned int i = 0;
    for (auto & tuple : components) {

        PointLightComponent& pointLight = *std::get<0>(tuple);
        TransformComponent& transform = *std::get<1>(tuple);

        if (pointLight.live) {
            glm::vec3 relCamPos = (transform.Position() - (debugFreecamEnabled ? debugFreecamPos : camera.position));
            // std::printf("rel pos = %f %f %f %f\n", relCamPos.x, relCamPos.y, relCamPos.z, pointLight.Range());
            auto info = PointLightInfo {
                .colorAndRange = glm::vec4(pointLight.Color().x, pointLight.Color().y, pointLight.Color().z, pointLight.Range()),
                .relPos = glm::vec4(relCamPos.x, relCamPos.y, relCamPos.z, 0)
            };
            //std::printf("byte offset %llu\n", POINT_LIGHT_OFFSET + (lightCount * sizeof(PointLightInfo)));
            (*(PointLightInfo*)(pointLightDataBuffer.Data() + POINT_LIGHT_OFFSET + (i * sizeof(PointLightInfo)))) = info;

            lightCount++;
        }
        i++;
    }

    // say how many point lights there are
    *(GLuint*)(pointLightDataBuffer.Data()) = lightCount;
}

void GraphicsEngine::SetColor(const RenderComponent& comp, const glm::vec4& rgba) {
    assert(comp.meshpoolId != -1);
    meshpools[comp.shaderProgramId][comp.materialId][comp.meshpoolId]->SetColor(comp.meshpoolSlot, comp.meshpoolInstance, rgba);
}

void GraphicsEngine::SetModelMatrix(const RenderComponent& comp, const glm::mat4x4& model) {
    assert(comp.meshpoolId != -1);
    meshpools[comp.shaderProgramId][comp.materialId][comp.meshpoolId]->SetModelMatrix(comp.meshpoolSlot, comp.meshpoolInstance, model);
}

void GraphicsEngine::SetNormalMatrix(const RenderComponent& comp, const glm::mat3x3& normal) {
    assert(comp.meshpoolId != -1);
    meshpools[comp.shaderProgramId][comp.materialId][comp.meshpoolId]->SetNormalMatrix(comp.meshpoolSlot, comp.meshpoolInstance, normal);
}

void GraphicsEngine::SetTextureZ(const RenderComponent& comp, const float textureZ) {
    assert(comp.meshpoolId != -1);
    meshpools[comp.shaderProgramId][comp.materialId][comp.meshpoolId]->SetTextureZ(comp.meshpoolSlot, comp.meshpoolInstance, textureZ);
}

void GraphicsEngine::UpdateMeshpools() {
    for (auto & [shaderId, map1] : meshpools) {
        for (auto & [textureId, map2] : map1) {
            for (auto & [poolId, pool] : map2) {
                pool->Update();
            } 
        } 
    }
}

// w/ simple for loop: 8ms
// w/ parallelized std::for_each: about 9ms??? unclear if it was actually parallel

// TODO: idk if there's any practical way around this but data locality is probably messed up by jumping around inside the meshpool vbos
void GraphicsEngine::UpdateRenderComponents() {
    // auto start = Time();
    
    auto cameraPos = (debugFreecamEnabled) ? debugFreecamPos : camera.position;

    // Get components of all gameobjects that have a transform and render component
    auto components = ComponentRegistry::GetSystemComponents<RenderComponent, TransformComponent>();
    
    // for (auto & tuple: components) {
    //     auto & renderComp = *std::get<0>(tuple);
    //     auto & transformComp = *std::get<1>(tuple);
    //     if (renderComp.live) {
    //         //std::cout << "Component " << j <<  " at " << renderComp << " is live \n";
    //         // if (renderComp.componentPoolId != i) {
    //         //     //std::cout << "Warning: comp at " << renderComp << " has id " << renderComp->componentPoolId << ", i=" << i << ". ABORT\n";
    //         //     abort();
    //         // }
    //         // TODO: we only need to call SetNormalMatrix() when the object is rotated
    //         SetNormalMatrix(renderComp.meshLocation, transformComp.GetNormalMatrix()); 
    //         SetModelMatrix(renderComp.meshLocation, transformComp.GetGraphicsModelMatrix(cameraPos));
            
    //         if (renderComp.textureZChanged > 0) {renderComp.textureZChanged -= 1; SetTextureZ(renderComp.meshLocation, renderComp.textureZ);}
    //         if (renderComp.colorChanged > 0) {renderComp.colorChanged -= 1; SetColor(renderComp.meshLocation, renderComp.color);}
    //     }
    // }
    
    std::for_each(std::execution::par, components.begin(), components.end(), [this, &cameraPos](std::tuple<RenderComponent*, TransformComponent*>& tuple) {
        auto & renderComp = *std::get<0>(tuple);
        auto & transformComp = *std::get<1>(tuple);
        if (renderComp.live) {
            //std::cout << "Component " << j <<  " at " << renderComp << " is live \n";
            // if (renderComp.componentPoolId != i) {
            //     //std::cout << "Warning: comp at " << renderComp << " has id " << renderComp->componentPoolId << ", i=" << i << ". ABORT\n";
            //     abort();
            // }
            // TODO: we only need to call SetNormalMatrix() when the object is rotated
            SetNormalMatrix(renderComp, transformComp.GetNormalMatrix()); 
            SetModelMatrix(renderComp, transformComp.GetGraphicsModelMatrix(cameraPos));
            
            if (renderComp.textureZChanged > 0) {renderComp.textureZChanged -= 1; SetTextureZ(renderComp, renderComp.textureZ);}
            if (renderComp.colorChanged > 0) {renderComp.colorChanged -= 1; SetColor(renderComp, renderComp.color);}
        }
    });

    // Get components of all gameobjects that have a transform and no floating origin render component
    auto components2 = ComponentRegistry::GetSystemComponents<RenderComponentNoFO, TransformComponent>();
    std::for_each(std::execution::par, components2.begin(), components2.end(), [this](std::tuple<RenderComponentNoFO*, TransformComponent*>& tuple) {
        auto & renderCompNoFO = *std::get<0>(tuple);
        auto & transformComp = *std::get<1>(tuple);
        if (renderCompNoFO.live) {
            
            //std::cout << "Component " << j <<  " at " << renderComp << " is live \n";
            // if (renderComp.componentPoolId != i) {
            //     //std::cout << "Warning: comp at " << renderComp << " has id " << renderComp->componentPoolId << ", i=" << i << ". ABORT\n";
            //     abort();
            // }
            // TODO: we only need to call SetNormalMatrix() when the object is rotated
            SetNormalMatrix(renderCompNoFO, transformComp.GetNormalMatrix()); 
            SetModelMatrix(renderCompNoFO, transformComp.GetGraphicsModelMatrix({0, 0, 0}));
            
            if (renderCompNoFO.textureZChanged > 0) {renderCompNoFO.textureZChanged -= 1; SetTextureZ(renderCompNoFO, renderCompNoFO.textureZ);}
            if (renderCompNoFO.colorChanged > 0) {renderCompNoFO.colorChanged -= 1; SetColor(renderCompNoFO, renderCompNoFO.color);}
        }
    });

    // LogElapsed(start, "Rendercomp update elapsed ");
}

glm::mat4x4 GraphicsEngine::UpdateDebugFreecam() {
    assert(debugFreecamEnabled);

    // camera acceleration
    if (PRESSED_KEYS[GLFW_KEY_W] or PRESSED_KEYS[GLFW_KEY_S] or PRESSED_KEYS[GLFW_KEY_A] or PRESSED_KEYS[GLFW_KEY_D] or PRESSED_KEYS[GLFW_KEY_Q] or PRESSED_KEYS[GLFW_KEY_E]) {
        debugFreecamSpeed += debugFreecamAcceleration;
    }
    else {
        debugFreecamSpeed = debugFreecamAcceleration;
    }

    // camera rotation
    debugFreecamPitch += 0.5 * MOUSE_DELTA.y;
    debugFreecamYaw += 0.5 * MOUSE_DELTA.x;
    if (debugFreecamYaw > 180) {
        debugFreecamYaw -= 360;
    } else if (debugFreecamYaw < -180) {
        debugFreecamYaw += 360;
    } 
    debugFreecamPitch = std::max(-90.0, std::min(debugFreecamPitch, 90.0));

    glm::dquat rotation = glm::rotate(glm::rotate(glm::identity<glm::dmat4x4>(), glm::radians(debugFreecamPitch), glm::dvec3(1, 0, 0)), glm::radians(debugFreecamYaw), glm::dvec3(0, 1, 0));

    // camera movement
    auto look = LookVector(glm::radians(debugFreecamPitch), glm::radians(debugFreecamYaw));
    auto right = LookVector(0, glm::radians(debugFreecamYaw + 90));
    auto up = glm::cross(look, right);
    glm::dvec3 rightMovement = right * debugFreecamSpeed * (double)(PRESSED_KEYS[GLFW_KEY_D] - PRESSED_KEYS[GLFW_KEY_A]);
    glm::dvec3 upMovement = up * debugFreecamSpeed * (double)(PRESSED_KEYS[GLFW_KEY_Q] - PRESSED_KEYS[GLFW_KEY_E]);
    glm::dvec3 forwardMovement = look * debugFreecamSpeed * (double)(PRESSED_KEYS[GLFW_KEY_W] - PRESSED_KEYS[GLFW_KEY_S]);
    debugFreecamPos += rightMovement + forwardMovement + upMovement;

    

    return glm::mat4x4((glm::quat)rotation); 
}

void GraphicsEngine::AddCachedMeshes() {
    if (renderComponentsToAdd.size() > 0) {
        std::cout << "There are " << renderComponentsToAdd.size() << " to add.\n";
    }
    
    for (auto & [shaderId, map1] : renderComponentsToAdd) {
        for (auto & [textureId, map2] : map1) {
            for (auto & [meshId, components] : map2) {
                std::cout << "\tInfo: " << meshId << " " << textureId << " " << shaderId << " size " << renderComponentsToAdd.size() << "\n";

                std::shared_ptr<Mesh>& m = Mesh::Get(meshId);
                const unsigned int verticesNBytes = m->vertices.size() * sizeof(GLfloat);
                const unsigned int indicesNBytes = m->indices.size() * sizeof(GLuint);

                // pick best pool for mesh
                // TODO: O(n) complexity might be an issue
                int bestPoolScore = INT_MAX;
                int bestPoolId = -1;
                for (auto & [poolId, pool] : meshpools[shaderId][textureId]) {
                    int score = pool->ScoreMeshFit(verticesNBytes, indicesNBytes, m->vertexFormat);
                    if (score == -1) {continue;} // this continues the inner loop which is what we want
                    if (score < bestPoolScore) {
                        bestPoolScore = score;
                        bestPoolId = poolId;
                    }
                }

                if (bestPoolId == -1) { // if we didn't find a suitable pool just make one
                    std::cout << "\tmust create pool for new mesh.\n";
                    bestPoolId = lastPoolId++;
                    meshpools[shaderId][textureId][bestPoolId] = new Meshpool(m->vertexFormat, m->vertices.size() * m->vertexFormat.GetNonInstancedVertexSize()/sizeof(GLfloat));
                }
                std::cout << "\tmesh pool id is " << bestPoolId << "\n"; 

                auto objectPositions = meshpools.at(shaderId).at(textureId).at(bestPoolId)->AddObject(meshId, components.size());
                std::cout << "\tAdded.\n";
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

void GraphicsEngine::RenderComponent::Init(unsigned int mesh_id, unsigned int material_id, unsigned int shader_id) {
    assert(live);
    assert(mesh_id != 0);

    color = glm::vec4(1, 1, 1, 1);
    textureZ = -1.0;
    meshId = mesh_id;

    colorChanged = (Mesh::Get(meshId)->vertexFormat.attributes.color->instanced)? INSTANCED_VERTEX_BUFFERING_FACTOR : -1;
    textureZChanged = (Mesh::Get(meshId)->vertexFormat.attributes.textureZ->instanced)? INSTANCED_VERTEX_BUFFERING_FACTOR : -1;

    materialId = material_id;
    shaderProgramId = shader_id;
    meshpoolId = -1;
    meshpoolInstance = -1;

    // std::cout << "Initialized RenderComponent with mesh locatino at " << &meshLocation << " and pool at " << pool << "\n.";

    Get().AddObject(shader_id, materialId, mesh_id, this); 

};

void GraphicsEngine::RenderComponent::Destroy() {
    assert(live == true);

    if (Mesh::Get(meshId)->dynamic) {
        unsigned int i = 0;
        for (auto & component : GraphicsEngine::Get().dynamicMeshUsers.at(meshId)) {
            if (this == component) {
                GraphicsEngine::Get().dynamicMeshUsers.at(meshId).at(i) = GraphicsEngine::Get().dynamicMeshUsers.at(meshId).back();
                GraphicsEngine::Get().dynamicMeshUsers.at(meshId).pop_back();
                break;
            }
            i++;
        }
       
    }

    // if some pyschopath created a RenderComponent and then instantly deleted it, we need to remove it from GraphicsEngine::meshesToAdd
    // meshLocation will still have its shaderProgramId and textureId set tho immediately by AddObject
    unsigned int shaderId = shaderProgramId, textureId = materialId;
    if (meshpoolId == -1) { 
        auto & vec = Get().renderComponentsToAdd.at(shaderId).at(textureId).at(meshId);
        int index = 0;
        for (auto & ptr : vec) {
            if (ptr == this) {
                break;
            }
            index++;
        }
        vec.erase(vec.begin() + index); // todo: pop erase
    }
    else { // otherwise just remove object from graphics engine
        Get().meshpools[shaderId][textureId][meshpoolId]->RemoveObject(meshpoolSlot, meshpoolInstance);
    }
}

GraphicsEngine::RenderComponent::RenderComponent() {

}

void GraphicsEngine::RenderComponent::SetColor(const glm::vec4& rgba) {
    assert(colorChanged != -1); // color must be instanced
    colorChanged = INSTANCED_VERTEX_BUFFERING_FACTOR;
    color = rgba;
}

// set to -1.0 for no texture
void GraphicsEngine::RenderComponent::SetTextureZ(const float z) {
    assert(textureZChanged != -1); // textureZ must be instanced
    textureZChanged = INSTANCED_VERTEX_BUFFERING_FACTOR;
    textureZ = z;
}

GraphicsEngine::RenderComponentNoFO::RenderComponentNoFO() {

}
