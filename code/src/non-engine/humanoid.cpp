#include "humanoid.hpp"

Humanoid::Humanoid(std::shared_ptr<Mesh> mesh):
	Creature(mesh, Body::Humanoid())
{

}

Humanoid::~Humanoid()
{
}

void Humanoid::Think(float dt)
{
}
