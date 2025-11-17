#include "gameobject_tests.hpp"
#include <graphics/gengine.hpp>
#include <conglomerates/basic_renderer.hpp>

void TestPhysics() {
	GraphicsEngine::Get().debugFreecamEnabled = true;

	BasicRenderer::Setup();
	TestSkybox();

	//TestGrassFloor();

	TestStationaryPointlight();

	//TestBrickWall();

	TestPit();

	//TestCubeArray({ 2, 2, 2 }, { 0, 8, 0 }, { 3, 3, 3 }, true);

	for (int x = 0; x < 1; x+= 2) {
		for (int y = 3; y < 1; y+=2) {
			for (int z = 0; z < 1; z+=2) {
				TestSphere(x, y, z, true);
			}
		}
	}
}