#include "tests/graphics_test.hpp"
#include <vector>
#include <string>
#include <tests/physics_test.hpp>

void GameInit(std::vector<std::string> args) {
	//TestGraphics();
	TestPhysics();
}

void GameClose() {}