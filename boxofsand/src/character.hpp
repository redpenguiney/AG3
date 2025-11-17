#pragma once
//#include "entity.hpp"
#include <memory>
#include <vector>

class Client;
class GameObject;

class Character {
public:
	

private:
	float health = 100;

	Character(std::shared_ptr<Client> client);

	void Update();

	const std::shared_ptr<GameObject> graphicalObject;
	const std::shared_ptr<GameObject> collider;
	const std::shared_ptr<Client> client;

	static std::vector<std::shared_ptr<Character>> characters;
};