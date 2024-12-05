#pragma once
#include "schedule.hpp"
#include <array>

// A single unit of work that can be carried out by a specific person.
// If all crewmates complete all their tasks, they win.
class Task {
	
};

// Stores work tasks of the same priority.
struct WorkGroup {
	int priority = 100;

	// indices not stable
	std::vector<Task> tasks;
};

class TaskScheduler {
public:
	static TaskScheduler& Get();

private:
	// key is work group id
	// vector indices not stable
	std::array<std::vector<Task>, NUM_WORK_GROUPS> tasks;
};

