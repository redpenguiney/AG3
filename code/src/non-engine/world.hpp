#pragma once
#include "utility/tree.hpp"
#include <array>
#include <vector>
#include <list>
#include <memory>
#include <glm/vec3.hpp>
#include "events/event.hpp"
#include "tile_chunk.hpp"
#include "utility/hash_glm.hpp"

class Entity;

struct TerrainTile {
	int floor; // a tile, either solid or liquid, potentially with a roof. 
	int furniture; // either a wall or something like a tree
};

struct TerrainChunk {
	std::array<std::array<TerrainTile, 16>, 16> tiles;
	//std::vector<std::unique_ptr<Entity>> entities;
};

struct ClimateTile {
	float temperature;
	float temperatureDispersion; 
	float humidity;
	float humidityDisperion;
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
	// TODO
};

// Generates terrain and simulates AI in the chunks around it.
struct ChunkLoader {
	int radius; // in chunks
	glm::ivec2 centerPosition; // in chunks
};

class World {
public:
	// Tile ids; always have defined value.
	static int DIRT, ROCK, GRASS, SNOW;

	// the currently loaded world.
	static inline std::unique_ptr<World> loaded = nullptr;

	// TODO: list performance is suspect
	std::list<ChunkLoader> chunkLoaders;

	// creates a new world from scratch
	// requires that no world is currently loaded.
	static void Generate();

	// unloads the currently loaded world.
	static void Unload();

	TerrainTile GetTile(int x, int z);

	// returns the chunk that the given coordinates lie inside
	TerrainChunk GetChunk(int x, int z); 

	~World();

private:
	std::unique_ptr<Event<float>::Connection> preRenderConnection;

	// depth 0 = root (16 million m)^2
		// holds the 256 (1 Mm)^2 chunks for climate simulation 
	// depth 1 = (1 million m)^2
	// depth 2 = chunks for biome and altitude determination (65536m)^2
	// depth 3 = chunks for weather determination (4096m)^2
	// depth 4 = (256m)^2
	// depth 5 = chunks for AI simulation, hold terrain tiles (16m)^2
	// Note that for most of the above things (climate, biome, weather, altitude) we then interpolate.
	// Chunks for depth 1 and 2 are immediately generated, chunks for depths 3-5 are made on demand.
	ChunkTree world;

	// indices into terrain for chunks that should always be loaded (like those around player built structures)
	std::vector<unsigned int> forceLoadedChunks;

	std::vector<TerrainChunk> terrain;
	std::vector<ClimateTile> climate;
	std::vector<BiomeTile> biomes;

	std::unordered_map<glm::ivec2, RenderChunk> renderChunks;

	World();
	
};
