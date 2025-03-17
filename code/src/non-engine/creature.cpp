#include "creature.hpp"
#include "graphics/mesh.hpp"
#include "world.hpp"
#include <gameobjects/lifetime.hpp>

GameobjectCreateParams creatureParams({ ComponentBitIndex::Render, ComponentBitIndex::Transform, ComponentBitIndex::Collider, ComponentBitIndex::Rigidbody, /*ComponentBitIndex::Animation*/});

GameobjectCreateParams GetCreatureCreateParams(unsigned int mId) {
	auto & p = creatureParams;
	p.meshId = mId;
	return p;
}

double Creature::GetMoveSpeed()
{
	return 1.0;
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
	auto g = DebugPlacePointOnPosition({ worldPos.x, 2, worldPos.y });
	//g->Destroy();
	//NewObjectLifetime(g, 0.1);
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
		// validate current path (TODO: what if a better path appears?)
		if (!currentPath.has_value() || currentPath->wayPoints.empty() || currentPath->wayPoints.back() != Pos()) {
			currentPath = std::make_optional(World::Loaded()->ComputePath(Pos(), currentGoal, ComputePathParams()));
			currentPathWaypointIndex = 0;
		}
		//DebugLogInfo("Nodes ", path.wayPoints.size());
		if (currentPath->wayPoints.size() < 2) return;
		auto nextPos = glm::dvec3(currentPath->wayPoints[currentPathWaypointIndex].x, gameObject->RawGet<TransformComponent>()->Position().y, currentPath->wayPoints[currentPathWaypointIndex].y);
		currentPathWaypointIndex++;
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
	else
		currentPath = std::nullopt;
}