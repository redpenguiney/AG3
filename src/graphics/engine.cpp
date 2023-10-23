#include "engine.hpp"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <iostream>
#include "../gameobjects/pointlight_component.hpp"

GraphicsEngine& GraphicsEngine::Get() {
    static GraphicsEngine engine;
    return engine;
}

// thing we send to gpu to tell it about a light
struct PointLightInfo {
    glm::vec4 colorAndRange; // w-coord is range, xyz is rgb
    glm::vec4 rel_pos; // w-coord is padding; openGL wants everything on a vec4 alignment
};

GraphicsEngine::GraphicsEngine(): pointLightDataBuffer(GL_SHADER_STORAGE_BUFFER, 1, sizeof(PointLightInfo) * 1024) {
    debugFreecamEnabled = false;
    debugFreecamPos = {0.0, 0.0, 0.0};
    debugFreecamPitch = 0.0;
    debugFreecamYaw = 0.0;

    

    defaultShaderProgramId = ShaderProgram::New("../shaders/world_vertex.glsl", "../shaders/world_fragment.glsl")->shaderProgramId;
    skyboxShaderProgram = ShaderProgram::New("../shaders/skybox_vertex.glsl", "../shaders/skybox_fragment.glsl");

    skybox = new RenderableMesh(Mesh::FromFile("../models/skybox.obj"));

    glDepthFunc(GL_LEQUAL); // the skybox's z-coord is hardcoded to 1 so it's not drawn over anything, but depth buffer is all 1 by default so this makes skybox able to be drawn
}

GraphicsEngine::~GraphicsEngine() {
    for (auto & [shaderId, map1] : meshpools) {
        for (auto & [textureId, map2] : map1) {
            for (auto & [poolId, pool] : map2) {
                delete pool;
            } 
        } 
    }

    delete skybox;
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

void GraphicsEngine::RenderScene() {
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT); // clear screen
    glEnable(GL_DEPTH_TEST); // stuff near the camera should be drawn over stuff far from the camera
    glEnable(GL_CULL_FACE); // backface culling

    if (PRESS_BEGAN_KEYS[GLFW_KEY_ESCAPE]) {
        window.Close();
    }
    if (PRESS_BEGAN_KEYS[GLFW_KEY_TAB]) {
        window.SetMouseLocked(!window.mouseLocked);
    }

    // Update various things
    AddCachedMeshes();
    UpdateRenderComponents();      
    UpdateMeshpools(); // NOTE: this was at the end of the function before, if it breaks move to after DrawSkybox()
    UpdateLights();
    //CalculateLightingClusters();

    // Update shaders with the camera's new rotation/projection
    glm::mat4x4 cameraMatrix = (debugFreecamEnabled) ? UpdateDebugFreecam() : camera.GetCamera();
    glm::mat4x4 projectionMatrix = camera.GetProj((float)window.width/(float)window.height); 
    ShaderProgram::SetCameraUniforms(projectionMatrix * cameraMatrix);

    // Draw world stuff.
    for (auto & [shaderId, map1] : meshpools) {
        ShaderProgram::Get(shaderId)->Use();
        for (auto & [textureId, map2] : map1) {
            Texture::Get(textureId)->Use();
            for (auto & [poolId, pool] : map2) {
                pool->Draw();
            } 
        } 
    }

    // Draw skybox afterwards to encourage early z-test
    DrawSkybox();

    
    window.Update(); // this flips the buffer so it goes at the end; TODO maybe poll events at start of frame instead
}

void GraphicsEngine::DrawSkybox() {
    glDisable(GL_CULL_FACE);
    skyboxShaderProgram->Use();
    skyboxTexture->Use();
    skybox->Draw();
    
    glClear(GL_DEPTH_BUFFER_BIT); // make sure skybox isn't drawn over everything else
}

void GraphicsEngine::UpdateLights() {
    // say how many point lights there are
    *(GLuint*)(pointLightDataBuffer.Data()) = PointLightComponent::POINT_LIGHT_COMPONENTS.size();

    // set properties of point lights on gpu
    const unsigned int POINT_LIGHT_OFFSET = sizeof(glm::vec4); // although the first value is just one float we need vec4 alignment so yeah
    unsigned int i = 0;
    for (auto &pointLight : PointLightComponent::POINT_LIGHT_COMPONENTS) {
        std::printf("doing thing with light\n");
        i++;
        glm::vec3 relCamPos = ((debugFreecamEnabled ? debugFreecamPos : camera.position) - pointLight->transform->position());
        *(PointLightInfo*)(pointLightDataBuffer.Data() + POINT_LIGHT_OFFSET + (i * sizeof(PointLightInfo))) = PointLightInfo {.colorAndRange = glm::vec4(pointLight->lightColor.x, pointLight->lightColor.y, pointLight->lightColor.z, pointLight->lightRange), .rel_pos = glm::vec4(relCamPos.x, relCamPos.y, relCamPos.z, 0)};
    }
}

void GraphicsEngine::SetColor(MeshLocation& location, glm::vec4 rgba) {
    assert(location.initialized);
    meshpools[location.shaderProgramId][location.textureId][location.poolId]->SetColor(location.poolSlot, location.poolInstance, rgba);
}

void GraphicsEngine::SetModelMatrix(MeshLocation& location, glm::mat4x4 model) {
    assert(location.initialized);
    meshpools[location.shaderProgramId][location.textureId][location.poolId]->SetModelMatrix(location.poolSlot, location.poolInstance, model);
}

void GraphicsEngine::SetTextureZ(MeshLocation& location, float textureZ) {
    assert(location.initialized);
    meshpools[location.shaderProgramId][location.textureId][location.poolId]->SetTextureZ(location.poolSlot, location.poolInstance, textureZ);
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

void GraphicsEngine::UpdateRenderComponents() {
    //auto start = Time();
    auto cameraPos = (debugFreecamEnabled) ? debugFreecamPos : camera.position;

    //assert(RENDER_COMPONENTS.pools.size() == TransformComponent::TRANSFORM_COMPONENTS.pools.size());
    //static_assert(RENDER_COMPONENT_POOL_SIZE == TransformComponent::TRANSFORM_COMPONENT_POOL_SIZE, "render comp and transform comp need to have to same pool size");
    for (unsigned int i = 0; i < RENDER_COMPONENTS.pools.size(); i++) {
        auto renderArray = RENDER_COMPONENTS.pools.at(i);
        auto transformArray = TransformComponent::TRANSFORM_COMPONENTS.pools.at(i);
        for (unsigned int j = 0; j < RENDER_COMPONENTS.COMPONENTS_PER_POOL; j++) {
            auto renderComp = renderArray + j;
            auto transformComp = transformArray + j;
            if (renderComp->live) {
                SetModelMatrix(renderComp->meshLocation, transformComp->GetGraphicsModelMatrix(cameraPos));
                
                if (renderComp->textureZChanged > 0) {renderComp->textureZChanged -= 1; SetTextureZ(renderComp->meshLocation, renderComp->textureZ);}
                if (renderComp->colorChanged > 0) {renderComp->colorChanged -= 1; SetColor(renderComp->meshLocation, renderComp->color);}
            }
        }
    }

    //LogElapsed(start, "\nRendercomp update elapsed ");
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
    for (auto & [shaderId, map1] : meshesToAdd) {
        for (auto & [textureId, map2] : map1) {
            for (auto & [meshId, meshLocations] : map2) {

                std::shared_ptr<Mesh>& m = Mesh::Get(meshId);
                const unsigned int verticesNBytes = m->vertices.size() * sizeof(GLfloat);
                const unsigned int indicesNBytes = m->indices.size() * sizeof(GLuint);
                const bool shouldInstanceColor = m->instancedColor;
                const bool shouldInstanceTextureZ = m->instancedTextureZ;

                // pick best pool for mesh
                // TODO: O(n) complexity might be an issue
                int bestPoolScore = INT_MAX;
                int bestPoolId = -1;
                for (auto & [poolId, pool] : meshpools[shaderId][textureId]) {
                    int score = pool->ScoreMeshFit(verticesNBytes, indicesNBytes, shouldInstanceColor, shouldInstanceTextureZ);
                    if (score == -1) {continue;} // this continues the inner loop which is what we want
                    if (score < bestPoolScore) {
                        bestPoolScore = score;
                        bestPoolId = poolId;
                    }
                }

                if (bestPoolId == -1) { // if we didn't find a suitable pool just make one
                    bestPoolId = lastPoolId++;
                    meshpools[shaderId][textureId][bestPoolId] = new Meshpool(m);
                }

                auto objectPositions = meshpools[shaderId][textureId][bestPoolId]->AddObject(meshId, meshLocations.size());
                for (unsigned int i = 0; i < meshLocations.size(); i++) {
                    meshLocations[i]->poolId = bestPoolId;
                    meshLocations[i]->poolSlot = objectPositions[i].first;
                    meshLocations[i]->poolInstance = objectPositions[i].second;
                    meshLocations[i]->initialized = true;
                }
            } 
        }
    }
    // TODO: instead of clearing the entire thing do something else
    meshesToAdd.clear();
}

void GraphicsEngine::AddObject(unsigned int shaderId, unsigned int textureId, unsigned int meshId, MeshLocation* meshLocation) {
    meshLocation->shaderProgramId = shaderId;
    meshLocation->textureId = textureId;
    meshesToAdd[shaderId][textureId][meshId].push_back(meshLocation);
}

GraphicsEngine::RenderComponent* GraphicsEngine::RenderComponent::New(unsigned int mesh_id, unsigned int texture_id, bool visible, unsigned int shader_id) {
    auto ptr = Get().RENDER_COMPONENTS.GetNew();
    ptr->live = visible;

    if (visible) {
        assert(mesh_id != 0 && texture_id != 0);

        ptr->colorChanged = (Mesh::Get(mesh_id)->instancedColor)? INSTANCED_VERTEX_BUFFERING_FACTOR : -1;
        ptr->textureZChanged = (Mesh::Get(mesh_id)->instancedTextureZ)? INSTANCED_VERTEX_BUFFERING_FACTOR : -1;
        ptr->color = glm::vec4(1, 1, 1, 1);
        ptr->textureZ = -1.0;
        ptr->meshId = mesh_id;

        ptr->meshLocation.textureId = texture_id;
        ptr->meshLocation.shaderProgramId = shader_id;

        Get().AddObject(shader_id, texture_id, mesh_id, &ptr->meshLocation); 
    }

    return ptr;

};

void GraphicsEngine::RenderComponent::Destroy() {
    assert(live == true);

    // if some pyschopath created a RenderComponent and then instantly deleted it, we need to remove it from GraphicsEngine::meshesToAdd
    // meshLocation will still have its shaderProgramId and textureId set tho immediately by AddObject
    unsigned int shaderId = meshLocation.shaderProgramId, textureId = meshLocation.textureId;
    if (!meshLocation.initialized) { 
        auto & vec = Get().meshesToAdd.at(shaderId).at(textureId).at(meshId);
        int index = 0;
        for (auto & ptr : vec) {
            if (ptr == &meshLocation) {
                break;
            }
            index++;
        }
        vec.erase(vec.begin() + index);
    }
    else { // otherwise just remove object from graphics engine
        Get().meshpools[shaderId][textureId][meshLocation.poolId]->RemoveObject(meshLocation.poolSlot, meshLocation.poolInstance);
    }

    live = false;
    
    Get().RENDER_COMPONENTS.ReturnObject(this);
}

GraphicsEngine::RenderComponent::RenderComponent() {
    live = false;
}

void GraphicsEngine::RenderComponent::SetColor(glm::vec4 rgba) {
    assert(colorChanged != -1); // color must be instanced
    colorChanged = INSTANCED_VERTEX_BUFFERING_FACTOR;
    color = rgba;
}

// set to -1.0 for no texture
void GraphicsEngine::RenderComponent::SetTextureZ(float z) {
    assert(textureZChanged != -1); // textureZ must be instanced
    textureZChanged = INSTANCED_VERTEX_BUFFERING_FACTOR;
    textureZ = z;
}

