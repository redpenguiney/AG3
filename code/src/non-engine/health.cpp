#include "health.hpp"
#undef NDEBUG // SO ASSERTS ALWAYS FIRE
#include "debug/assert.hpp"

BodyAttribute::BodyAttribute(const BodyPart& b): limb(b)
{
}

BodyAttribute::operator float()
{
	return Value();
}

float BodyAttribute::Value()
{
	// TODO: handle pain and what not
	return Attribute::Value();
}

BodyPart::BodyPart(std::string name, float& oxygen):
	name(name),
	bodyOxygen(oxygen),
	manipulation(*this),
	gripping(*this),
	breathing(*this),
	vision(*this),
	strength(*this),
	balance(*this)
{
	parent = nullptr;
	muscleFatigue = 1;
	resilience = 10;
	condition = 1;
	health = 1;
	detachable = false;
}

//BodyPart::BodyPart(const BodyPart& original):
//	name(original.name),
//	bodyOxygen(original.bodyOxygen),
//	manipulation(original.manipulation),
//	gripping(original.gripping),
//	breathing(original.breathing),
//	vision(original.vision),
//	strength(original.strength),
//	balance(original.balance)
//{
//	parent = original.parent;
//	muscleFatigue = 1;
//	resilience = 10;
//	condition = 1;
//	health = 1;
//	detachable = false;
//
//}

Body Body::Humanoid()
{
	// see https://exrx.net/Kinesiology/Segments although i doubt that balanced

	Body b;
	std::unique_ptr<BodyPart> torso = std::make_unique<BodyPart>("torso", b.oxygen);
	torso->resilience = 54;
	BodyPart& head = torso->children.emplace_back("head", b.oxygen);
	head.parent = torso.get();
	head.resilience = 8;
	BodyPart& leftLeg = torso->children.emplace_back("left leg", b.oxygen);
	leftLeg.parent = torso.get();
	leftLeg.resilience = 17;
	BodyPart& rightLeg = torso->children.emplace_back("right leg", b.oxygen);
	rightLeg.parent = torso.get();
	rightLeg.resilience = 17;
	BodyPart& leftArm = torso->children.emplace_back("left arm", b.oxygen);
	leftArm.parent = torso.get();
	leftArm.resilience = 6;
	BodyPart& rightArm = torso->children.emplace_back("right arm", b.oxygen);
	rightArm.parent = torso.get();
	rightArm.resilience = 6;

	b.rootBodyPart = std::move(torso);
	return b;
}

Body::Body() {
	lungVolume = 60;
	oxygen = lungVolume;
	bloodRegenRate = 0.1;
	correctBloodVolume = 100;
	actualBloodVolume = correctBloodVolume;
}

Body::Body(const Body& original)
{
	oxygen = original.oxygen;
	lungVolume = original.lungVolume;
	actualBloodVolume = original.actualBloodVolume;
	correctBloodVolume = original.correctBloodVolume;
	bloodRegenRate = original.bloodRegenRate;

	// recursively copy body parts
}

void Body::Update(float dt)
{
	actualBloodVolume = std::min(dt * bloodRegenRate + actualBloodVolume, correctBloodVolume.Value());
}
