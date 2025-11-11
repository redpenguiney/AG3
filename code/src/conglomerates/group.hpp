#pragma once
#include <vector>

class GameObject;

// A collection of gameobjects you want to give the same lifetime and give special behaviour via provided callbacks.
class Group {
public:
	Group();
	~Group();

private:
	std::vector<GameObject> objects;
};