#include "texture_atlas.hpp"

TextureAtlas::Region::Region(int regionId, int x, int y, int regionWidth, int regionHeight, int atlasWidth, int atlasHeight) {
	Assert(regionId >= 0); // ids less than zero are reserved for "invisible" things
	id = regionId;
	left = float(x) / float(atlasWidth);
	right = float(x + regionWidth) / float(atlasWidth);
	top = float(y) / float(atlasHeight);
	bottom = float(y + regionHeight) / float(atlasHeight);
}