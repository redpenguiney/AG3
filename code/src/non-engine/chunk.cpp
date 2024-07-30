#include "chunk.hpp"
#include <algorithm>

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



Chunk::Chunk(glm::vec3 centerPos, unsigned int lodLevel)
{

}

void Chunk::Update()
{
	if (dirty) {

	}
}

void Chunk::MarkChanged() {
	dirty = true;
}