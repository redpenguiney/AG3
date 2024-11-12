#include "texture_atlas.hpp"

TextureAtlas::Region::Region(int x, int y, int regionWidth, int regionHeight, int atlasWidth, int atlasHeight) {
	//Assert(regionId >= 0); // ids less than zero are reserved for "invisible" things
	//id = regionId;

	float correctedX = float(x);
	float correctedY = float(y);


	left = float(correctedX + 1) / float(atlasWidth);
	right = float(correctedX - 1 + regionWidth) / float(atlasWidth);
	top = float(correctedY + 1) / float(atlasHeight);
	bottom = float(correctedY - 1 + regionHeight) / float(atlasHeight);
}