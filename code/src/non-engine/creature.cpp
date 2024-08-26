#include "creature.hpp"

GameobjectCreateParams creatureParams({ ComponentBitIndex::Render, ComponentBitIndex::Transform, ComponentBitIndex::Collider, ComponentBitIndex::Rigidbody, ComponentBitIndex::Animation });

GameobjectCreateParams GetCreatureCreateParams(unsigned int mId) {
	auto & p = creatureParams;
	p.meshId = mId;
	return p;
}

std::shared_ptr<Creature> Creature::New(const std::shared_ptr<Mesh>& mesh, const Body& b)
{
	return std::shared_ptr<Creature>(new Creature(mesh, b));
}

Creature::~Creature()
{
}

void Creature::UpdateAll(float dt)
{
	for (auto& c : creatures) {
		c->Think(dt);
	}
}

Creature::Creature(const std::shared_ptr<Mesh>& mesh, const Body& b) :
	body(b),
	gameObject(GameObject::New(GetCreatureCreateParams(mesh->meshId)))
{

}

void Creature::Think(float dt) {
	body.Update(dt);
}