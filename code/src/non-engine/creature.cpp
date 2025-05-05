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
	StopMoving();
	currentGoal = worldPos;
	//auto g = DebugPlacePointOnPosition({ worldPos.x, 2, worldPos.y });
	//g->Destroy();
	//NewObjectLifetime(g, 0.1);
}

void Creature::StopMoving() {
	currentGoal = Pos();
	currentPath = std::nullopt;
	currentPathWaypointIndex = -1;
}



Creature::Creature(const std::shared_ptr<Mesh>& mesh, const Body& b) :
	Entity(mesh, GetCreatureCreateParams(mesh->meshId)),
	body(b),
	currentPathWaypointIndex(-1)
	//gameObject(GameObject::New(GetCreatureCreateParams(mesh->meshId)))
{
	currentGoal = Pos();
}

void Creature::Think(float dt) {
	// First, unstuck if needed
	// TODO

	body.Update(dt);

	// Movement
	if (currentGoal != ExactPos()) {
		// validate current path (TODO: what if a better path appears?)
		if (!currentPath.has_value() || currentPath->wayPoints.empty() || currentPath->wayPoints.back() != Pos()) {
			currentPath = std::make_optional(World::Loaded()->ComputePath(Pos(), currentGoal, ComputePathParams()));
			currentPathWaypointIndex = 0;
			if (!currentPath.has_value()) DebugLogInfo("No path");
		}
		//DebugLogInfo("Nodes ", path.wayPoints.size());
		if (currentPath->wayPoints.size() < 2) return;
		while (currentPathWaypointIndex < currentPath->wayPoints.size() && currentPath->wayPoints[currentPathWaypointIndex] == Pos()) {
			currentPathWaypointIndex++;
		}
		if (currentPathWaypointIndex >= currentPath->wayPoints.size()) {
			currentPath = std::nullopt;
			return;
		}
		auto nextPos = currentPath->wayPoints[currentPathWaypointIndex]; //glm::dvec3(currentPath->wayPoints[currentPathWaypointIndex].x, gameObject->RawGet<TransformComponent>()->Position().y, currentPath->wayPoints[currentPathWaypointIndex].y);
		//DebugPlacePointOnPosition(glm::dvec3(currentPath->wayPoints[currentPathWaypointIndex].x, gameObject->RawGet<TransformComponent>()->Position().y, currentPath->wayPoints[currentPathWaypointIndex].y));

		//DebugLogInfo("Next ", nextPos);
		//Assert(path.wayPoints[1] != Pos());
		glm::dvec2 moveDir = glm::dvec2(nextPos) - ExactPos();
		Assert(moveDir.x != 0 || moveDir.y != 0);
		double speed = dt * GetMoveSpeed(); // TODO: CONSIDER TILE MOVEMENT PENALTIES
		if (moveDir.x * moveDir.x + moveDir.y * moveDir.y < speed * speed) {
			gameObject->RawGet<TransformComponent>()->SetPos(glm::dvec3(nextPos.x, gameObject->RawGet<TransformComponent>()->Position().y, nextPos.y)); // TODO: needs to try and move to next waypoint here with remaining time
		}
		else {
			gameObject->RawGet<TransformComponent>()->SetPos(gameObject->RawGet<TransformComponent>()->Position() + glm::normalize(glm::dvec3(moveDir.x, 0, moveDir.y)) * speed);
		}
	}
	else
		currentPath = std::nullopt;
}