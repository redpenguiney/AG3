#include "tile_chunk.hpp"
#include "gameobjects/gameobject.hpp"
#include "world.hpp"
#include "tile_data.hpp"

void RenderChunk::MakeMesh(glm::ivec2 centerPos, int stride, int radius) {

	MeshCreateParams params;
	params.meshVertexFormat.emplace(MeshVertexFormat({
		.position = VertexAttribute {.nFloats = 3, .instanced = false},
		.textureUV = VertexAttribute {.nFloats = 2, .instanced = false},
		.textureZ = VertexAttribute {.nFloats = 1, .instanced = true},
		.modelMatrix = VertexAttribute {.nFloats = 16, .instanced = true},
		.normalMatrix = VertexAttribute {.nFloats = 9, .instanced = true},
		.normal = VertexAttribute {.nFloats = 3, .instanced = false},
		.tangent = VertexAttribute {.nFloats = 3, .instanced = false},
	}, false, 0));
	params.normalizeSize = true;

	GLuint floatsPerVertex = sizeof(GLfloat) / params.meshVertexFormat->GetNonInstancedVertexSize();

	std::vector<GLfloat> vertices;
	std::vector<GLuint> indices;

	std::vector<TerrainTile> tiles((radius * 2 + 2) * (radius * 2 + 2));
	auto& world = World::loaded;

	// TODO: not best way to fill up tiles, repeatedly has to fetch same chunk
	for (int i = 0; i < radius * 2 + 2; i += stride) {
		for (int j = 0; j < radius * 2 + 2; j += stride) {
			tiles[i * (radius + 2) + j] = world->GetTile(i + centerPos.x - radius - 1, j + centerPos.y - radius - 1);
		}
	}

	for (int i = centerPos.x - radius; i < centerPos.x + radius; i += stride) {
		for (int j = centerPos.y - radius; j < centerPos.y + radius; j += stride) {
			const auto& tile = tiles[i * (radius + 2) + j];
			const auto& floorData = GetTileData(tile.floor);

			if (floorData.gameobject.has_value()) {
				objects.push_back(GameObject::New(floorData.gameobject.value()));
				objects.back()->RawGet<TransformComponent>()->SetPos(glm::dvec3(i, floorData.yOffset, j));
				objects.back()->RawGet<TransformComponent>()->SetScl(Mesh::Get(floorData.gameobject->meshId)->originalSize);
			}
			else {
				GLuint vertexIndex = vertices.size() / floatsPerVertex;
				unsigned int floatIndex = vertices.size();
				indices.push_back(vertexIndex);
				indices.push_back(vertexIndex + 1);
				indices.push_back(vertexIndex + 2);
				indices.push_back(vertexIndex);
				indices.push_back(vertexIndex + 2);
				indices.push_back(vertexIndex + 3);

				vertices.resize(vertices.size() + floatsPerVertex * 4);
				for (unsigned int x = 0; x < 2; x++) {
					for (unsigned int z = 0; z < 2; z++) {
						// positions
						vertices[floatIndex + x * 2 + z + params.meshVertexFormat->attributes.position->offset / sizeof(GLfloat)] = i + x;
						vertices[floatIndex + x * 2 + z + params.meshVertexFormat->attributes.position->offset / sizeof(GLfloat)] = floorData.yOffset;
						vertices[floatIndex + x * 2 + z + params.meshVertexFormat->attributes.position->offset / sizeof(GLfloat)] = j + z;

						// UVs
						vertices[floatIndex + x * 2 + z + params.meshVertexFormat->attributes.textureUV->offset / sizeof(GLfloat)] = 0;
						vertices[floatIndex + x * 2 + z + params.meshVertexFormat->attributes.textureUV->offset / sizeof(GLfloat)] = 0;

						// normals
						vertices[floatIndex + x * 2 + z + params.meshVertexFormat->attributes.normal->offset / sizeof(GLfloat)] = 0;
						vertices[floatIndex + x * 2 + z + params.meshVertexFormat->attributes.normal->offset / sizeof(GLfloat)] = 1;
						vertices[floatIndex + x * 2 + z + params.meshVertexFormat->attributes.normal->offset / sizeof(GLfloat)] = 0;

						// tangents
						vertices[floatIndex + x * 2 + z + params.meshVertexFormat->attributes.tangent->offset / sizeof(GLfloat)] = 1;
						vertices[floatIndex + x * 2 + z + params.meshVertexFormat->attributes.tangent->offset / sizeof(GLfloat)] = 0;
						vertices[floatIndex + x * 2 + z + params.meshVertexFormat->attributes.tangent->offset / sizeof(GLfloat)] = 0;
					}
				}
			}

			if (tile.furniture != -1) {
				const auto& furnitureData = GetTileData(tile.furniture);
				
				Assert(furnitureData.gameobject.has_value());

				objects.push_back(GameObject::New(furnitureData.gameobject.value()));
				objects.back()->RawGet<TransformComponent>()->SetPos(glm::dvec3(i, furnitureData.yOffset, j));
				objects.back()->RawGet<TransformComponent>()->SetScl(Mesh::Get(furnitureData.gameobject->meshId)->originalSize);
			}
		}
	}

	mesh = Mesh::New(RawMeshProvider(vertices, indices, params), false);
}

RenderChunk::RenderChunk(glm::ivec2 centerPos, int stride, int radius, std::shared_ptr<Material>& material, std::shared_ptr<TextureAtlas>& atlas):
	material(material),
	atlas(atlas)
{

	MakeMesh(centerPos, stride, radius);
	

	auto params = GameobjectCreateParams({ComponentBitIndex::Render, ComponentBitIndex::Collider, ComponentBitIndex::Transform});
	params.materialId = material->id;
	params.meshId = mesh->meshId;
	mainObject = GameObject::New(params);
	mainObject->RawGet<TransformComponent>()->SetPos(glm::dvec3(centerPos.x, 0, centerPos.y));
	mainObject->RawGet<TransformComponent>()->SetScl(glm::dvec3(radius * 2.0));

}

RenderChunk::~RenderChunk() {
	mainObject->Destroy();
	Mesh::Unload(mesh->meshId);
}
