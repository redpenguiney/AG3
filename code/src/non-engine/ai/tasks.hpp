#pragma once
#include "schedule.hpp"
#include <array>
#include <glm/vec2.hpp>

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
	static constexpr int BASE_TASK_PRIORITY = 100000;

	Task() = default;
	Task(const Task&) = delete;

	virtual ~Task() = default;

	// returns true if finished (successfully or not). May call Interrupt()
	virtual bool Progress(Humanoid& executor, float dt) = 0;

	// call when task is gracelessly cut short because NPC needs to do something else now.
	virtual void Interrupt() = 0;

	// Returns integer representing the priority of the task.
	// Negative return values indicate the task may/should not be completed by this Humanoid.
	virtual int EvaluateTaskUtility(const Humanoid& potentialExecutor) = 0;

protected:
	// never returns less than -100,000 or more than 0
	// penalty is proportional to distance squared for performance reasons
	static int TaskDistanceUtilityPenalty(const glm::ivec2& p1, const glm::ivec2& p2);
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

	// indices are stable, some values are nullptr
	std::vector<std::unique_ptr<Task>> tasks;
	std::vector<unsigned int> availableTaskIndices;
};

