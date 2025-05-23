#include "tile_chunk.hpp"
#include "gameobjects/gameobject.hpp"
#include "world.hpp"
#include "tile_data.hpp"
#include <tests/gameobject_tests.hpp>

void RenderChunk::MakeMesh(glm::ivec2 centerPos, int stride, int radius) {

	MeshCreateParams params;
	// TODO: graphics utterly fails if we try to not have color as an attribute
	params.meshVertexFormat.emplace(MeshVertexFormat({
		.position = VertexAttribute {.nFloats = 3, .instanced = false},
		.textureUV = VertexAttribute {.nFloats = 2, .instanced = false},
		.textureZ = VertexAttribute {.nFloats = 1, .instanced = false},
		.modelMatrix = VertexAttribute {.nFloats = 16, .instanced = true},
		.normalMatrix = VertexAttribute {.nFloats = 9, .instanced = true},
		//.normal = VertexAttribute {.nFloats = 3, .instanced = false},
		//.tangent = VertexAttribute {.nFloats = 3, .instanced = false},
		
	}, false, 0));
	params.meshVertexFormat->primitiveType = GL_TRIANGLES;
	
	//params.meshVertexFormat.emplace(MeshVertexFormat::Default());
	params.normalizeSize = true;

	GLuint floatsPerVertex = params.meshVertexFormat->GetNonInstancedVertexSize() / sizeof(GLfloat);

	std::vector<GLfloat> vertices;
	std::vector<GLuint> indices;

	int width = radius * 2;

	// +2s for edges
	std::vector<TerrainTile> tiles((width + 2) * (width + 2), TerrainTile {.layers = {2, -2}});
	auto& world = World::Loaded();

	// TODO: not best way to fill up tiles, repeatedly has to fetch same chunk
	for (int i = 0; i < width + 2; i += stride) {
		for (int j = 0; j < width + 2; j += stride) {
			int worldX = i + centerPos.x - radius - 1;
			int worldZ = j + centerPos.y - radius - 1;
			tiles[i * (width + 2) + j] = world->GetTile(worldX, worldZ);
		}
	}

	//DebugLogInfo("Radius ", radius);

	for (int i = 0; i < width; i += stride) {
		for (int j = 0; j < width; j += stride) {

			//const auto& tile = world->GetTile(i - radius + centerPos.x, j - radius + centerPos.y);
			const auto& tile = tiles[(i + 1) * (width + 2) + j + 1];

			Assert(tile.layers[TileLayer::Floor] != -2);

			if (tile.layers[TileLayer::Floor] == -1) {
				continue;
			}

			const auto& floorData = GetTileData(tile.layers[TileLayer::Floor]);

			if (floorData.gameobject.has_value()) {
				objects.push_back(GameObject::New(floorData.gameobject.value()));
				objects.back()->RawGet<TransformComponent>()->SetPos(glm::dvec3(i - radius + centerPos.x, floorData.yOffset, j - radius + centerPos.y));
				objects.back()->RawGet<TransformComponent>()->SetScl(Mesh::Get(floorData.gameobject->meshId)->originalSize);
			}
			else {
				GLuint vertexIndex = vertices.size() / floatsPerVertex;
				unsigned int floatIndex = vertices.size();
				indices.push_back(vertexIndex);
				indices.push_back(vertexIndex + 1);
				indices.push_back(vertexIndex + 2);
				indices.push_back(vertexIndex + 1);
				indices.push_back(vertexIndex + 3);
				indices.push_back(vertexIndex + 2);

				float hUvs[2] = {World::TerrainAtlas()->regions[floorData.texAtlasRegionId].left, World::TerrainAtlas()->regions[floorData.texAtlasRegionId].right };
				float vUvs[2] = {World::TerrainAtlas()->regions[floorData.texAtlasRegionId].top, World::TerrainAtlas()->regions[floorData.texAtlasRegionId].bottom };

				//DebugLogInfo("Pushing ", vertexIndex);

				vertices.resize(vertices.size() + floatsPerVertex * 4);
				for (unsigned int x = 0; x < 2; x++) {
					for (unsigned int z = 0; z < 2; z++) {
						//DebugLogInfo("Writing position ", i + x, " ", j + z);

						// positions
						vertices[floatIndex + params.meshVertexFormat->attributes.position->offset / sizeof(GLfloat)] = i + x;
						vertices[floatIndex + params.meshVertexFormat->attributes.position->offset / sizeof(GLfloat) + 1] = floorData.yOffset;
						vertices[floatIndex + params.meshVertexFormat->attributes.position->offset / sizeof(GLfloat) + 2] = j + z;

						// UVs
						vertices[floatIndex + params.meshVertexFormat->attributes.textureUV->offset / sizeof(GLfloat)] = hUvs[x];
						vertices[floatIndex + params.meshVertexFormat->attributes.textureUV->offset / sizeof(GLfloat) + 1] = vUvs[z];
						vertices[floatIndex + params.meshVertexFormat->attributes.textureZ->offset / sizeof(GLfloat)] = floorData.texArrayZ;

						// normals
						//vertices[floatIndex + params.meshVertexFormat->attributes.normal->offset / sizeof(GLfloat)] = 0;
						//vertices[floatIndex + params.meshVertexFormat->attributes.normal->offset / sizeof(GLfloat) + 1] = 1;
						//vertices[floatIndex + params.meshVertexFormat->attributes.normal->offset / sizeof(GLfloat) + 2] = 0;

						// tangents
						//vertices[floatIndex + params.meshVertexFormat->attributes.tangent->offset / sizeof(GLfloat)] = 1;
						//vertices[floatIndex + params.meshVertexFormat->attributes.tangent->offset / sizeof(GLfloat) + 1] = 0;
						//vertices[floatIndex + params.meshVertexFormat->attributes.tangent->offset / sizeof(GLfloat) + 2] = 0;

						floatIndex += floatsPerVertex;
					}
				}
			}

			if (tile.layers[TileLayer::Furniture] != -1) {
				const auto& furnitureData = GetFurnitureData(tile.layers[TileLayer::Furniture]);
				
				if (furnitureData.gameobjectMaker.has_value()) {
					(*furnitureData.gameobjectMaker)({ i + centerPos.x - radius, j + centerPos.y - radius}, objects);
				}
			}


		}
	}

	mesh = Mesh::New(RawMeshProvider(vertices, indices, params), false);
}

RenderChunk::RenderChunk(glm::ivec2 centerPos, int stride, int radius, const std::shared_ptr<Material>& material, const std::shared_ptr<TextureAtlas>& atlas):
	pos(centerPos),
	material(material),
	atlas(atlas)
{
	
	MakeMesh(centerPos, stride, radius);
	//DebugLogInfo("Mesh ", mesh->vertices.size());

	//TestSphere(pos.x, 4, pos.y, false);
	//mesh = CubeMesh();
	//DebugLogInfo("Created at ",  mesh->vertices.size());

	auto params = GameobjectCreateParams({ComponentBitIndex::Render, ComponentBitIndex::Collider, ComponentBitIndex::Transform});
	params.materialId = material ? material->id: 0;
	params.meshId = mesh->meshId;
	mainObject = GameObject::New(params);
	mainObject->RawGet<ColliderComponent>()->SetCollisionLayer(COLLISION_LAYER);
	mainObject->RawGet<TransformComponent>()->SetPos(glm::dvec3(centerPos.x - 0.5, 0, centerPos.y - 0.5));
	mainObject->RawGet<TransformComponent>()->SetScl(mesh->originalSize);
}

RenderChunk::~RenderChunk() {
	//DebugLogInfo("Destroyed at ", pos);

	//if (mainObject) {
		mainObject->Destroy();
		mainObject = nullptr;
	//}
	
	for (auto& o : objects) {
		o->Destroy();
	}
	objects.clear();
	Mesh::Unload(mesh->meshId);
}
