#pragma once
#include <array>
#include <vector>
#include <list>
#include <memory>
#include <glm/vec3.hpp>
#include "events/event.hpp"
#include "tile_chunk.hpp"
#include "utility/hash_glm.hpp"
#include <unordered_set>

class Entity;

struct ComputePathParams {

};

struct Path {
	// move in straight line between waypoints
	std::vector<glm::ivec2> wayPoints;
};

enum class TileLayer {
	Floor, // a tile, either solid or liquid, potentially with a roof. 
	Furniture, // either a wall or something like a tree
	NumTileLayers // the number of tile layers
};


struct TerrainTile {
	
	std::array<int, static_cast<size_t>(TileLayer::NumTileLayers)> layers;

};

struct TerrainChunk {
	TerrainChunk(glm::ivec2 position);
	//TerrainChunk(std::array<std::array<TerrainTile, 16>, 16> tiles);

	std::array<std::array<TerrainTile, 16>, 16> tiles;
	//std::vector<std::unique_ptr<Entity>> entities;

	// TODO
	//bool modified = false; // if 
};

struct ClimateTile {
	float typicalTemperature; // degrees Kelvin
	float temperatureDispersion; // average absolute deviation from typical temperature; how much it varies
	float typicalHumidity; // absolute humidity (g water/m^3)
	float humidityDisperion; // average absolute deviation from typical humidity; how much it varies
};
//using ClimateChunk = std::array<std::array<ClimateTile, 16>, 16>;

struct BiomeTile {
	enum class Biome {

	};

	Biome id;

	// meters above sea level
	float altitude;
};
//using BiomeChunk = std::array<std::array<BiomeTile, 16>, 16>;

struct WeatherTile {
	float cloudCover; // in range [0-1], percentage f
	float rainfall; // m/hr 
	glm::vec3 wind; // m/s

};

// Generates terrain and simulates AI in the chunks around it.
struct ChunkLoader {
	int radius; // in tiles, must be multiple of 16
	glm::ivec2 centerPosition; // in tiles, but must be a chunk position
};

class World {
public:
	struct TerrainIds {
		// Tile ids; always have defined value.
		int DIRT, ROCK, GRASS, SNOW, MISSING;

		// Furniture ids; always have defined value.
		int TREE;

		TerrainIds();
	};
	static TerrainIds& TERRAIN_IDS();

	// returns the currently loaded world.
	static std::unique_ptr<World>& Loaded();

	static std::shared_ptr<TextureAtlas>& TerrainAtlas();

	static std::shared_ptr<Material>& TerrainMaterial();

	// TODO: list performance is suspect
	std::list<ChunkLoader> chunkLoaders;

	// creates a new world from scratch
	// requires that no world is currently loaded.
	static void Generate();

	// unloads the currently loaded world. (does nothing if no loaded world)
	static void Unload();

	
	Path ComputePath(glm::ivec2 origin, glm::ivec2 goal, ComputePathParams params);

	TerrainTile GetTile(int x, int z);

	// returns the chunk that the given coordinates lie inside
	TerrainChunk& GetChunk(int x, int z); 

	~World();

private:
	std::unique_ptr<Event<float>::Connection> preRenderConnection = nullptr;
	std::unique_ptr<Event<float>::Connection> prePhysicsConnection = nullptr;

	// returns center positions of all chunks within range of a chunk loader
	std::unordered_set<glm::ivec2> GetLoadedChunks();

	// depth 0 = root (16 million m)^2
		// holds the 256 (1 Mm)^2 chunks for climate simulation 
	// depth 1 = (1 million m)^2
	// depth 2 = chunks for biome and altitude determination (65536m)^2
	// depth 3 = chunks for weather determination (4096m)^2
	// depth 4 = (256m)^2
	// depth 5 = chunks for AI simulation, hold terrain tiles (16m)^2
	// Note that for most of the above things (climate, biome, weather, altitude) we then interpolate.
	// Chunks for depth 1 and 2 are immediately generated, chunks for depths 3-5 are made on demand.
	//ChunkTree world;

	// indices into terrain for chunks that should always be loaded (like those around player built structures)
	//std::vector<unsigned int> forceLoadedChunks;

	// 16m by 16m, stores terrain/buildings/etc., AI simulated at this level
	std::unordered_map<glm::ivec2, std::unique_ptr<TerrainChunk>> terrain;

	// 65536m by 65536m biome tiles
	std::unordered_map<glm::ivec2, std::unique_ptr<BiomeTile>> biomes;

	// each tile is 1Mm by 1Mm, climate simulation done between these 
	std::array<std::array<std::unique_ptr<ClimateTile>, 16>, 16> climate;

	// unique_ptr because unordered_map::emplace was bullying me
	// key is position
	std::unordered_map<glm::ivec2, std::unique_ptr<RenderChunk>> renderChunks; 

	World();
	
};
