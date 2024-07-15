#pragma once
#include <cassert>
#include <vector>

// The TextureAtlas struct describes where each individual object texture/region is on a larger image/texture. 
// The TextureAtlas stores no image data of its own and is not in itself used for rendering,
	// but rather, mesh constructors use it to create the correct UVs which the actual texture containing the atlas is rendered with.
struct TextureAtlas {
	struct Region {
		int id;
		float left;
		float top;
		float right;
		float bottom;

		// sizes/positions are in pixels. (x, y) refers to top left corner of the region.
		Region(int regionId, int x, int y, int regionWidth, int regionHeight, int atlasWidth, int atlasHeight);
	};

	std::vector<Region> regions;
};