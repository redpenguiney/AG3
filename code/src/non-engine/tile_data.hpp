#pragma once
#include <memory>
#include <string>

class Mesh;

// Stores intrinsic properties of a tile that are the same for every instance of that tile (appearance, max health, movement penalities, etc.)
struct TileData {
	std::string displayName = "???";
	std::shared_ptr<Mesh> mesh = nullptr; // When a tile is registered, is made a square mesh if nothing provided.
	
	float yOffset = 0.0;
	int id = -1; // value assigned when RegisterTile() is called

};

// Registers the given tile data so that it can be returned by GetTileData().
// Returns tile id.
int RegisterTileData(TileData tile);

// Throws if the id doesn't correspond to a real tile.
const TileData& GetTileData(int id);