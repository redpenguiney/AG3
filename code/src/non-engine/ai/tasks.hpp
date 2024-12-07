#pragma once
#include "schedule.hpp"
#include <array>

enum class TaskType {
	Store,
	Mine,
	HarvestTree,
	HarvestCrop
};

// A single unit of work that can be carried out by a specific person.
// If all crewmates complete all their tasks, they win.
class Task {
public:
	Task(const Task&) = delete;

	// returns true if finished (successfully or not). May call Interrupt(
	virtual bool Progress(Humanoid& executor);

	// 
	virtual void Interrupt();
};

// Copyable reference to a task which enables cache-friendly iteration through the information needed for an NPC to pick which task to do.
class TaskInfo {
	Task* const task;
	TaskType type;
};

// Stores work tasks of the same priority.
struct WorkGroup {
	int priority = 100;

	// indices not stable
	std::vector<TaskInfo> tasks;
};

class TaskScheduler {
public:
	static TaskScheduler& Get();

	// key is work group id
	// vector indices not stable
	std::array<WorkGroup, NUM_WORK_GROUPS> taskInfos;

	//
	std::vector<std::unique_ptr<Task>> tasks;
};

