#pragma once
#include "creature.hpp"

class Schedule;

// An intelligent creature with a human body.
class Humanoid: public Creature {
public:
	std::shared_ptr<Humanoid> New();
	virtual ~Humanoid();

	// (charisma), determines effectiveness of conversation
	Attribute rizz;

	// general knowledge, assists in conversation and etc. idk lol todo`
	Attribute wisdom;

	void SetSchedule(std::shared_ptr<Schedule>& schedule);
	const std::shared_ptr<Schedule>& GetSchedule();

protected:
	Humanoid(std::shared_ptr<Mesh> mesh);

	void Think(float dt) override;

private:

	std::shared_ptr<Schedule> schedule;
};