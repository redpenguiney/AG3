#pragma once
#include "creature.hpp"

class Task;
class Schedule;

// An intelligent creature with a human body.
class Humanoid: public Creature {
public:
	static std::shared_ptr<Humanoid> New();
	virtual ~Humanoid();

	// (charisma), determines effectiveness of conversation
	Attribute rizz;

	// general knowledge, assists in conversation and etc. idk lol todo
	Attribute wisdom;

	void SetSchedule(std::shared_ptr<Schedule>& schedule);
	const std::shared_ptr<Schedule>& GetSchedule();

protected:
	Humanoid(std::shared_ptr<Mesh> mesh);

	void Think(float dt) override;

private:
	// It would be too expensive to evaluate the best task for every single humanoid every frame;
		// instead, each humanoid just evaluates a handful of options and the one they're currently doing every frame and picks the best one.
		// this index keeps track of where they are in iterating through all tasks. (not the index of currentTask; currentTask isn't even in the task array if it's in here)
	int currentTaskIndex = -1;
	std::unique_ptr<Task> currentTask;

	std::shared_ptr<Schedule> schedule;

	static constexpr int TASKS_PER_FRAME = 100;

	friend class Task;
};