#pragma once
#include <memory>
#include "tasks.hpp"

class Team {
public:
	Team(const Team&) = delete;

	int id;
	TaskScheduler scheduler;

private:
};