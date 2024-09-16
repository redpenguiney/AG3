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

    //SetColor(glm::vec4(1, 1, 1, 1));
    //SetTextureZ(-1.0);
    // colorChanged = (Mesh::Get(meshId)->vertexFormat.attributes.color->instanced)? INSTANCED_VERTEX_BUFFERING_FACTOR : -1;
    // textureZChanged = (Mesh::Get(meshId)->vertexFormat.attributes.textureZ->instanced)? INSTANCED_VERTEX_BUFFERING_FACTOR : -1;

    DebugLogInfo("Initializing ", this);

    GraphicsEngine::Get().AddObject(shaderId, materialId, meshId, this); 

};

RenderComponent::~RenderComponent() {
    //Assert(live == true);
    GraphicsEngine::Get().RemoveObject(this);
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
