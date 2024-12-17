#pragma once
#include "tile_data.hpp"
#include <vector>
#include "debug/assert.hpp"

std::vector<TileData>& Tiles() {
	static std::vector<TileData> data;
	return data;
}

std::vector<FurnitureData>& Furniture() {
	static std::vector<FurnitureData> data;
	return data;
}

int RegisterTileData(TileData tile) {
	Assert(tile.texAtlasRegionId > -1);
	//DebugLogInfo("Registering ", Data().size());
	Tiles().push_back(tile);
	return Tiles().size() - 1;
}

const TileData& GetTileData(int id) {
	Assert(id < Tiles().size());
	return Tiles()[id];
}

int RegisterFurnitureData(FurnitureData furniture)
{
	//DebugLogInfo("Registering ", Data().size());
	Furniture().push_back(furniture);
	return Furniture().size() - 1;
}

const FurnitureData& GetFurnitureData(int id)
{
	Assert(id < Furniture().size());
	return Furniture()[id];
}
