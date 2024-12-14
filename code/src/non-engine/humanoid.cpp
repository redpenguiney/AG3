#include "humanoid.hpp"
#include "ai/schedule.hpp"
#include "ai/tasks.hpp"

Humanoid::Humanoid(std::shared_ptr<Mesh> mesh):
	Creature(mesh, Body::Humanoid())
{

}

Humanoid::~Humanoid()
{
}

void Humanoid::Think(float dt)
{
	// is there something i desperately need to do which supersedes the schedule?


	// if not, determine work group and pick a task
	auto& group = schedule->Evalute(*this);

	int bestTaskUtility = currentTaskIndex == -1 ? -1 : EvaluteTaskUtility(currentTaskIndex);
	int bestTaskIndex = currentTaskIndex;

	int start = currentTaskIndex + 1; // works if -1
	while (currentTaskIndex < start + TASKS_PER_FRAME) {

	}

	if (bestTaskUtility == -1) { // then no task was found
		DebugLogError("Failed to find a task for humanoid.");
	}
	else {
		currentTaskIndex = bestTaskIndex;
		auto& task = *TaskScheduler::Get().tasks[bestTaskIndex];
		if (task.Progress(*this)) // then we finished the task (or it's no longer valid)
			currentTaskIndex = -1; 
	}
}

int Humanoid::EvaluteTaskUtility(int taskIndex)
{
	Assert(taskIndex >= 0);

	return -1;
}
