#include "tasks_impl.hpp"
#include "../humanoid.hpp"

ChangeTileTask::ChangeTileTask(glm::ivec2 tilePos, const ChangeTileTaskInfo& info):
	Task(),
	info(info),
	pos(tilePos),
	progress(0)
{

}

int ChangeTileTask::EvaluateTaskUtility(const Humanoid& potentialExecutor)
{
	return BASE_TASK_PRIORITY + TaskDistanceUtilityPenalty(pos, potentialExecutor.gameObject->RawGet<TransformComponent>()->Position());
}
