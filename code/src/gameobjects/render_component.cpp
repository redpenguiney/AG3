#include "render_component.hpp"

RenderComponent::RenderComponent(unsigned int mId, unsigned int matId, unsigned int shaderId):
    shaderProgramId(shaderId),
    materialId(matId),
    meshId(mId),
    meshpoolId(-1)
{
    //Assert(live);
    Assert(meshId != 0);

    // color = glm::vec4(1, 1, 1, 1);
    // textureZ = -1.0;
    //meshId = mesh_id;

    SetColor(glm::vec4(1, 1, 1, 1));
    SetTextureZ(-1.0);
    // colorChanged = (Mesh::Get(meshId)->vertexFormat.attributes.color->instanced)? INSTANCED_VERTEX_BUFFERING_FACTOR : -1;
    // textureZChanged = (Mesh::Get(meshId)->vertexFormat.attributes.textureZ->instanced)? INSTANCED_VERTEX_BUFFERING_FACTOR : -1;

    // std::cout << "Initialized RenderComponent with mesh locatino at " << &meshLocation << " and pool at " << pool << "\n.";

    GraphicsEngine::Get().AddObject(shaderId, materialId, meshId, this); 

};

RenderComponent::~RenderComponent() {
    //Assert(live == true);

    // tell them we're not a dynamic mesh user anymore since we dying
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

    // make sure no one tries to finish updating instanced vertex attributes for a deleted gameobject
    GraphicsEngine::Get().updater1.CancelUpdate(this);
    GraphicsEngine::Get().updater2.CancelUpdate(this);
    GraphicsEngine::Get().updater3.CancelUpdate(this);
    GraphicsEngine::Get().updater4.CancelUpdate(this);
    GraphicsEngine::Get().updater3x3.CancelUpdate(this);
    GraphicsEngine::Get().updater4x4.CancelUpdate(this);

    // if some pyschopath created a RenderComponent and then instantly deleted it, we need to remove it from GraphicsEngine::meshesToAdd
    // meshLocation will still have its shaderProgramId and textureId set tho immediately by AddObject
    unsigned int shaderId = shaderProgramId, textureId = materialId;
    if (meshpoolId == -1) { 
        auto & vec = GraphicsEngine::Get().renderComponentsToAdd.at(shaderId).at(textureId).at(meshId);
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
        //DebugLogInfo("Please remove thyself.");
        GraphicsEngine::Get().meshpools[meshpoolId]->RemoveObject(drawHandle);
    }
}

//RenderComponent::RenderComponent() {

//}

template <>
void RenderComponent::SetInstancedVertexAttribute<float>(const unsigned int attributeName, const float& value) {
    GraphicsEngine::Get().updater1.AddUpdate(this, attributeName, value);
}
template <>
void RenderComponent::SetInstancedVertexAttribute<glm::vec2>(const unsigned int attributeName, const glm::vec2& value) {
    GraphicsEngine::Get().updater2.AddUpdate(this, attributeName, value);
}
template <>
void RenderComponent::SetInstancedVertexAttribute<glm::vec3>(const unsigned int attributeName, const glm::vec3& value) {
    GraphicsEngine::Get().updater3.AddUpdate(this, attributeName, value);
}
template <>
void RenderComponent::SetInstancedVertexAttribute<glm::vec4>(const unsigned int attributeName, const glm::vec4& value) {
    GraphicsEngine::Get().updater4.AddUpdate(this, attributeName, value);
}
template <>
void RenderComponent::SetInstancedVertexAttribute<glm::mat3x3>(const unsigned int attributeName, const glm::mat3x3& value) {
    GraphicsEngine::Get().updater3x3.AddUpdate(this, attributeName, value);
}
template <>
void RenderComponent::SetInstancedVertexAttribute<glm::mat4x4>(const unsigned int attributeName, const glm::mat4x4& value) {
    GraphicsEngine::Get().updater4x4.AddUpdate(this, attributeName, value);
}



RenderComponentNoFO::RenderComponentNoFO(unsigned int meshId, unsigned int materialId, unsigned int shaderId): 
    RenderComponent(meshId, materialId, shaderId) 
{
}
