#pragma once
#include "entity.hpp"

class Client;
class GameObject;

class Character {
public:
	

private:
	Character(std::shared_ptr<Client> client);



	const std::vector<std::shared_ptr<GameObject>> gameobjects;
	const std::shared_ptr<Client> client;

	static std::vector<std::shared_ptr<Character>> characters;
};