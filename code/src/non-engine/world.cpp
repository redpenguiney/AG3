#include "world.hpp"
#include <debug/assert.hpp>
#include "graphics/gengine.hpp"

TerrainTile GetTile(int x, int z)
{
    
}

void World::Generate()
{
    Assert(loaded == nullptr);
    loaded = std::unique_ptr<World>(new World());
}

void World::Unload() {
    loaded = nullptr;
    
}

World::~World() {
    preRenderConnection = nullptr;
}

World::World() {
    world.Split(world.Root());

    climate.resize(256);
    biomes.resize(256 * 256);
    unsigned int climateIndex = 0;
    unsigned int biomeIndex = 0;

    world.ForEach([&climateIndex, &biomeIndex, this](ChunkTree::Node& node, int depth) {
        
        if (depth == 1) { // climate
            node.data = climateIndex++;
            //climate[node.data]. 

            world.Split(node); 
        }
        else if (depth == 2) { // biome
            node.data = biomeIndex++;
            
        }
    });

    preRenderConnection = GraphicsEngine::Get().preRenderEvent->ConnectTemporary([](float) {
        Assert(loaded != nullptr);
        
        

        // determine which chunks must be updated
        std::unordered_set<glm::ivec2> chunkLocations; // in world space, in chunks

        for (auto& loader : loaded->chunkLoaders) {
            for (int x = loader.centerPosition.x - loader.radius / 2; x <= loader.centerPosition.x + loader.radius / 2; x++) {
                for (int y = loader.centerPosition.y - loader.radius / 2; y <= loader.centerPosition.y + loader.radius / 2; y++) {
                    chunkLocations.insert({ x, y });
                }
            }
        }

        // update those chunks
    });
}