#include "creature.hpp"
#include "graphics/mesh.hpp"

GameobjectCreateParams creatureParams({ ComponentBitIndex::Render, ComponentBitIndex::Transform, ComponentBitIndex::Collider, ComponentBitIndex::Rigidbody, ComponentBitIndex::Animation });

GameobjectCreateParams GetCreatureCreateParams(unsigned int mId) {
	auto & p = creatureParams;
	p.meshId = mId;
	return p;
}

std::shared_ptr<Creature> Creature::New(const std::shared_ptr<Mesh>& mesh, const Body& b)
{
	auto ptr = std::shared_ptr<Creature>(new Creature(mesh, b));
	Entities().push_back(ptr);
	GameObjectsToEntities()[gameObject.get()] = ptr;
	return ptr;
}

Creature::~Creature()
{
}



Creature::Creature(const std::shared_ptr<Mesh>& mesh, const Body& b) :
	Entity(mesh, GetCreatureCreateParams(mesh->meshId)),
	body(b)
	//gameObject(GameObject::New(GetCreatureCreateParams(mesh->meshId)))
{

}

void Creature::Think(float dt) {
	body.Update(dt);
}