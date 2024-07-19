#include "attribute.hpp"

Attribute::Attribute(float baseValue)
	//activeModifiers()
{
	base = baseValue;
}

Attribute::Attribute(const Attribute& original)
{
	base = original.base;
}

float Attribute::Value() {
	float baseFactor = base, multFactor = 1;

	// tick modifiers and remove them if need be.
	for (unsigned int i = 0; i < activeModifiers.size(); i++) {
		if (activeModifiers.at(i)->Tick()) {
			// a fast-remove that works on (noncopyable) unique_ptr
			activeModifiers[i].swap(activeModifiers.back());
			activeModifiers.pop_back();
			i--; // don't worry, it's ok if i underflows here
		}
		else {
			baseFactor += activeModifiers[i]->constFactor;
			multFactor *= activeModifiers[i]->multFactor;
		}
	}

	return baseFactor * multFactor;
}

Attribute::operator float()
{
	return Value();
}

void Attribute::ApplyModifier(std::unique_ptr<Modifier> mod)
{
	activeModifiers.push_back(std::move(mod));
}

void Attribute::ClearModifier(ModifierId modId)
{
	for (unsigned int i = 0; i < activeModifiers.size(); i++) {
		if (activeModifiers.at(i)->id == modId) {
			// a fast-remove that works on (noncopyable) unique_ptr
			activeModifiers[i].swap(activeModifiers.back());
			activeModifiers.pop_back();
			i--;
		}
	}
}

