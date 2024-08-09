#pragma once
#include "graphics/mesh.hpp"
#include <gameobjects/component_registry.hpp>
#include "utility/hash_glm.hpp"

const unsigned int MAX_LOD_LEVELS = 4;
const float MAX_CHUNK_SIZE = 32 * powf(2, MAX_LOD_LEVELS - 1); // size of the lowest-detail possible chunk
const float MAX_CHUNK_RESOLUTION = 1 * powf(2, MAX_LOD_LEVELS - 1); // resolution of the lowest-detail possible chunk; meters per sample
unsigned int MAX_LOADED_CHUNKS_PER_FRAME = 4;

// returns pairs of <lodLevel, relPos> for chunks.
//std::vector <std::pair<unsigned int, glm::vec3 >> CalculateChunkLoadOrder();
// pairs of <lodLevel, relPos>
//const static inline std::vector <std::pair<unsigned int, glm::vec3 >> CHUNK_LOAD_ORDER = CalculateChunkLoadOrder();

struct OctreeNode;

class Chunk {
public:
	// defined in world_loader.cpp. loads/unloads chunks, call every frame.
	static void LoadWorld(glm::vec3 cameraPos, float minBaseLodDistance, unsigned int baseGridSize);

	// center position
	const glm::vec3 pos;

	// higher is more detail, maximum is MAX_LOD_LEVELS - 1. Size of the chunk is MAX_CHUNK_SIZE * 2 ^ (-lod)
	const unsigned int lod;

	float Size();

	Chunk(glm::vec3 centerPos, unsigned int lodLevel);
	~Chunk();
	Chunk(const Chunk&) = delete;

	// remeshes the chunk if it's dirty
	void Update();

	// mark the chunk as changed
	void MarkChanged();

	

private:
	static void RecursiveLoad(OctreeNode*);

	static inline std::unordered_map<glm::vec3, std::unique_ptr<Chunk>> chunks;

	bool toDelete;

	// true if mesh needs to be modified
	bool dirty;

	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<GameObject> object;
};