#pragma once
#include <memory>
#include <string>
#include <optional>
#include "gameobjects/gameobject.hpp"

class Mesh;

// Stores intrinsic properties of a tile that are the same for every instance of that tile (appearance, max health, movement penalities, etc.)
struct TileData {
	std::string displayName = "Unknown tile";
	std::optional<GameobjectCreateParams> gameobject = std::nullopt; // If non-existent, object will just be a quad (or cube if yOffset != 0) textured based on texAtlasRegionId.
	
	int texAtlasRegionId = 0; // which part of the texture atlas textures this tile, should never be negative
	float texArrayZ = -1;

	float yOffset = 0.0;
	int id = -1; // value assigned when RegisterTile() is called

};

// Stores intrinsic properties of a furniture. Furniture is placed on top of tiles and can store metadata unlike tiles.

struct FurnitureData {
	std::string displayName = "Unknown furniture";
	std::optional<GameobjectCreateParams> gameobject = std::nullopt; // If non-existent, furniture will be invisible.

	float yOffset = 0.0;
	int id = -1; // value assigned when RegisterTile() is called
};

// Registers the given tile data so that it can be returned by GetTileData().
// Returns tile id.
int RegisterTileData(TileData tile);

// Throws if the id doesn't correspond to a real tile.
const TileData& GetTileData(int id);

// Registers the given furniture data so that it can be returned by GetTileData().
// Returns tile id.
int RegisterFurnitureData(FurnitureData furniture);

// Throws if the id doesn't correspond to a real tile.
const FurnitureData& GetFurnitureData(int id);