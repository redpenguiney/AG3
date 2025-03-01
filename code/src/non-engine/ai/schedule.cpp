#include "schedule.hpp"

bool WorkBlock::Evaluate(const WorkGroupConditionEvaluationArgs& args) const
{
	for (auto& vec : conditions) {
		for (auto& c : vec) {
			if (!c(args)) {
				break;
			}
		}
		return true;
	}

	return false;
}

const WorkBlock& Schedule::Evalute(const Humanoid& humanoid) const
{
	WorkGroupConditionEvaluationArgs args{
		.humanoid = &humanoid
	};
	
	for (auto& block : workBlocks) {
		if (block.Evaluate(args))
			return block;
	}

	static WorkBlock fallbackBlock{
		.allowedWork = {
			.allowedWorkGroups = true,
			.allowLeisure = true
		},
	};
	return fallbackBlock;
}
