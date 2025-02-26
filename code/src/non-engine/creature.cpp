#include "creature.hpp"
#include "graphics/mesh.hpp"
#include "world.hpp"

GameobjectCreateParams creatureParams({ ComponentBitIndex::Render, ComponentBitIndex::Transform, ComponentBitIndex::Collider, ComponentBitIndex::Rigidbody, /*ComponentBitIndex::Animation*/});

GameobjectCreateParams GetCreatureCreateParams(unsigned int mId) {
	auto & p = creatureParams;
	p.meshId = mId;
	return p;
}

double Creature::GetMoveSpeed()
{
	return 0.2;
}

std::shared_ptr<Creature> Creature::New(const std::shared_ptr<Mesh>& mesh, const Body& b)
{
	auto ptr = std::shared_ptr<Creature>(new Creature(mesh, b));
	Entities().push_back(ptr);
	GameObjectsToEntities()[ptr->gameObject.get()] = ptr;
	return ptr;
}

Creature::~Creature()
{
}

void Creature::MoveTo(glm::ivec2 worldPos)
{
	currentGoal = worldPos;
}



Creature::Creature(const std::shared_ptr<Mesh>& mesh, const Body& b) :
	Entity(mesh, GetCreatureCreateParams(mesh->meshId)),
	body(b)
	//gameObject(GameObject::New(GetCreatureCreateParams(mesh->meshId)))
{
	currentGoal = Pos();
}

void Creature::Think(float dt) {
	body.Update(dt);

	if (currentGoal != Pos()) {
		// TODO: DON'T PATHFIND EVERY FRAME LOL
		const auto path = World::Loaded()->ComputePath(Pos(), currentGoal, ComputePathParams());
		//DebugLogInfo("Nodes ", path.wayPoints.size());
		if (path.wayPoints.size() < 2) return;
		auto nextPos = glm::dvec3(path.wayPoints[1].x, gameObject->RawGet<TransformComponent>()->Position().y, path.wayPoints[1].y);
		//DebugLogInfo("Next ", nextPos);
		//Assert(path.wayPoints[1] != Pos());
		auto moveDir = nextPos - gameObject->RawGet<TransformComponent>()->Position();
		auto speed = GetMoveSpeed(); // TODO: CONSIDER TILE MOVEMENT PENALTIES
		if (glm::length2(moveDir) < speed * speed) {
			gameObject->RawGet<TransformComponent>()->SetPos(nextPos);
		}
		else {
			gameObject->RawGet<TransformComponent>()->SetPos(gameObject->RawGet<TransformComponent>()->Position() + glm::normalize(moveDir) * speed * (double)dt);
		}
	}
	
}