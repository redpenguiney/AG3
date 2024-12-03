#pragma once
#include "health.hpp"
#include "entity.hpp"

// Anything that needs AI should inherit from this class.
class Creature: public Entity {
public:
	Body body;
	

	// affects combat ability
	Attribute reactionTime;

	static std::shared_ptr<Creature> New(const std::shared_ptr<Mesh>& mesh, const Body& b);
	
	virtual ~Creature(); // virtual destructor required to ensure correct deletion by shared_ptr of subclasses

protected:

	Creature(const std::shared_ptr<Mesh>& mesh, const Body& b);

	// Makes the creature update its AI and do stuff and all that.
	virtual void Think(float dt) override;
};