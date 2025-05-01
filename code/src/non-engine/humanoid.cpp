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
	gameObject->name = "Humanoid";
}

std::shared_ptr<Humanoid> Humanoid::New()
{
	auto ptr = std::shared_ptr<Humanoid>(new Humanoid(CubeMesh())); // TODO: make_shared?
	GameObjectsToEntities()[ptr->gameObject.get()] = ptr;
	Entities().push_back(ptr);
	return ptr; 
}

Humanoid::~Humanoid()
{
}

void Humanoid::Think(float dt)
{
	auto& scheduler = TaskScheduler::Get();

	// is there something i desperately need to do which supersedes the schedule?


	// if not, determine work group and pick a task
	const WorkBlock& block = schedule ? schedule->Evalute(*this) : Schedule::PERMISSIVE_BLOCK;

	int bestTaskUtility = currentTask ? currentTask->EvaluateTaskUtility(*this) : -1;
	int oldP = bestTaskUtility;
	int bestTaskIndex = currentTask ? -1 : currentTaskIndex;
	int oldTaskIndex = bestTaskIndex;

	int i = 0; // works if -1
	int num = std::min(TASKS_PER_FRAME, (int)scheduler.tasks.size());
	if (!scheduler.tasks.empty()) {
		while (i++ < num) {
			currentTaskIndex++;
			if (currentTaskIndex >= (scheduler.tasks.size()))
				currentTaskIndex = 0;
			if (scheduler.tasks[currentTaskIndex] == nullptr) continue;
			int utility = scheduler.tasks[currentTaskIndex]->EvaluateTaskUtility(*this);
			if (utility > bestTaskUtility) {
				bestTaskUtility = utility;
				bestTaskIndex = currentTaskIndex;
				DebugLogInfo("Better P found");
			}
		}
	}

	if (currentTask && oldTaskIndex != bestTaskIndex) {
		currentTask->Interrupt();
		DebugLogInfo("Interrupted ", currentTask.get());
		if (scheduler.availableTaskIndices.empty()) {
			scheduler.tasks.push_back(std::move(currentTask));
		}
		else {
			scheduler.tasks[scheduler.availableTaskIndices.back()] = std::move(currentTask);
			scheduler.availableTaskIndices.pop_back();
		}
	}
	
	if (!currentTask) 
		if (bestTaskUtility == -1) { // then no task was found
			//DebugLogError("Failed to find a task for humanoid.");
		}
		else {
			currentTask = std::move(scheduler.tasks[bestTaskIndex]);
			DebugLogInfo("Commencing new task ", currentTask.get());
		}
	
	if (currentTask)
		if (currentTask->Progress(*this, dt)) { // then we finished the task (or it's no longer valid)
			currentTask = nullptr; // destroy the task since it's done or invalid
			DebugLogInfo("WE're DONE HERE");
		}
		//DebugLogInfo("PRogress task");

	Creature::Think(dt);
}

//int Humanoid::EvaluteTaskUtility(int taskIndex)
//{
//	Assert(taskIndex >= 0);
//
//	return -1;
//}
