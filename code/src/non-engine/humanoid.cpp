#include "humanoid.hpp"
#include "ai/schedule.hpp"

Humanoid::Humanoid(std::shared_ptr<Mesh> mesh):
	Creature(mesh, Body::Humanoid())
{

}

Humanoid::~Humanoid()
{
}

void Humanoid::Think(float dt)
{
	// is there something i desperately need to do which supersedes the schedule?


	// if not, determine work group 
	auto& group = schedule->Evalute(*this);
}
