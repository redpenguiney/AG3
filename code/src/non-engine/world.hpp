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
	Quadtree world;
	std::vector<TerrainTile> tiles;

	World();
};
