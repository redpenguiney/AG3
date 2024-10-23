#pragma once
#include <vector>
#include <memory>
#include <string>
#include "attribute.hpp"
#include "injury.hpp"

struct BodyPart;

// body parts need a special attribute type that accounts for condition/health/etc.
class BodyAttribute: public Attribute {
	public:
	BodyAttribute(const BodyPart& b);
	
	// overrides
	operator float();
	float Value();

	private:
	const BodyPart& limb;
};

struct BodyPart {
	public:
	BodyPart(std::string name, float& oxygen);

	//// copies the body part and its children. parent will be updated to also have the new copy as a child.
	//BodyPart(const BodyPart&);

	// helper function
	//void Parent(BodyPart& newParent);

	std::string name;

	std::vector<Injury> injuries;

	// non-owning, may be nullptr
	BodyPart* parent;

	// owning, never nullptrs, may be empty
	std::vector<BodyPart> children;

	// physical damage/how much of the body part is still present; range [0, 1], at 0 is either destroyed or detached
	float condition;

	// how much of the body part's tissue is still alive/not suffering from necrosis; range [0, 1]
	// stuff like frostbite, burns, hypoxia, etc. decrease this
	float health;

	// multiplier on body part's manipulation/movement abilities; range [0, 1], at 0 limb is utterly useless.
	float muscleFatigue;

	// things like a mouth are not detachable, an arm is.
	bool detachable;

	// just a reference to the containing body's oxygen level. 
	float& bodyOxygen;

	//  relevant to prostheses and stuff
	bool organic;

	// determines rate at which condition/health decreases
	Attribute resilience;

	// ability of the body part to finely manipulate objects.
	BodyAttribute manipulation;

	// ability of the body part to grip things.
	BodyAttribute gripping;

	// ability of the body part to balance, helping dodging/mvmt
	BodyAttribute balance;

	// ability of the body part to exert force (for movement, attacking, etc.)
	BodyAttribute strength;

	// ability of the body part to see things.
	BodyAttribute vision;

	// ability of the body part to breathe.
	BodyAttribute breathing;

};

// Container for body parts.
class Body {
	public:
	// factory constructor which returns a humanoid body.
	static Body Humanoid();

	// copy constructor has to copy body parts.
	Body(const Body&);
	Body();

	// probably the torso.
	std::unique_ptr<BodyPart> rootBodyPart;

	// how much oxygen is available. Range [0, lungVolume].
	float oxygen;

	// how much oxygen the body can hold.
	Attribute lungVolume;

	// how much blood is in the body.
	float actualBloodVolume;

	// how much blood is supposed to be in the body.
	Attribute correctBloodVolume;

	// per second
	Attribute bloodRegenRate;

	// dt in seconds
	void Update(float dt);

	private:
	
};