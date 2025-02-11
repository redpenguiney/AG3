#include "world.hpp"
#include <debug/assert.hpp>
#include "graphics/gengine.hpp"
#include "tile_data.hpp"
#include <tests/gameobject_tests.hpp>
#include <noise/noise.h>
#include "entity.hpp"
#include <physics/pengine.hpp>
#include <queue>

World::TerrainIds& World::TERRAIN_IDS()
{
    static TerrainIds ids;
    return ids;
}

std::unique_ptr<World>& World::Loaded()
{
    static std::unique_ptr<World> world = nullptr;
    return world;
}

std::shared_ptr<TextureAtlas>& World::TerrainAtlas()
{
    static std::shared_ptr<TextureAtlas> atlas = std::make_shared<TextureAtlas>();
    return atlas;
}

std::shared_ptr<Material>& World::TerrainMaterial()
{
    auto p = TextureCreateParams({
        TextureSource("../textures/terrain_color_atlas.png")
    }, Texture::ColorMap);
    p.filteringBehaviour = Texture::NoTextureFiltering;
    p.mipmapBehaviour = Texture::UseNearestMipmap;
    p.mipmapGenerationMethod = Texture::AutoGeneration;
    p.wrappingBehaviour = Texture::WrapClampToEdge;

    static auto [_, texture] = Material::New(MaterialCreateParams{
        .textureParams = {
            p
        },
        .type = Texture::Texture2D,
        .requireUniqueTextureCollection = true
    });

    return texture;
}

void World::Generate()
{
    Assert(Loaded() == nullptr);
    Loaded() = std::unique_ptr<World>(new World());
}

void World::Unload() {
    Loaded() = nullptr;
    
}

Path World::ComputePath(glm::ivec2 origin, glm::ivec2 goal, ComputePathParams params)
{
    // A* pathfinding based on https://en.wikipedia.org/wiki/A*_search_algorithm 
    // However, since the map is effectively infinite we simulataneously pathfind from the origin to the goal and vice versa, using the distance between the two heads as the heuristic.

    //Path forward;
    //Path backward;
    std::unordered_map<glm::ivec2, float> nodeCosts; // key is node pos, value is cost of best currently known path to node (at index) from origin
    //std::unordered_map < glm::ivec2, float > nodeToGoalCosts; // key is node pos, value is cost of best known path from origin to goal that goes through that node
    nodeCosts[origin] = 0; // free to go to origin since you started there

    std::unordered_map<glm::ivec2, glm::ivec2> cameFrom;

    // Potential places to check next. Sorted from greatest to lowest of nodeCosts[node] + heuristic[node].
    std::vector<glm::ivec2> openSet;
    
    auto heuristic = [&goal](glm::ivec2 node) {
        // bruh glm::length doesn't work for integral types
        return 100 * std::sqrtf((goal.x - node.x) * (goal.x - node.x) + (goal.y - node.y) * (goal.y - node.y));
    };
    
    auto addNodeNeighborsToQueue = [&](glm::ivec2 node) mutable {
        constexpr std::array<glm::ivec2, 4> neighbors = { glm::ivec2 {-1, 0}, { 1, 0}, {0, 1}, {0, -1} };
        for (const auto& neighborOffset : neighbors) {
            auto neighbor = node + neighborOffset;
            auto tileMoveCost = GetTileData(GetTile(neighbor.x, neighbor.y).layers[TileLayer::Floor]).moveCost;
            auto furniture = GetTile(neighbor.x, neighbor.y).layers[TileLayer::Furniture];
            auto furnitureMoveCost = furniture < 0 ? 0 : GetFurnitureData(furniture).moveCostModifier;
            if (tileMoveCost < 0 || furnitureMoveCost < 0) { /*DebugPlacePointOnPosition(glm::dvec3(neighbor.x + 0.5, 3.0, neighbor.y + 0.5), { 1, 0, 0, 1 })*/; continue; }
            //DebugPlacePointOnPosition(glm::dvec3(neighbor.x + 0.5, 3.0, neighbor.y + 0.5), { 1, 1, 1, 0.5 });
            auto cost = nodeCosts[node] + tileMoveCost + furnitureMoveCost;
            if (!nodeCosts.contains(neighbor) || cost < nodeCosts[neighbor]) { // then we found a better way to get to this node

                nodeCosts[neighbor] = cost;
                cameFrom[neighbor] = node;

                // TODO: better time-complexity way to keep openSet sorted???
                // if this neighbor is in openSet, remove it because its cost changed and we gotta reinsert it in the right place.
                auto it = std::find(openSet.begin(), openSet.end(), neighbor);
                if (it != openSet.end())
                    openSet.erase(it);

                // regardless of whether it was in the set previously, we now need to insert it at the right place.
                auto costToEnd = cost + heuristic(neighbor);
                if (openSet.size() == 0) {
                    openSet.push_back(neighbor);
                    //DebugPlacePointOnPosition(glm::dvec3(node.x + 0.5, 3.0, node.y + 0.5), {1, 0, 1, 1});
                    //TestBillboardUi(glm::dvec3(node.x - 0.5, 3.0, node.y - 0.5), std::to_string((int)(cost)));
                }
                else {
                    for (auto it = openSet.begin(); it != openSet.end(); it++) {
                        if (nodeCosts[*it] + heuristic(*it) <= costToEnd) {
                            openSet.insert(it, neighbor);
                            goto done;
                        }
                    }
                    openSet.push_back(neighbor);
                    done:;
                }      
            }
            
        }
    };


    openSet.push_back(origin);
    nodeCosts[origin] = 0;

    int i = 0;
    while (i++ < params.maxIterations && openSet.size() > 0) {
        auto node = openSet.back();
        
        openSet.pop_back();

        
        /*int c = std::numeric_limits<int>::max();
        auto bestit = openSet.begin();
        for (auto it = openSet.begin(); it != openSet.end(); it++) {
            int betterc = nodeCosts[*it] + heuristic(*it);
            if (betterc < c) {
                bestit = it;
                c = betterc;
            }
        }
        glm::ivec2 node = *bestit;
        openSet.erase(bestit);*/

        //DebugLogInfo("Trying ", node, " set is ", openSet.size());
        


        if (node == goal) {
            // WE WIN
            Path p;
            glm::ivec2 current = goal;
            while (true) {
                p.wayPoints.push_back(current);
                if (cameFrom.contains(current)) {
                    current = cameFrom[current];
                }
                else {
                    break;
                }
            }
            std::reverse(p.wayPoints.begin(), p.wayPoints.end());
            Assert(p.wayPoints[0] == origin);
            Assert(p.wayPoints.back() == goal);

            //for (auto& node : p.wayPoints) {
            //    //TestBillboardUi(glm::dvec3(node.x - 0.5, 3.0, node.y - 0.5), std::to_string((int)(nodeCosts[node])) + std::string(" ") + std::to_string(node.x - 0.5) + std::string(", ") + std::to_string(node.y - 0.5));
            //    TestBillboardUi(glm::dvec3(node.x + 0.5, 3.0, node.y + 0.5), GetTileData(GetTile(node.x, node.y).layers[TileLayer::Floor]).displayName);
            //}

            

            return p;
        }

        addNodeNeighborsToQueue(node);
    }

    DebugLogInfo("Failed with ", i, " iterations");

    // return empty path if we ran out of iterations or we confirmed that no valid path exists.
    return Path();
    //Path combined = forward;
    //combined.wayPoints.insert(forward.wayPoints.end(), backward.wayPoints.rbegin(), backward.wayPoints.rend());
    //return combined;
}

TerrainTile World::GetTile(int x, int z)
{
    glm::ivec2 pos = glm::ivec2(x, z) - glm::floorMultiple(glm::ivec2(x, z) + 8, glm::ivec2(16)) + 8;
    return GetChunk(x, z).tiles[pos.x][pos.y];
}

const TerrainChunk& World::GetChunk(int x, int z)
{
    //DebugLogInfo("Chunk ")
    glm::ivec2 pos(x, z);
    pos = glm::floorMultiple(pos + 8, glm::ivec2(16)) + 8;
    //DebugLogInfo("Pos ", pos);

    auto& chunk = terrain[pos];

    if (chunk == nullptr) {
        chunk = std::make_unique<TerrainChunk>(pos);
    }

    return *chunk;
}

World::~World() {
    Entity::Cleanup();

    // technically pointless
    preRenderConnection = nullptr;
    prePhysicsConnection = nullptr;
}

std::unordered_set<glm::ivec2> World::GetLoadedChunks() {
    std::unordered_set<glm::ivec2> activeChunkLocations; // in world space, in tiles

    for (auto& loader : Loaded()->chunkLoaders) {
        Assert(loader.radius % 16 == 0);
        Assert((loader.centerPosition.x - 8) % 16 == 0);
        Assert((loader.centerPosition.y - 8) % 16 == 0);
        for (int x = loader.centerPosition.x - loader.radius; x <= loader.centerPosition.x + loader.radius; x++) {
            for (int y = loader.centerPosition.y - loader.radius; y <= loader.centerPosition.y + loader.radius; y++) {
                activeChunkLocations.insert({ x, y });
            }
        }
    }

    return activeChunkLocations;
}

World::World() {

    chunkLoaders.push_back(ChunkLoader(16, glm::ivec2(8, 8)));

    //world.Split(world.rootNodeIndex);

    //climate.resize(256);
    //biomes.resize(256 * 256);
    //unsigned int climateIndex = 0;
    //unsigned int biomeIndex = 0;

    //world.ForEach([&climateIndex, &biomeIndex, this](int nodeIndex, int depth) {
    //    
    //    if (depth == 1) { // climate
    //        world.GetNode(nodeIndex).data = climateIndex++;
    //        //climate[node.data]. 

    //        world.Split(nodeIndex); 
    //    }
    //    else if (depth == 2) { // biome
    //        world.GetNode(nodeIndex).data = biomeIndex++;
    //        
    //    }
    //});

    prePhysicsConnection = PhysicsEngine::Get().prePhysicsEvent->ConnectTemporary([this](float dt) {

        Assert(Loaded() != nullptr);
       

        // determine which chunks must be updated
        auto activeChunkLocations = GetLoadedChunks(); // in world space, in tiles

        for (auto& loader : Loaded()->chunkLoaders) {
            Assert(loader.radius % 16 == 0);
            Assert((loader.centerPosition.x - 8) % 16 == 0);
            Assert((loader.centerPosition.y - 8) % 16 == 0);
            for (int x = loader.centerPosition.x - loader.radius; x <= loader.centerPosition.x + loader.radius; x++) {
                for (int y = loader.centerPosition.y - loader.radius; y <= loader.centerPosition.y + loader.radius; y++) {
                    activeChunkLocations.insert({ x, y });
                }
            }
        }

        // update those chunks


        // update entities
        Entity::UpdateAll(dt);
    });

    //static std::shared_ptr<GameObject> testCorner = nullptr;

    preRenderConnection = GraphicsEngine::Get().preRenderEvent->ConnectTemporary([this](float dt) {
        //Assert(Loaded() != nullptr);

        auto activeChunkLocations = GetLoadedChunks(); // in world space, in tiles

        //// determine which chunks must be rendered
        auto& cam = GraphicsEngine::Get().GetMainCamera();
        glm::vec3 topLeft = -cam.ProjectToWorld(glm::vec2(0, 0), glm::ivec2(GraphicsEngine::Get().window.width, GraphicsEngine::Get().window.height));
        
        //DebugLogInfo(topLeft);
            
        topLeft *= cam.position.y / topLeft.y;

        ///*auto params = GameobjectCreateParams({ ComponentBitIndex::Transform, ComponentBitIndex::Render });
        //params.meshId = CubeMesh()->meshId;
        //if (testCorner) testCorner->Destroy();
        //testCorner = GameObject::New(params);
        //testCorner->RawGet<TransformComponent>()->SetPos((glm::dvec3(topLeft) + cam.position) * glm::dvec3(1, 0, 1));
        //testCorner->RawGet<RenderComponent>()->SetColor(glm::vec4(1, 1, 1, 1));
        //testCorner->RawGet<RenderComponent>()->SetTextureZ(-1);*/

        glm::vec3 bottomRight = -topLeft;
        glm::ivec3 roundedTopLeft = glm::floorMultiple(glm::dvec3(topLeft) + cam.position, glm::dvec3(16.0f));
        glm::ivec3 roundedBottomRight = glm::ceilMultiple(glm::dvec3(bottomRight) + cam.position, glm::dvec3(16.0f));
        //Assert(roundedTopLeft.y == 0 && roundedBottomRight.y == 0);

        for (int x = roundedTopLeft.x - 8; x <= roundedBottomRight.x + 8; x += 16) {
            for (int y = roundedTopLeft.z - 8; y <= roundedBottomRight.z + 8; y += 16) {
                if (!activeChunkLocations.contains({ x, y })) continue;
                if (!renderChunks.count({ x, y })) {
                    renderChunks[glm::ivec2(x, y)] = std::unique_ptr<RenderChunk>(new RenderChunk(glm::ivec2(x, y), 1, 8, TerrainMaterial(), TerrainAtlas()));
                }
                renderChunks[{x, y}]->dead = false;
            }
        }

        //if (!renderChunks.count({ 0, 0 })) {
            //renderChunks[glm::ivec2(0, 0)] = std::unique_ptr<RenderChunk>(new RenderChunk(glm::ivec2(0, 0), 1, 8, TerrainMaterial(), TerrainAtlas()));
            
        //}
        //renderChunks[{0, 0}]->dead = false;

        // render
        for (auto it = renderChunks.begin(); it != renderChunks.end(); it++) {
            auto& [_, c] = *it;
            if (c->dead) {
                it = renderChunks.erase(it);
                if (it == renderChunks.end()) break;
            }
            else {
                c->dead = true;
            }
            
        }
    });

    /*auto path = ComputePath({ -13, -13 }, { 3, -4 }, ComputePathParams());
    DebugPlacePointOnPosition(glm::dvec3(17 + 0.5, 3.0, 24 + 0.5));
    DebugPlacePointOnPosition(glm::dvec3(3 + 0.5, 3.0, -4 + 0.5));
    DebugLogInfo("path ", path.wayPoints.size());
    for (auto& waypoint : path.wayPoints) {
        DebugPlacePointOnPosition(glm::dvec3(waypoint.x + 0.5, 3.0, waypoint.y + 0.5));
    }*/

    /*for (int x = 0; x < 16; x++) {
        for (int z = 0; z < 16; z++) {
            TestBillboardUi({ x, 3, z }, GetTileData(GetTile(x, z).layers[TileLayer::Floor]).displayName);
        }
    }*/
}

int AddAtlasRegion(int x, int y) {
    //auto atlasSize = //World::TerrainMaterial()->GetTextureCollection()->textures[0]->GetSize();
    World::TerrainAtlas()->regions.push_back(TextureAtlas::Region(x * 256, y * 256, 256, 256, 1024, 1024));
    return World::TerrainAtlas()->regions.size() - 1;
}



TerrainChunk::TerrainChunk(glm::ivec2 position)
{
    static noise::module::Perlin perlinNoiseGenerator;
    static noise::module::Perlin treeNoiseGenerator;
    //perlinNoiseGenerator.SetSeed(1);
    //perlinNoiseGenerator.SetFrequency(16);
    //perlinNoiseGenerator.SetOctaveCount(3);

    int localX = 0;
    for (int worldX = position.x - 8; worldX < position.x + 8; worldX++) {
        int localZ = 0;
        for (int worldZ = position.y - 8; worldZ < position.y + 8; worldZ++) {
            float height = perlinNoiseGenerator.GetValue(worldX/16.f, worldZ/16.f, 0);
            float tree = treeNoiseGenerator.GetValue(worldX * 1.5f, worldZ * 1.5f, 0);
            

            //DebugLogInfo("Hiehgt ", height);

            tiles[localX][localZ].layers[TileLayer::Furniture] = -1;
            if (height > 0) {
                tiles[localX][localZ].layers[TileLayer::Floor] = World::TERRAIN_IDS().GRASS;

                if (tree > 0.5) {
                    tiles[localX][localZ].layers[TileLayer::Furniture] = World::TERRAIN_IDS().TREE;
                }
            }
            else {
                tiles[localX][localZ].layers[TileLayer::Floor] = World::TERRAIN_IDS().ROCK;
            }

           

            localZ++;
        }
        localX++;
    }
}

const std::vector<TerrainChunk::NavmeshNode>& TerrainChunk::GetNavmesh()
{
    //if (!navmesh.has_value()) {

    //}

    return navmesh.value();
}

void P(std::vector<Mesh::MeshRet> vec, glm::ivec2 pos, std::vector<std::shared_ptr<GameObject>>& objects) {
    auto d = GameobjectCreateParams({ ComponentBitIndex::Render, ComponentBitIndex::Transform });
    
    constexpr double SCL_FACTOR = 0.1;
    int i = 0;
    for (auto& ret : vec) {
        d.meshId = ret.mesh->meshId;
        objects.push_back(GameObject::New(d));

        objects.back()->RawGet<TransformComponent>()->SetPos(glm::dvec3(ret.posOffset) * SCL_FACTOR + glm::dvec3((double)pos.x, SCL_FACTOR * ret.mesh->originalSize.y / 2.0, (double)pos.y));
        objects.back()->RawGet<TransformComponent>()->SetRot(glm::quat(ret.rotOffset));
        objects.back()->RawGet<TransformComponent>()->SetScl(SCL_FACTOR * ret.mesh->originalSize);

        RenderComponent* render = objects.back()->MaybeRawGet<RenderComponent>();
        if (render) {
            render->SetTextureZ(-1);
            if (i == 0) {
                render->SetColor({ 0.5, 0.5, 0, 1 });
            }
            else {
                render->SetColor({ 0, 1, 0, 0.5 });
            }
            
        }
        i++;
    }
    
   
}

World::TerrainIds::TerrainIds()
{

    MISSING = RegisterTileData({
        .displayName = "???",
        .texAtlasRegionId = AddAtlasRegion(0, 0),
        .texArrayZ = 0.0f
    });
    GRASS = RegisterTileData({
        .displayName = "Grass",
        .texAtlasRegionId = AddAtlasRegion(1, 0),
        .texArrayZ = 0.0f,
        .moveCost = 100,
    });
    ROCK = RegisterTileData({
        .displayName = "Rock",
        .texAtlasRegionId = AddAtlasRegion(2, 0),
        .texArrayZ = 0.0f,
        .yOffset = 0.5,
        .moveCost = -1
    });

    TREE = RegisterFurnitureData({
        .displayName = "Anomalous Tree",
        .gameobjectMaker = [](glm::ivec2 pos, std::vector<std::shared_ptr<GameObject>>& objects) {
            static auto vec = Mesh::MultiFromFile("../models/tree.fbx");
            P(vec, pos, objects);
        },
        .moveCostModifier = -1
    });
}
