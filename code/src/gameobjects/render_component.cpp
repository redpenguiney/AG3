#include "render_component.hpp"

RenderComponent::RenderComponent(unsigned int mesh_id, unsigned int material_id, unsigned int shader_id) {
    //Assert(live);
    Assert(mesh_id != 0);

    

    // color = glm::vec4(1, 1, 1, 1);
    // textureZ = -1.0;
    meshId = mesh_id;

    SetColor(glm::vec4(1, 1, 1, 1));
    SetTextureZ(-1.0);
    // colorChanged = (Mesh::Get(meshId)->vertexFormat.attributes.color->instanced)? INSTANCED_VERTEX_BUFFERING_FACTOR : -1;
    // textureZChanged = (Mesh::Get(meshId)->vertexFormat.attributes.textureZ->instanced)? INSTANCED_VERTEX_BUFFERING_FACTOR : -1;

    materialId = material_id;
    shaderProgramId = shader_id;
    meshpoolId = -1;
    meshpoolInstance = -1;

    // std::cout << "Initialized RenderComponent with mesh locatino at " << &meshLocation << " and pool at " << pool << "\n.";

    GraphicsEngine::Get().AddObject(shader_id, materialId, mesh_id, this); 

};

RenderComponent::~RenderComponent() {
    //Assert(live == true);

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
        GraphicsEngine::Get().meshpools[shaderId][textureId][meshpoolId]->RemoveObject(meshpoolSlot, meshpoolInstance);
    }
}

//RenderComponent::RenderComponent() {

//}

void RenderComponent::SetInstancedVertexAttribute(const unsigned int attributeName, const float& value) {
    // todo: this is dumb type stuff for different nFloats values
    GraphicsEngine::Get().Instanced1ComponentVertexAttributeUpdates.push_back(std::make_tuple(this, attributeName, glm::vec1(value), INSTANCED_VERTEX_BUFFERING_FACTOR));
}

void RenderComponent::SetInstancedVertexAttribute(const unsigned int attributeName, const glm::vec2& value) {
    // todo: this is dumb type stuff for different nFloats values
    GraphicsEngine::Get().Instanced2ComponentVertexAttributeUpdates.push_back(std::make_tuple(this, attributeName, value, INSTANCED_VERTEX_BUFFERING_FACTOR));
}

void RenderComponent::SetInstancedVertexAttribute(const unsigned int attributeName, const glm::vec3& value) {
    // todo: this is dumb type stuff for different nFloats values
    GraphicsEngine::Get().Instanced3ComponentVertexAttributeUpdates.push_back(std::make_tuple(this, attributeName, value, INSTANCED_VERTEX_BUFFERING_FACTOR));
}

void RenderComponent::SetInstancedVertexAttribute(const unsigned int attributeName, const glm::vec4& value) {
    // todo: this is dumb type stuff for different nFloats values
    GraphicsEngine::Get().Instanced4ComponentVertexAttributeUpdates.push_back(std::make_tuple(this, attributeName, value, INSTANCED_VERTEX_BUFFERING_FACTOR));
}


RenderComponentNoFO::RenderComponentNoFO(unsigned int meshId, unsigned int materialId, unsigned int shaderId): 
    RenderComponent(meshId, materialId, shaderId) 
{

}
