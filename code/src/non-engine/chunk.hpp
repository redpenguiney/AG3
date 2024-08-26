#pragma once
#include "graphics/mesh.hpp"
#include <gameobjects/gameobject.hpp>
#include "utility/hash_glm.hpp"


const inline unsigned int MAX_LOD_LEVELS = 1;
const inline float MAX_CHUNK_SIZE = 32 * powf(2, MAX_LOD_LEVELS - 1); // size of the lowest-detail possible chunk
const inline float MAX_CHUNK_RESOLUTION = 4 * powf(2, MAX_LOD_LEVELS - 1); // resolution of the lowest-detail possible chunk; meters per sample
const inline unsigned int MAX_LOADED_CHUNKS_PER_FRAME = 4000000;

// returns pairs of <lodLevel, relPos> for chunks.
//std::vector <std::pair<unsigned int, glm::vec3 >> CalculateChunkLoadOrder();
// pairs of <lodLevel, relPos>
//const static inline std::vector <std::pair<unsigned int, glm::vec3 >> CHUNK_LOAD_ORDER = CalculateChunkLoadOrder();

struct OctreeNode;

class Chunk {
public:
	// defined in world_loader.cpp. loads/unloads chunks, call every frame.
	static void LoadWorld(glm::vec3 cameraPos, float minBaseLodDistance);

	// center position
	const glm::vec3 pos;

	// higher is more detail, must be less than MAX_LOD_LEVELS. Size of the chunk is MAX_CHUNK_SIZE * 2 ^ (-lod)
	const int lod;

	float Size();

	Chunk(glm::vec3 centerPos, unsigned int lodLevel);
	~Chunk();
	Chunk(const Chunk&) = delete;

	// remeshes the chunk if it's dirty
	void Update();

	// mark the chunk as changed
	void MarkChanged();

	

private:
	static void RecursiveLoad(OctreeNode*, int& loadedSoFar);

	// exists because when the chunks unordered_map was a static variable, the undefined destruction order caused issues
	static std::unordered_map<glm::vec3, std::unique_ptr<Chunk>>& GetChunks();

	bool toDelete;

	// true if mesh needs to be modified
	bool dirty;

	// nullptr when no mesh bc empty
	std::shared_ptr<Mesh> mesh;

	// nullptr when no mesh bc empty
	std::shared_ptr<GameObject> object;
};