@pragma once
#include "humanoid.hpp"

class Player : public Humanoid {
public:
	Player();
	~Player();

protected:
	Think();
};