#include "tile_chunk.hpp"
#include "gameobjects/gameobject.hpp"

std::pair < std::vector<GLfloat>, std::vector<GLuint> > GetMesh(glm::ivec2 centerPos, int stride, int radius) {

}

RenderChunk::RenderChunk(glm::ivec2 centerPos, int stride, int radius, std::shared_ptr<Material>& material):
	material(material)
{

	auto [vertices, indices] = GetMesh(centerPos, stride, radius);
	mesh = Mesh::New(RawMeshProvider(vertices, indices, MeshCreateParams::Default()));

	auto params = GameobjectCreateParams({ComponentBitIndex::Render, ComponentBitIndex::Collider, ComponentBitIndex::Transform});
	params.materialId = material->id;
	object = GameObject::New(params);

}

RenderChunk::~RenderChunk() {
	object->Destroy();
	Mesh::Unload(mesh->meshId);
}
