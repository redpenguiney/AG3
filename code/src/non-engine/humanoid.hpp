#pragma once
#include "creature.hpp"

// An intelligent creature with a human body.
class Humanoid: public Creature {
public:
	Humanoid(std::shared_ptr<Mesh> mesh);
	virtual ~Humanoid();

	// (charisma), determines effectiveness of conversation
	Attribute rizz;

	// general knowledge, assists in conversation and etc. idk lol todo`
	Attribute wisdom;

	
	

protected:
	void Think(float dt) override;
};