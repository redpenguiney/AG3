#include "worldgen.hpp"
#include "noise/noise.h"

inline noise::module::Perlin perlinNoiseGenerator;

float CalcWorldHeightmap(glm::vec3 pos)
{
	float height = perlinNoiseGenerator.GetValue(pos.x / 32.0f, pos.z / 32.0f, 1);
	return pos.y - height;
}
