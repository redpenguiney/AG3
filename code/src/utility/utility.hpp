#pragma once

#include <glm/vec3.hpp>
#include "GLM/vec4.hpp"
#include <vector>

class GameObject;

// uses radians
glm::dvec3 LookVector(double pitch, double yaw);

// places a little cube of the requested color at the requested position (and returns that cube)
// TODO: probably shouldn't use the rainbow cube for that
std::shared_ptr<GameObject> DebugPlacePointOnPosition(glm::dvec3 position, glm::vec4 color = glm::vec4(1, 1, 1, 1));

// returns time in seconds
double Time();

// Generic class for providing reusable, consecutive object ids.
// Will never return an id of 0.
class IdProvider {
public:
	IdProvider();
	void ReturnId(unsigned int id);
	unsigned int GetId();
private:
	std::vector<unsigned int> freeIds;
	unsigned int largestId;
};