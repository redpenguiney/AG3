#include "creature.hpp"
#include "graphics/mesh.hpp"
#include "world.hpp"
#include <gameobjects/lifetime.hpp>
#include "tile_data.hpp"

GameobjectCreateParams creatureParams({ ComponentBitIndex::Render, ComponentBitIndex::Transform, ComponentBitIndex::Collider, ComponentBitIndex::Rigidbody, /*ComponentBitIndex::Animation*/});

GameobjectCreateParams GetCreatureCreateParams(unsigned int mId) {
	auto & p = creatureParams;
	p.meshId = mId;
	return p;
}

double Creature::GetMoveSpeed()
{
	return 100.0;
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
	if (not currentGoal or *currentGoal != glm::dvec2(worldPos)) {
		StopMoving();
		currentGoal = worldPos;
	}
	//auto g = DebugPlacePointOnPosition({ worldPos.x, 2, worldPos.y });
	//g->Destroy();
	//NewObjectLifetime(g, 0.1);
}

void Creature::StopMoving() {
	//currentGoal = Pos();
	//DebugLogInfo("Stopped");
	currentGoal = std::nullopt;
	currentPath = std::nullopt;
	currentPathWaypointIndex = -1;
}



Creature::Creature(const std::shared_ptr<Mesh>& mesh, const Body& b) :
	Entity(mesh, GetCreatureCreateParams(mesh->meshId)),
	body(b),
	currentPathWaypointIndex(-1)
	//gameObject(GameObject::New(GetCreatureCreateParams(mesh->meshId)))
{
	currentGoal = std::nullopt;
}

void Creature::Think(float dt) {
	// First, unstuck if needed
	int moveCost = World::Loaded()->GetMoveCost(Pos().x, Pos().y);
	if (moveCost < 0) {

	}

	body.Update(dt);

	// Movement
	if (currentGoal && currentGoal != ExactPos()) {
		// validate current path (TODO: what if a better path appears?)
		if (!currentPath.has_value() || currentPath->wayPoints.empty() /*|| currentPath->wayPoints.back() != Pos()*/) {
			if (currentPath) {
				//DebugLogInfo("Reset ", currentPath->wayPoints.size());
			}
			else {
				//DebugLogInfo("Reset bru");
			}
			currentPath = std::make_optional(World::Loaded()->ComputePath(Pos(), *currentGoal, ComputePathParams()));
			currentPathWaypointIndex = 0; 
			if (!currentPath.has_value()) DebugLogInfo("No path");
		}
		//DebugLogInfo("Nodes ", path.wayPoints.size());
		//currentPathWaypointIndex = 0;
		if (currentPath->wayPoints.size() < 1) return;
		//do {
			//currentPathWaypointIndex++;
		//} while (currentPathWaypointIndex < currentPath->wayPoints.size() && currentPath->wayPoints[currentPathWaypointIndex] != Pos());
		

		double movesLeft = dt * GetMoveSpeed();
		while (movesLeft > 0 && currentPathWaypointIndex < currentPath->wayPoints.size()) {

			auto nextPos = currentPath->wayPoints[currentPathWaypointIndex]; //glm::dvec3(currentPath->wayPoints[currentPathWaypointIndex].x, gameObject->RawGet<TransformComponent>()->Position().y, currentPath->wayPoints[currentPathWaypointIndex].y);
			auto t = DebugPlacePointOnPosition(glm::dvec3(currentPath->wayPoints[currentPathWaypointIndex].x, gameObject->RawGet<TransformComponent>()->Position().y, currentPath->wayPoints[currentPathWaypointIndex].y));
			t->Destroy();
			NewObjectLifetime(t, 0.1);

			//Assert(path.wayPoints[1] != Pos());
			glm::dvec2 moveDir = glm::dvec2(nextPos) - ExactPos();
			//Assert(moveDir.x != 0 || moveDir.y != 0);
			int moveCost = World::Loaded()->GetMoveCost(Pos().x, Pos().y);
			//Assert(moveCost > 0);
			double dist = glm::length(moveDir) * moveCost;
			if (dist > movesLeft) {
				//DebugLogInfo("oh? ", currentPathWaypointIndex, " of ", currentPath->wayPoints.size());
				double percent = abs(movesLeft / dist);
				movesLeft = 0;
				gameObject->RawGet<TransformComponent>()->SetPos(gameObject->RawGet<TransformComponent>()->Position() + glm::normalize(glm::dvec3(moveDir.x, 0, moveDir.y)) * 0.1);
			}
			else {
				//DebugLogInfo("we're not done here");
				movesLeft -= dist;
				gameObject->RawGet<TransformComponent>()->SetPos(glm::dvec3(nextPos.x, gameObject->RawGet<TransformComponent>()->Position().y, nextPos.y)); 
				currentPathWaypointIndex++;
			}

			
		}

		if (currentPathWaypointIndex >= currentPath->wayPoints.size()) {
			StopMoving();
			//DebugLogInfo("Pathing succesfful.");
		}
	}
	else {
		currentPath = std::nullopt;
		//DebugLogInfo("Yeeted path")
	}
}