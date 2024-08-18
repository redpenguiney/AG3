#include "worldgen.hpp"
//#include "noise/noise.h"

//noise::module::Perlin perlinNoiseGenerator;

float CalcWorldHeightmap(glm::vec3 pos)
{
	float height = 1; //* //perlinNoiseGenerator.GetValue(pos.x / 32.0f, pos.z / 32.0f, 1);
	return pos.y - height;
}
