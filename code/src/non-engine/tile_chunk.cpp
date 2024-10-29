#include "tile_chunk.hpp"
#include "gameobjects/gameobject.hpp"
#include "world.hpp"

std::pair < std::vector<GLfloat>, std::vector<GLuint> > GetMesh(glm::ivec2 centerPos, int stride, int radius) {
	std::vector<GLfloat> vertices;
	std::vector<GLuint> indices;

	std::vector<TerrainTile> tiles((radius * 2 + 2) * (radius * 2 + 2));
	auto& world = World::loaded;

	// TODO: not best way to fill up tiles
	for (int i = 0; i < radius * 2 + 2; i++) {
		for (int j = 0; j < radius * 2 + 2; j++) {
			tiles[i * (radius + 2) + j] = world->GetTile(i + centerPos.x - radius - 1, j + centerPos.y - radius - 1);
		}
	}

	for (int i = centerPos.x - radius; i < centerPos.x + radius; i++) {
		for (int j = centerPos.y - radius; j < centerPos.y + radius; j++) {

		}
	}

	return { vertices, indices };
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
