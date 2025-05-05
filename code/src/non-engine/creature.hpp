#pragma once
#include "health.hpp"
#include "entity.hpp"
#include "world.hpp"

// Anything that needs AI should inherit from this class.
class Creature: public Entity {
public:
	Body body;
	
	// in tiles/second
	double GetMoveSpeed();

	// affects combat ability
	Attribute reactionTime;

	static std::shared_ptr<Creature> New(const std::shared_ptr<Mesh>& mesh, const Body& b);
	
	virtual ~Creature(); // virtual destructor required to ensure correct deletion by shared_ptr of subclasses

	// Make the creature start trying to move towards this destination. Will pathfind as neccesary.
	void MoveTo(glm::ivec2 worldPos);

	void StopMoving();

protected:

	Creature(const std::shared_ptr<Mesh>& mesh, const Body& b);

	// Makes the creature update its AI and do stuff and all that.
	virtual void Think(float dt) override;

private:
	std::optional<Path> currentPath = std::nullopt;
	glm::dvec2 currentGoal; // where creature is currently tryna pathfind to
	int currentPathWaypointIndex;
};