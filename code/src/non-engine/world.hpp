#pragma once
#include "utility/tree.hpp"

struct TerrainTile {
	int floor; // a tile, either solid or liquid, potentially with a roof. 
	int furniture; // either a wall or something like a tree
};

class World {
public:
	// the currently loaded world.
	static inline World* loaded = nullptr;

	// creates a new world from scratch
	// requires that no world is currently loaded.
	static void Generate();

	// unloads the currently loaded world.
	static void Unload();

	

	
	TerrainTile GetTile(int x, int z);

private:
	// depth 0 = root (16 million m)^2
	// depth 1 = LOD for climate simulation (1 million m)^2
	// depth 2  (
	//
	//
	//
	// depth 6 = individual tiles 
	ChunkTree world;
	std::vector<TerrainTile> tiles;

	World();
};
