#pragma once
#include "glm/vec3.hpp"
#include <memory>

class Gui;

// Used for stuff like task completion.
class WorldProgressBar {
public:
	static std::shared_ptr<WorldProgressBar> New(glm::dvec3 initialPos);

	WorldProgressBar(const WorldProgressBar&) = delete;

	void SetPos(glm::dvec3);
	void SetProgress(float); // in range [0, 1]

private:
	WorldProgressBar(glm::dvec3 initialPos);

	glm::dvec3 pos;
	float progress; // in range [0, 1]
	std::shared_ptr<Gui> root;
};