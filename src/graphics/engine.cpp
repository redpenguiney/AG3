#pragma once
#include "engine.hpp"
#include <cstdio>

void GraphicsEngine::Init() {
    debugFreecamEnabled = false;
    debugFreecamPos = {0.0, 0.0, 0.0};
    debugFreecamPitch = 0.0;
    debugFreecamYaw = 0.0;

    defaultShaderProgramId = ShaderProgram::New("../shaders/world_vertex.glsl", "../shaders/world_fragment.glsl");
    std::printf("\n Def id is %u", defaultShaderProgramId);
}

void GraphicsEngine::Terminate() {
    for (auto & [shaderId, map1] : meshpools) {
        for (auto & [textureId, map2] : map1) {
            for (auto & [poolId, pool] : map2) {
                delete pool;
            } 
        } 
    }
}

// Used by Texture to throw an error if someone tries to unload a texture being used
bool GraphicsEngine::IsTextureInUse(unsigned int textureId) { 
    for (auto & [shaderId, map] : meshpools) {
        if (map.count(textureId)) {
            return true;
        }
    }
    return false;
}

// Used by ShaderProgram to throw an error if someone tries to unload a shader being used.
bool GraphicsEngine::IsShaderProgramInUse(unsigned int shaderId) {
    return meshpools.count(shaderId);
}

// returns true if the user is trying to close the application, or if glfwSetWindowShouldClose was explicitly called (like by a quit game button)
bool GraphicsEngine::ShouldClose() {
    return window.ShouldClose();
}

// Draws everything
void GraphicsEngine::RenderScene() {
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT); // clear screen
    glEnable(GL_DEPTH_TEST); // stuff near the camera should be drawn over stuff far from the camera
    glEnable(GL_CULL_FACE); // backface culling

    Update();

    glm::mat4x4 cameraMatrix = (debugFreecamEnabled) ? UpdateDebugFreecam() : camera.GetCamera();
    glm::mat4x4 projectionMatrix = camera.GetProj((float)window.width/(float)window.height); 
    ShaderProgram::SetCameraUniforms(projectionMatrix * cameraMatrix);

    for (auto & [shaderId, map1] : meshpools) {
        ShaderProgram::Get(shaderId)->Use();
        for (auto & [textureId, map2] : map1) {
            Texture::Get(textureId)->Use();
            for (auto & [poolId, pool] : map2) {
                pool->Draw();
            } 
        } 
    }


    window.Update(); // this flips the buffer so it goes at the end; TODO maybe poll events at start of frame instead
}

void GraphicsEngine::SetColor(MeshLocation& location, glm::vec4 rgba) {
    assert(location.initialized);
    meshpools[location.shaderProgramId][location.textureId][location.poolId]->SetColor(location.poolSlot, location.poolInstance, rgba);
}

void GraphicsEngine::SetModelMatrix(MeshLocation& location, glm::mat4x4 model) {
    assert(location.initialized);
    meshpools[location.shaderProgramId][location.textureId][location.poolId]->SetModelMatrix(location.poolSlot, location.poolInstance, model);
}

// set to -1.0 for no texture
void GraphicsEngine::SetTextureZ(MeshLocation& location, float textureZ) {
    assert(location.initialized);
    meshpools[location.shaderProgramId][location.textureId][location.poolId]->SetTextureZ(location.poolSlot, location.poolInstance, textureZ);
}

void GraphicsEngine::Update() {  
    if (PRESS_BEGAN_KEYS[GLFW_KEY_ESCAPE]) {
        window.Close();
    }
    if (PRESS_BEGAN_KEYS[GLFW_KEY_TAB]) {
        window.SetMouseLocked(!window.mouseLocked);
    }
    AddCachedMeshes();
    UpdateRenderComponents();        
}

void GraphicsEngine::UpdateRenderComponents() {
        auto cameraPos = (debugFreecamEnabled) ? debugFreecamPos : camera.position;

        for (unsigned int i = 0; i < RENDER_COMPONENTS.pools.size(); i++) {
            auto renderArray = RENDER_COMPONENTS.pools.at(i);
            auto transformArray = TransformComponent::TRANSFORM_COMPONENTS.pools.at(i);
            for (unsigned int j = 0; j < RENDER_COMPONENT_POOL_SIZE; j++) {
                auto renderComp = renderArray + j;
                auto transformComp = transformArray + j;
                if (renderComp->live) {
                    SetModelMatrix(renderComp->meshLocation, transformComp->GetModel(cameraPos));
                    SetTextureZ(renderComp->meshLocation, 0.0);
                }
            }
        }
    }

// updates the freecam based off user input (WASD and mouse) and then returns a camera matrix
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
    debugFreecamPitch = std::max((float)-89.999, std::min(debugFreecamPitch, (float)89.999));

    glm::quat rotation = glm::rotate(glm::rotate(glm::identity<glm::mat4x4>(), glm::radians(debugFreecamPitch), glm::vec3(1, 0, 0)), glm::radians(debugFreecamYaw), glm::vec3(0, 1, 0));

    // camera movement
    auto look = LookVector(glm::radians(debugFreecamPitch), glm::radians(debugFreecamYaw));
    auto right = glm::cross(look, glm::dvec3(0, 1, 0));
    auto up = glm::cross(look, right);
    glm::dvec3 rightMovement = right * debugFreecamSpeed * (double)(PRESSED_KEYS[GLFW_KEY_A] - PRESSED_KEYS[GLFW_KEY_D]);
    glm::dvec3 upMovement = up * debugFreecamSpeed * (double)(PRESSED_KEYS[GLFW_KEY_Q] - PRESSED_KEYS[GLFW_KEY_E]);
    glm::dvec3 forwardMovement = look * debugFreecamSpeed * (double)(PRESSED_KEYS[GLFW_KEY_S] - PRESSED_KEYS[GLFW_KEY_W]);
    debugFreecamPos += rightMovement + forwardMovement + upMovement;

    

    return glm::mat4x4(rotation); 
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
    // TODO: instead of clearing the entire me
    meshesToAdd.clear();
}

// Returns a drawId used to modify the mesh later on.
// Does not actually add the object for performance reasons, just puts it on a list of stuff to add when GraphicsEngine::addCachedMeshes is called.
// Contents of MeshLocation pointer are undefined until addCachedMeshes() is called.
void GraphicsEngine::AddObject(unsigned int shaderId, unsigned int textureId, unsigned int meshId, MeshLocation* meshLocation) {
    meshesToAdd[shaderId][textureId][meshId].push_back(meshLocation);
}

GraphicsEngine::RenderComponent* GraphicsEngine::RenderComponent::New(unsigned int mesh_id, unsigned int texture_id, unsigned int shader_id) {
    auto ptr = RENDER_COMPONENTS.GetNew();
    ptr->live = true;
    ptr->shaderId = shader_id;
    ptr->textureId = texture_id;
    ptr->meshId = mesh_id;

    ptr->meshLocation.textureId = texture_id;
    ptr->meshLocation.shaderProgramId = shader_id;

    AddObject(ptr->shaderId, ptr->textureId, ptr->meshId, &ptr->meshLocation);
    return ptr;

};

// call instead of deleting the pointer.
// obviously don't touch component after this.
void GraphicsEngine::RenderComponent::Destroy() {
    assert(live == true);

    // if some pyschopath created a RenderComponent and then instantly deleted it, we need to remove it from GraphicsEngine::meshesToAdd
    if (!meshLocation.initialized) { 
        auto & vec = meshesToAdd.at(shaderId).at(textureId).at(meshId);
        int index = -1;
        for (auto & ptr : vec) {
            if (ptr == &meshLocation) {
                break;
            }
            index++;
        }
        vec.erase(vec.begin() + index);
    }
    else { // otherwise just remove object from graphics engine
        // TODO
    }

    live = false;
    
    RENDER_COMPONENTS.ReturnObject(this);
}

GraphicsEngine::RenderComponent::RenderComponent() {
    live = false;
}

