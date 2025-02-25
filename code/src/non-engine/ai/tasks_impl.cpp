#include "tasks_impl.hpp"
#include "../humanoid.hpp"

ChangeTileTask::ChangeTileTask(glm::ivec2 tilePos, const ChangeTileTaskInfo& info):
	Task(),
	info(info),
	pos(tilePos),
	progress(0)
{

}

bool ChangeTileTask::Progress(Humanoid& executor, float dt)
{
	// Is humanoid within range?
	glm::ivec2 hPos = executor.gameObject->RawGet<TransformComponent>()->Position();
	if (std::abs(hPos.x - pos.x) + std::abs(hPos.y - pos.y) > 1) { // then no; have them pathfind over
		executor.MoveTo(hPos);
		return false;
	}
	else {
		float WORK_SPEED = 1.0f; // TODO
		progress += dt * WORK_SPEED;
		if (progress >= info.baseTimeToComplete)
			return true; // we finished
		else
			return false; // we're not done
	}
}

int ChangeTileTask::EvaluateTaskUtility(const Humanoid& potentialExecutor)
{
	return BASE_TASK_PRIORITY + TaskDistanceUtilityPenalty(pos, potentialExecutor.gameObject->RawGet<TransformComponent>()->Position());
}

void ChangeTileTask::Interrupt() {
	 // trivial
}