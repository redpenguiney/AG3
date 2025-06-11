#include "tasks_impl.hpp"
#include "../humanoid.hpp"
//#include "tests/gameobject_tests.hpp"
#include "../ui_helpers.hpp"

#pragma warning( disable : 26495) // uninitialized variable warning; will only be read if pathCache exists, in which case it has already been set
ChangeTileTask::ChangeTileTask(glm::ivec2 tilePos, const ChangeTileTaskInfo& info):
	Task(),
	info(info),
	pos(tilePos),
	progressBar(nullptr),
	progress(0)
{
	//DebugPlacePointOnPosition({ pos.x, 3, pos.y });
	//TestBillboardUi({ tilePos.x, 2, tilePos.y }, "Bob");
	
}

bool ChangeTileTask::Progress(Humanoid& executor, float dt)
{
	//DebugLogInfo("Progressing task, executor at ", executor.gameObject->RawGet<TransformComponent>()->Position());

	// Is humanoid within range?
	glm::ivec2 hPos = executor.Pos();
	if (std::abs(hPos.x - pos.x) + std::abs(hPos.y - pos.y) > 1) { // then no; have them move to the task
		//if (pathCache && executor.Pos() == cachedPathOrigin)
			//executor.MoveTo(std::move(pathCache));
		//else
			executor.MoveTo(pos);
		progressBar = nullptr;

		return false;
	}
	else {
		executor.StopMoving();
		if (!progressBar) progressBar = WorldProgressBar::New({ pos.x, 2, pos.y });

		float WORK_SPEED = 1.0f; // TODO
		progress += dt * WORK_SPEED;
		//DebugLogInfo("work ", this);
		//DebugLogInfo("WORK ", progress);

		progressBar->SetProgress(progress / info.baseTimeToComplete);

		if (progress >= info.baseTimeToComplete) {
			
			World::Loaded()->SetTile(pos.x, pos.y, info.layer, info.newType);

			return true; // we finished
		}
		else
			return false; // we're not done
	}
}

int ChangeTileTask::EvaluateTaskUtility(const Humanoid& potentialExecutor)
{
	constexpr ComputePathParams params{};

	if (!pathCache) {
		
		pathCache = World::Loaded()->ComputePath(potentialExecutor.Pos(), pos, params);
		if (!pathCache) return -1; // TODO: some way to cache no path existing
		cachedPathOrigin = potentialExecutor.Pos();
		cachedPathIndex = 0;
		cachedPathCostModifier = 0;
	}
	else if (potentialExecutor.Pos() != cachedPathOrigin) {
		if (cachedPathIndex < pathCache->wayPoints.size() && pathCache->wayPoints.at(cachedPathIndex) == potentialExecutor.Pos()) { // try to reuse path if they're following it
			cachedPathOrigin = pathCache->wayPoints[cachedPathIndex];
			cachedPathIndex++;
			cachedPathCostModifier -= World::Loaded()->GetMoveCost(cachedPathOrigin.x, cachedPathOrigin.y);
			
		}
		else {
			pathCache = World::Loaded()->ComputePath(potentialExecutor.Pos(), pos, params);
			cachedPathOrigin = potentialExecutor.Pos();
		}
	}

	int distancePenalty = pathCache->totalMoveCost + cachedPathCostModifier;
	DebugLogInfo("Evaluated task ", this, " utility, distance penalty ", distancePenalty);
	return BASE_TASK_PRIORITY - std::min(BASE_TASK_PRIORITY, distancePenalty);; //+ TaskDistanceUtilityPenalty(pos, potentialExecutor.gameObject->RawGet<TransformComponent>()->Position());
}

void ChangeTileTask::Interrupt() {
	 // trivial
}