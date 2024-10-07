#pragma once
#include <memory>
#include <tuple>

class Mesh;
class Material;

std::shared_ptr<Mesh> CubeMesh();
std::shared_ptr<Mesh> SphereMesh();
std::pair<float, std::shared_ptr<Material>> GrassMaterial();
std::pair<float, std::shared_ptr<Material>> BrickMaterial();

void TestCubeArray(int x, int y, int z, bool physics);
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

