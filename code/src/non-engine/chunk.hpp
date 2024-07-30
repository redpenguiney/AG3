#pragma once
#include "graphics/mesh.hpp"
#include <gameobjects/component_registry.hpp>


const unsigned int MAX_LOD_LEVELS = 4;
const float MAX_CHUNK_SIZE = 32 * powf(2, MAX_LOD_LEVELS - 1); // size of the lowest-detail possible chunk
const float MAX_CHUNK_RESOLUTION = 1 * powf(2, MAX_LOD_LEVELS - 1); // resolution of the lowest-detail possible chunk; meters per sample
unsigned int MAX_LOADED_CHUNKS_PER_FRAME = 4;

// returns pairs of <lodLevel, relPos> for chunks.
//std::vector <std::pair<unsigned int, glm::vec3 >> CalculateChunkLoadOrder();
// pairs of <lodLevel, relPos>
//const static inline std::vector <std::pair<unsigned int, glm::vec3 >> CHUNK_LOAD_ORDER = CalculateChunkLoadOrder();



// defined in world_loader.cpp. call every frame.
void LoadWorld(glm::vec3 cameraPos, float minBaseLodDistance);

class Chunk {
public:
	static inline std::unordered_map<glm::vec3, std::unique_ptr<Chunk>> chunks;

	// lodLevel 0 = full resolution, 1 = half, etc.
	Chunk(glm::vec3 centerPos, unsigned int lodLevel);

	// remeshes the chunk if it's dirty
	void Update();

	// mark the chunk as changed
	void MarkChanged();

	

private:
	// true if mesh needs to be modified
	bool dirty;

	std::shared_ptr<GameObject> object;
	std::shared_ptr<Mesh> mesh;
};