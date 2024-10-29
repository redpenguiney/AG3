#include "tile_chunk.hpp"
#include "gameobjects/gameobject.hpp"
#include "world.hpp"
#include "tile_data.hpp"

void RenderChunk::MakeMesh(glm::ivec2 centerPos, int stride, int radius) {
	std::vector<GLfloat> vertices;
	std::vector<GLuint> indices;

	std::vector<TerrainTile> tiles((radius * 2 + 2) * (radius * 2 + 2));
	auto& world = World::loaded;

	// TODO: not best way to fill up tiles, repeatedly has to fetch same chunk
	for (int i = 0; i < radius * 2 + 2; i++) {
		for (int j = 0; j < radius * 2 + 2; j++) {
			tiles[i * (radius + 2) + j] = world->GetTile(i + centerPos.x - radius - 1, j + centerPos.y - radius - 1);
		}
	}

	for (int i = centerPos.x - radius; i < centerPos.x + radius; i++) {
		for (int j = centerPos.y - radius; j < centerPos.y + radius; j++) {
			const auto& tile = tiles[i * (radius + 2) + j];
			const auto& floorData = GetTileData(tile.floor);

			for (auto& i : floorData.mesh->indices) {
				indices.push_back(i + vertices.size());
			}
			unsigned int vIndex = 0; // we need to rewrite UVs
			bool uvs = floorData.mesh->vertexFormat.attributes.textureUV.has_value();
			for (auto v : floorData.mesh->vertices) {
				if (uvs && floorData.mesh->vertexFormat.attributes.textureUV.va)
				vertices.push_back(v);
			}

			if (tile.furniture != -1) {
				const auto& furnitureData = GetTileData(tile.furniture);
				for (auto& i : furnitureData.mesh->indices) {
					indices.push_back(i + vertices.size());
				}
				vertices.insert(vertices.end(), furnitureData.mesh->vertices.begin(), furnitureData.mesh->vertices.end());
			}
		}
	}

	mesh = Mesh::New(RawMeshProvider(vertices, indices, MeshCreateParams::Default()));
}

RenderChunk::RenderChunk(glm::ivec2 centerPos, int stride, int radius, std::shared_ptr<Material>& material, std::shared_ptr<TextureAtlas>& atlas):
	material(material),
	atlas(atlas)
{

	MakeMesh(centerPos, stride, radius);
	

	auto params = GameobjectCreateParams({ComponentBitIndex::Render, ComponentBitIndex::Collider, ComponentBitIndex::Transform});
	params.materialId = material->id;
	params.meshId = mesh->meshId;
	object = GameObject::New(params);

}

RenderChunk::~RenderChunk() {
	object->Destroy();
	Mesh::Unload(mesh->meshId);
}
