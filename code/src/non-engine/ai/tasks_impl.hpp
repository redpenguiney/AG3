#pragma once
#include "tasks.hpp"
#include <non-engine/world.hpp>

class WorldProgressBar;

struct ChangeTileTaskInfo {
	TileLayer layer; // which layer of the tile is being changed
	int newType; // id of what the tile will be changed into

	float baseTimeToComplete; // note that various modifiers will affect the actual time to completion
};

class ChangeTileTask : public Task {
public:
	const ChangeTileTaskInfo& info;
	const glm::ivec2 pos;

	// tilePos is in world space.
	ChangeTileTask(glm::ivec2 tilePos, const ChangeTileTaskInfo& info);

	virtual bool Progress(Humanoid& executor, float dt) override;
	virtual void Interrupt() override;
	virtual int EvaluateTaskUtility(const Humanoid& potentialExecutor) override;

private:
	std::shared_ptr<WorldProgressBar> progressBar;
	float progress; // 0 - info.baseTimeToComplete; not neccesarily in seconds since rate of completion varies
	
	// TODO: with current caching scheme they'll still potentially try to repath to unreachable tasks every frame
	std::unique_ptr<Path> pathCache = nullptr;
	glm::ivec2 cachedPathOrigin;
	int cachedPathIndex = 0;
	int cachedPathCostModifier = 0;
};