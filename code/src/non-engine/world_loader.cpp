#include "chunk.hpp"
#undef NDEBUG
#include "debug/assert.hpp"

// we do an adaptive octree to decide which chunks to load at which LOD.
// derived from https://courses.cs.duke.edu/cps124/fall02/notes/12_datastructures/lod_terrain.html although this obviously isn't a heightmap
struct OctreeNode {

	// nullptr if no children, otherwise array of length 8
	OctreeNode* children = nullptr;

	// center position
	glm::vec3 position;

	// equivalent to its depth in the octree; higher is more detail, maximum is MAX_LOD_LEVELS - 1. Size of the chunk is MAX_CHUNK_SIZE * 2^(-lod)
	int lod = 0;


	
	
	float Size() {
		return MAX_CHUNK_SIZE * powf(2, -lod);
	}

	void Split() {
		Assert(children == nullptr);
		Assert(lod != MAX_LOD_LEVELS);
		children = new OctreeNode[8];
		for ( float x = 0; x < 2; x ++) {
			for (float y = 0; y < 2; y ++) {
				for (float z = 0; z < 2; z ++) {
					children[int(x * 4 + y * 2 + z)].lod = lod + 1;
					children[int(x * 4 + y * 2 + z)].position = position + (glm::vec3(x - 0.5, y - 0.5, z - 0.5) * Size());
				}
			}
		}
	}

	OctreeNode() = default;
	OctreeNode(const OctreeNode&) = delete;
	~OctreeNode() {
		if (children != nullptr) {
			delete [8] children;
		}
	}
};

void RecursiveSplit(OctreeNode* n, unsigned int targetDepth, float splitDistanceSquared, glm::vec3 cameraPos) {
	if (n->lod == targetDepth - 1) { // split node's lod level will be targetDepth
		if (glm::length2(cameraPos - n->position) <= splitDistanceSquared) {
			n->Split();
		}
	}
	else {
		if (n->children != nullptr) {
			for (unsigned int i = 0; i < 8; i++) {
				RecursiveSplit(&n->children[i], targetDepth, splitDistanceSquared, cameraPos);
			}
		}
	}
}

void Chunk::RecursiveLoad(OctreeNode* n, int& loadedSoFar) {
	if (loadedSoFar == MAX_LOADED_CHUNKS_PER_FRAME) {
		return;
	}

	if (n->children) {
		for (unsigned int i = 0; i < 8; i++) {
			RecursiveLoad(&n->children[i], loadedSoFar);
		}
	}
	else {
		// see if there's already a chunk here
		if (GetChunks().count(n->position)) {
			auto& c = GetChunks().at(n->position);

			// if the chunk has the same lod as this node, we keep it.
			if (c->lod == n->lod) {
				c->toDelete = false;
				return;
			}
			// otherwise, the chunk goes bye bye and is replaced with a new one at the proper lod.
			else {
				GetChunks().erase(n->position);
				// (chunk emplaced outside the if statement)
			}
		}
		// if not, make one
		GetChunks().emplace(std::make_pair(n->position, std::make_unique<Chunk>(n->position, n->lod)));
		loadedSoFar++;
	}
}

std::unordered_map<glm::vec3, std::unique_ptr<Chunk>>& Chunk::GetChunks()
{
	static std::unordered_map<glm::vec3, std::unique_ptr<Chunk>> chunks;
	return chunks;
}

constexpr int BASE_GRID_SIZE = 2;
void Chunk::LoadWorld(glm::vec3 cameraPos, float minBaseLodDistance) {

	
	
	// mark all existing chunks for deletion
	for (auto& [pos, c] : GetChunks()) {
		(void)pos;
		c->toDelete = true;
	}

	// build octree array around camera
	OctreeNode nodes[BASE_GRID_SIZE * 2 + 1][BASE_GRID_SIZE * 2 + 1][BASE_GRID_SIZE * 2 + 1];
	for (int x = -BASE_GRID_SIZE; x <= BASE_GRID_SIZE; x++) {
		for (int y = -BASE_GRID_SIZE; y <= BASE_GRID_SIZE; y++) {
			for (int z = -BASE_GRID_SIZE; z <= BASE_GRID_SIZE; z++) {
				nodes[x + BASE_GRID_SIZE][y + BASE_GRID_SIZE][z + BASE_GRID_SIZE].position = glm::roundMultiple(cameraPos, glm::vec3(MAX_CHUNK_SIZE)) + MAX_CHUNK_SIZE * glm::vec3(x, y, z);
				//DebugLogInfo("Rounded c is ", glm::to_string(glm::roundMultiple(cameraPos, glm::vec3(MAX_CHUNK_SIZE)) + MAX_CHUNK_SIZE * glm::vec3(x, y, z)));
			}
		}
	}
	
	// split octree nodes based on distance from camera (closer = get split more)
	unsigned int depth = 0;
	for (unsigned int depth = 0; depth < MAX_LOD_LEVELS; depth++) {

		float splitThresholdSquared = powf(minBaseLodDistance * powf(2, -float(depth)), 2);

		for (unsigned int ix = 0; ix < BASE_GRID_SIZE * 2 + 1; ix++) {
			for (unsigned int iy = 0; iy < BASE_GRID_SIZE * 2 + 1; iy++) {
				for (unsigned int iz = 0; iz < BASE_GRID_SIZE * 2 + 1; iz++) {
					RecursiveSplit(&nodes[ix][iy][iz], depth, splitThresholdSquared, cameraPos);
				}
			}
		}
	}

	// load chunks based on octree nodes
	for (unsigned int depth = 0; depth < MAX_LOD_LEVELS; depth++) {

		float splitThresholdSquared = powf(minBaseLodDistance * powf(2, -float(depth)), 2);

		int loadedSoFar = 0;
		for (unsigned int ix = 0; ix < BASE_GRID_SIZE * 2 + 1; ix++) {
			for (unsigned int iy = 0; iy < BASE_GRID_SIZE * 2 + 1; iy++) {
				for (unsigned int iz = 0; iz < BASE_GRID_SIZE * 2 + 1; iz++) {
					RecursiveLoad(&nodes[ix][iy][iz], loadedSoFar);
				}
			}
		}
	}

	// lastly delete every chunk that remained marked for deletion. (RecursiveLoad() will have unmarked chunks that should have stayed loaded)
	std::erase_if(GetChunks(), [](const auto& item) {
		return item.second->toDelete;
	});

}