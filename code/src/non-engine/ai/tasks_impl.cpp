#include "tasks_impl.hpp"
#include "../humanoid.hpp"
//#include "tests/gameobject_tests.hpp"
#include "../ui_helpers.hpp"

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
	if (std::abs(hPos.x - pos.x) + std::abs(hPos.y - pos.y) > 1) { // then no; have them pathfind over
		executor.MoveTo(pos);
		progressBar = nullptr;
		return false;
	}
	else {
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
	return BASE_TASK_PRIORITY + TaskDistanceUtilityPenalty(pos, potentialExecutor.gameObject->RawGet<TransformComponent>()->Position());
}

void ChangeTileTask::Interrupt() {
	 // trivial
}