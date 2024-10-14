#include "worldgen.hpp"
#include "noise/noise.h"

inline noise::module::Perlin perlinNoiseGenerator;

float Noise(float x, float y, float z, float amplitude, float frequency) {
	return perlinNoiseGenerator.GetValue(x / frequency, y / frequency, z / frequency) * amplitude;
}

float CalcWorldHeightmap(glm::vec3 pos)
{
	float height = Noise(pos.x, pos.z, pos.y, 32.0, 64.0) + Noise(pos.x, pos.z, pos.y, 8.0, 16.0);
	return pos.y - height;
}
