#pragma once
#include "health.hpp"
#include "gameobjects/component_registry.hpp"

// Anything that needs AI should inherit from this class.
class Creature {
public:
	Body body;
	std::shared_ptr<GameObject> gameObject;

	// affects combat ability
	Attribute reactionTime;

	std::shared_ptr<Creature> New(const std::shared_ptr<Mesh>& mesh, const Body& b);
	
	virtual ~Creature(); // virtual destructor required to ensure correct deletion by shared_ptr of subclasses

	static void UpdateAll(float dt);

protected:

	Creature(const std::shared_ptr<Mesh>& mesh, const Body& b);

	// Makes the creature update its AI and do stuff and all that.
	virtual void Think(float dt);

private:
	static inline std::vector<std::shared_ptr<Creature>> creatures;
};