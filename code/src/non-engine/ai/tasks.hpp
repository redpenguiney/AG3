#pragma once
#include "schedule.hpp"
#include <array>

// A single unit of work that can be carried out by a specific person.
// If all crewmates complete all their tasks, they win.
class Task {
	
};

class TaskScheduler {
	// key is work group
	// vector indices not stable
	std::array<std::vector<Task>, NUM_WORK_GROUPS> tasks;
};

