#pragma once
#include "tile_data.hpp"
#include <vector>
#include "debug/assert.hpp"

std::vector<TileData>& Data() {
	static std::vector<TileData> data;
	return data;
}

int RegisterTileData(TileData tile) {
	DebugLogInfo("Registering ", Data().size());
	Data().push_back(tile);
	return Data().size() - 1;
}

const TileData& GetTileData(int id) {
	Assert(id < Data().size());
	return Data()[id];
}