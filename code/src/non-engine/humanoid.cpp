#include "humanoid.hpp"
#include "ai/schedule.hpp"
#include "ai/tasks.hpp"
#include <tests/gameobject_tests.hpp>

void Humanoid::SetSchedule(std::shared_ptr<Schedule>& schedule)
{
	this->schedule = schedule;
}

const std::shared_ptr<Schedule>& Humanoid::GetSchedule()
{
	return schedule;
}

Humanoid::Humanoid(std::shared_ptr<Mesh> mesh):
	Creature(mesh, Body::Humanoid())
{

}

std::shared_ptr<Humanoid> Humanoid::New()
{
	return std::shared_ptr<Humanoid>(new Humanoid(CubeMesh())); // TODO: make_shared?
}

Humanoid::~Humanoid()
{
}

void Humanoid::Think(float dt)
{
	auto& scheduler = TaskScheduler::Get();

	// is there something i desperately need to do which supersedes the schedule?


	// if not, determine work group and pick a task
	auto& group = schedule->Evalute(*this);

	int bestTaskUtility = currentTaskIndex == -1 ? -1 : scheduler.tasks[currentTaskIndex]->EvaluateTaskUtility(*this);
	int bestTaskIndex = currentTaskIndex;
	int oldTaskIndex = currentTaskIndex;

	int i = 0; // works if -1
	while (i++ < TASKS_PER_FRAME) {
		currentTaskIndex++;
		if (currentTaskIndex > (scheduler.tasks.size()))
			currentTaskIndex = 0;
		int utility = scheduler.tasks[currentTaskIndex]->EvaluateTaskUtility(*this);
		if (utility > bestTaskUtility) {
			bestTaskUtility = utility;
			bestTaskIndex = currentTaskIndex;
		}
	}

	if (oldTaskIndex != -1 && oldTaskIndex != bestTaskIndex)
		scheduler.tasks[oldTaskIndex]->Interrupt();

	if (bestTaskUtility == -1) { // then no task was found
		DebugLogError("Failed to find a task for humanoid.");
	}
	else {
		currentTaskIndex = bestTaskIndex;
		auto& task = *scheduler.tasks[bestTaskIndex];
		if (task.Progress(*this, dt)) // then we finished the task (or it's no longer valid)
			currentTaskIndex = -1; 
	}
}

//int Humanoid::EvaluteTaskUtility(int taskIndex)
//{
//	Assert(taskIndex >= 0);
//
//	return -1;
//}
