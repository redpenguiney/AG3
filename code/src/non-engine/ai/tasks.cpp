#include "tasks.hpp"

TaskScheduler& TaskScheduler::Get()
{
	static TaskScheduler s;
	return s;
}
