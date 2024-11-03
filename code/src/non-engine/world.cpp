#include "world.hpp"
#include <debug/assert.hpp>
#include "graphics/gengine.hpp"
#include "tile_data.hpp"
#include <tests/gameobject_tests.hpp>

std::unique_ptr<World>& World::Loaded()
{
    static std::unique_ptr<World> world = nullptr;
    return world;
}

void World::Generate()
{
    Assert(Loaded() == nullptr);
    Loaded() = std::unique_ptr<World>(new World());
}

void World::Unload() {
    Loaded() = nullptr;
    
}

TerrainTile World::GetTile(int x, int z)
{
    TerrainTile t;
    t.furniture = -1;
    if (x > 0) {
        t.floor = World::DIRT;
    }
    else {
        t.floor = World::ROCK;
    }
    return t;
}

World::~World() {
    preRenderConnection = nullptr;
}

World::World() {
    world.Split(world.rootNodeIndex);

    climate.resize(256);
    biomes.resize(256 * 256);
    unsigned int climateIndex = 0;
    unsigned int biomeIndex = 0;

    world.ForEach([&climateIndex, &biomeIndex, this](int nodeIndex, int depth) {
        
        if (depth == 1) { // climate
            world.GetNode(nodeIndex).data = climateIndex++;
            //climate[node.data]. 

            world.Split(nodeIndex); 
        }
        else if (depth == 2) { // biome
            world.GetNode(nodeIndex).data = biomeIndex++;
            
        }
    });

    preRenderConnection = GraphicsEngine::Get().preRenderEvent->ConnectTemporary([this](float) {
        Assert(Loaded() != nullptr);
        
        

        // determine which chunks must be updated
        std::unordered_set<glm::ivec2> chunkLocations; // in world space, in chunks

        for (auto& loader : Loaded()->chunkLoaders) {
            for (int x = loader.centerPosition.x - loader.radius / 2; x <= loader.centerPosition.x + loader.radius / 2; x++) {
                for (int y = loader.centerPosition.y - loader.radius / 2; y <= loader.centerPosition.y + loader.radius / 2; y++) {
                    chunkLocations.insert({ x, y });
                }
            }
        }

        // update those chunks


        auto atlas = std::shared_ptr<TextureAtlas>(new TextureAtlas());

        // determine which chunks must be rendered
        auto& cam = GraphicsEngine::Get().GetMainCamera();
        glm::vec3 topLeft = cam.ProjectToWorld(glm::vec2(0, 0), glm::ivec2(GraphicsEngine::Get().window.width, GraphicsEngine::Get().window.height));
        topLeft *= cam.position.y / topLeft.y;
        glm::vec3 bottomRight = -topLeft;
        glm::ivec3 roundedTopLeft = glm::floorMultiple(glm::dvec3(topLeft) + cam.position, glm::dvec3(16.0f));
        glm::ivec3 roundedBottomRight = glm::ceilMultiple(glm::dvec3(bottomRight) + cam.position, glm::dvec3(16.0f));
        //Assert(roundedTopLeft.y == 0 && roundedBottomRight.y == 0);

        /*for (int x = roundedTopLeft.x - 8; x <= roundedTopLeft.x + 8; x += 16) {
            for (int y = roundedTopLeft.z - 8; y <= roundedTopLeft.z + 8; y += 16) {
                if (!renderChunks.count({ x, y })) {
                    renderChunks[glm::ivec2(x, y)] = std::unique_ptr<RenderChunk>(new RenderChunk(glm::ivec2(x, y), 1, 8, GrassMaterial().second, atlas));
                }
                renderChunks[{x, y}]->dead = false;
            }
        }*/

        if (!renderChunks.count({ 0, 0 })) {
            renderChunks[glm::ivec2(0, 0)] = std::unique_ptr<RenderChunk>(new RenderChunk(glm::ivec2(0, 0), 8, 1, GrassMaterial().second, atlas));
            
        }
        renderChunks[{0, 0}]->dead = false;

        // render
        for (auto it = renderChunks.begin(); it != renderChunks.end(); it++) {
            auto& [_, c] = *it;
            if (c->dead) {
                it = renderChunks.erase(it);
            }
            else {
                c->dead = true;
            }
            
        }
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