#pragma once
#include <functional>
#include <memory>
#include <array>

constexpr int NUM_WORK_GROUPS = 16;

class Humanoid;

// TODO: allowed physical areas (for example to enforce curfews/taking shelter)?
struct AllowedWork {
	std::array<bool, NUM_WORK_GROUPS> allowedWorkGroups;
	bool allowLeisure;
};

struct WorkGroupConditionEvaluationArgs {
	const Humanoid* humanoid; // mayn't be nullptr
};

namespace WorkBlockConditions {

}

class WorkBlock {
public:
	

	// if this returns true, then the work block should be executed.
	bool Evaluate(const WorkGroupConditionEvaluationArgs&);

	// for each vector of functions, if all of them return true it passes.
		// basically the inner vectors are joined by ANDs and the outer vectors by ORs
	std::vector<std::vector<std::function<bool(const WorkGroupConditionEvaluationArgs&)>>> conditions;

	AllowedWork allowedWork;
};

// A set of work blocks comprising a schedule.
// For each individual npc, it evaluates each work block in order and selects the first one whose call to Evaluate() returns true. 
	// The npc can then only perform tasks/leisure allowed by that work block.
// // If no work block in the schedule passes, it will return one allowing all actions. 
// Use by shared_ptr.
class Schedule {
public:
	static std::shared_ptr<Schedule> New();

	// Returns what work block the npc should consider when deciding what to do.	
		
	const WorkBlock& Evalute(const Humanoid&);

	// Modifying this vector may invalidate the reference returned by Evaluate, but other than that, this may be modified as you please.
	std::vector<WorkBlock> workBlocks;
};