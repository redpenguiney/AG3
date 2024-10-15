#pragma once
#include <memory>
#include <tuple>
#include <glm/vec3.hpp>

class Mesh;
class Material;

std::shared_ptr<Mesh> CubeMesh();
std::shared_ptr<Mesh> SphereMesh();
std::pair<float, std::shared_ptr<Material>> GrassMaterial();
std::pair<float, std::shared_ptr<Material>> BrickMaterial();

void TestCubeArray(glm::uvec3 stride, glm::uvec3 start, glm::uvec3 dim, bool physics, glm::vec3 scale = {1, 1, 1});
void TestSphere(int x, int y, int z, bool physics);
void TestBrickWall();
void TestGrassFloor();
void TestVoxelTerrain();

void TestSpinningSpotlight();
void TestStationaryPointlight();

// leaks memory
void TestUi();

void TestAnimation();

void TestGarticMusic();

