#include "world.hpp"
#include <debug/assert.hpp>
#include "graphics/gengine.hpp"
#include "tile_data.hpp"

TerrainTile GetTile(int x, int z)
{
    if (x > 0) {
        return TerrainTile(World::DIRT, -1);
    }
    else {
        return TerrainTile(World::ROCK, -1);
    }
}

void World::Generate()
{
    Assert(loaded == nullptr);
    loaded = std::unique_ptr<World>(new World());
}

void World::Unload() {
    loaded = nullptr;
    
}

TerrainTile World::GetTile(int x, int z)
{
    
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

        // determine which chunks must be rendered
        chunkLocations.clear();
        glm::vec2 topLeft = GraphicsEngine::Get().GetCurrentCamera().ProjectToWorld(glm::vec2(0, 0), glm::ivec2(GraphicsEngine::Get().window.width, GraphicsEngine::Get().window.height));
        glm::vec2 bottomRight = GraphicsEngine::Get().GetCurrentCamera().ProjectToWorld(glm::vec2(1, 1), glm::ivec2(GraphicsEngine::Get().window.width, GraphicsEngine::Get().window.height));
        topLeft = glm::floorMultiple(topLeft, glm::vec2(16.0f));
        bottomRight = glm::ceilMultiple(bottomRight, glm::vec2(16.0f));
        for ( )

        // render
    });
}

int World::DIRT = RegisterTileData({
    .displayName = "Dirt",
    .texAtlasRegionId = 0
});
int World::ROCK = RegisterTileData({
    .displayName = "Rock",
    .texAtlasRegionId = 1
});