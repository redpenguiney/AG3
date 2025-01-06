#include "chunk.hpp"
#include "worldgen.hpp"
#include <algorithm>
#include <tests/gameobject_tests.hpp>

//std::vector<std::pair<unsigned int, glm::vec3>> CalculateChunkLoadOrder()
//{
//    std::vector<std::pair<unsigned int, glm::vec3>> ret;
//
//    float maxRenderDistance = 0; // the farthest distance that should have chunks loaded for it
//    float maxLodDistances[MAX_TERRAIN_LOD_LEVELS]; // for each lod, the farthest distance that should use that lod
//    for (unsigned int lod = 0; lod < MAX_TERRAIN_LOD_LEVELS; lod++) {
//        maxRenderDistance += BASE_CHUNK_SIZE * (lod + 1);
//        maxLodDistances[lod] = maxRenderDistance;
//    }
//
//    for (float x = -maxRenderDistance; x <= maxRenderDistance; x += BASE_CHUNK_SIZE) {
//        for (float y = -maxRenderDistance; y <= maxRenderDistance; y += BASE_CHUNK_SIZE) {
//            for (float z = -maxRenderDistance; z <= maxRenderDistance; z += BASE_CHUNK_SIZE) {
//
//                glm::vec3 position(x, y, z);
//                float distance = glm::length(position);
//
//                if (distance > maxRenderDistance) { continue; }
//
//                // the lowest-quality lod whose grid the point (x, y, z) fits on
//                unsigned int positionMaxLod = MAX_TERRAIN_LOD_LEVELS;
//                for (unsigned int i = 0; i < 3; i++) {
//                    unsigned int lod = 0;
//                    for (unsigned int lodLevel = 0; lodLevel < MAX_TERRAIN_LOD_LEVELS; lodLevel++) {
//                        float f = position[i] / 32.0f / powf(2, lodLevel);
//                        if (floor(f)) {
//                            lod++;
//                        }
//                        else {
//                            break;
//                        }
//                    }
//                    positionMaxLod = std::min(lod, positionMaxLod);
//                }
//
//                // find the highest lod level that accepts chunks at this distance
//                for (unsigned int lod = 0; lod < MAX_TERRAIN_LOD_LEVELS; lod++) {
//
//                    // if this lod's grid size isn't aligned with (x, y, z) then don't bother
//                    if (positionMaxLod < lod) {
//                        break;
//                    }
//
//                    if (distance > maxLodDistances[lod]) {
//                        // then a chunk goes here
//                        ret.push_back(std::make_pair(lod, position));
//                        break;
//                    }
//                }
//            }
//        }
//    }
//
//    // now we've calculated what chunks should be loaded.
//    // however, we need to sort the ret vector by distance so that the closest chunks are loaded first.
//    std::sort(ret.begin(), ret.end(), 
//        [](const std::pair<unsigned int, glm::vec3>& a, const std::pair<unsigned int, glm::vec3>& b) {
//            return glm::length(a.second) < glm::length(b.second);
//        });
//
//    return ret;
//}

void TerrainInputProvider(Material* material, std::shared_ptr<ShaderProgram> program) {
	float tile = 4.0f;
	program->Uniform("textureRepeat", 1.0f/tile);
	glm::vec3 cPos = GraphicsEngine::Get().GetCurrentCamera().position;
	cPos.x = fmodf(cPos.x, tile);
	cPos.y = fmodf(cPos.y, tile);
	cPos.z = fmodf(cPos.z, tile);

	program->Uniform("envLightDirection", glm::normalize(glm::vec3(1, 1, 1)));
	program->Uniform("envLightColor", glm::vec3(0.7, 0.7, 1));
	program->Uniform("envLightDiffuse", 0.7f);
	program->Uniform("envLightAmbient", 0.2f);
	program->Uniform("envLightSpecular", 0.0f);

	program->Uniform("triplanarCameraPosition", cPos);
}

std::shared_ptr<ShaderProgram> TerrainShader() {
	static auto shader = ShaderProgram::New("../shaders/world_triplanar_vertex.glsl", "../shaders/world_triplanar_fragment.glsl");
	return shader;
}

unsigned int TerrainMaterial() {
	static auto material = Material::New(MaterialCreateParams{ 
		.textureParams = {
			TextureCreateParams({TextureSource("../textures/grass.png"),}, Texture::ColorMap)
		},
		.shader = TerrainShader(),
		.requireUniqueTextureCollection = true,
		.allowAppendaton = false,
		.inputProvider = ShaderInputProvider(TerrainInputProvider),
		
	});
	return material.second->id;
}

GameobjectCreateParams MakeCGOParams(int meshId) {
	GameobjectCreateParams p({ComponentBitIndex::Render, ComponentBitIndex::Transform, ComponentBitIndex::Collider});
	p.meshId = meshId;
	p.materialId = TerrainMaterial();
	return p;
}

MeshCreateParams meshParams { .meshVertexFormat = MeshVertexFormat::DefaultTriplanarMapping(), .expectedCount = 1 };

Chunk::Chunk(glm::vec3 centerPos, unsigned int lodLevel):
	pos(centerPos),
	lod(lodLevel),

	toDelete(false),
	dirty(true),

	mesh(Mesh::New(RawMeshProvider({}, {}, (meshParams.meshVertexFormat->primitiveType = GL_TRIANGLES, meshParams)), true)),
	object(GameObject::New(MakeCGOParams(mesh->meshId)))
{

	Assert(lodLevel < MAX_LOD_LEVELS);

	// scale and position are correct. Don't change them.
	object->Get<TransformComponent>()->SetPos(pos);
	//object->Get<TransformComponent>()->SetScl(glm::vec3(Size() + Resolution() ));
	//object->Get<TransformComponent>()->SetScl(glm::vec3(1, 1, 1));
	object->Get<TransformComponent>()->SetScl(glm::vec3(Size() /*+ Resolution() / 2.0f*/));

	object->Get<RenderComponent>()->SetColor({ 0.5, 0.7, 0.5, 1.0 });
	//object->Get<RenderComponent>()->SetTextureZ(-1);

	//object->Get<RenderComponent>()->SetColor(glm::vec4(glm::vec3(Resolution()/4.0f) + pos/10.0f + 0.3f, 1.0));

	//if (int(pos.y) > 0) {
		//object->Get<RenderComponent>()->SetColor(glm::vec4(1, 0, 0, 0));
	//}

	Update();
}

Chunk::~Chunk()
{

	if (mesh) {
		object->Destroy();
		object = nullptr;

		Mesh::Unload(mesh->meshId);
		mesh = nullptr;
	}
	
}

float Chunk::Size() {
	return MAX_CHUNK_SIZE * powf(2, -lod);
}

float Chunk::Resolution() const
{
	return MAX_CHUNK_RESOLUTION * powf(2, -lod);
}

void Chunk::Update()
{
	if (dirty) {
		DualContouringMeshProvider provider(meshParams);
		provider.point1 = pos -Size() / 2.0f; // 2.0f - Resolution() / 2.0f;
		provider.point2 = pos +Size() / 2.0f; // 2.0f + Resolution() / 2.0f;
		provider.fixVertexCenters = false;
		provider.resolution = Resolution();
		provider.distanceFunction = CalcWorldHeightmap;
		//provider.distanceFunction = [](glm::vec3 pos) {
			//return pos.y;
			//};
		
		//DebugLogInfo("Chunk at ", glm::to_string(pos), " size ", Size());
		mesh->Remesh(provider, true);

		if (mesh->indices.size() > 0) {
			DebugLogInfo("Scl ", mesh->originalSize, " Size ", Size(), " Pos ", pos, " had ", mesh->indices.size());
			Assert(mesh->originalSize == glm::vec3(Size()));
		}
		
		//object->Get<TransformComponent>()->SetScl(mesh->originalSize);
		//if (lod != 0) {
			//DebugLogInfo("Meshed from ", glm::to_string(provider.point1), " to ", glm::to_string(provider.point2));
			//DebugLogInfo("Chunk at ", glm::to_string(object->RawGet<TransformComponent>()->Position()), " size ", object->RawGet<TransformComponent>()->Scale(), " verts ", mesh->indices.size(), " lod ", lod);
		//}
	}
}

void Chunk::MarkChanged() {
	dirty = true;
}