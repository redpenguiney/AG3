#include "tasks.hpp"
#include <glm/geometric.hpp>

TaskScheduler& TaskScheduler::Get()
{
	static TaskScheduler s;
	return s;
}

int Task::TaskDistanceUtilityPenalty(const glm::ivec2& p1, const glm::ivec2& p2)
{
	auto d = (p2 - p1);
	d *= d;
	return std::max(-100000, -(d.x + d.y));
}
