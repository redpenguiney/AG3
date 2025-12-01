#include "tests/graphics_test.hpp"
#include <vector>
#include <string>
#include <tests/physics_test.hpp>
#include <physics/spatial_acceleration_structure.hpp>

void GameInit(std::vector<std::string> args) {
	//TestGraphics();
	TestPhysics();
	MousePhysicsGrab();
	//GraphicsEngine::Get().debugShowSAS = true;
}

void GameClose() {}