#pragma once
#include <vector>
#include "modifier.hpp"

// Attributes represent stuff like entity/item stats that need to support various kinds of complex modifiers.
class Attribute {
	public:
	Attribute(float baseValue = 0);
	virtual ~Attribute() = default;
	Attribute(const Attribute&); // copy will have no modifiers (TODO KINDA TROUBLING)

	// base value of the attribute.
	float base;

	// returns value of the attribute with modifers applied.
	virtual float Value();

	// implicit cast to float, returns Value().
	virtual operator float();

	// applies the given modifier.
	// takes a unique_ptr so that subclasses of Modifier work too.
	void ApplyModifier(std::unique_ptr<Modifier> mod);

	// removes all modifiers with the given id immediately.
	void ClearModifier(ModifierId modId);

	private:
	// TODO: this should be a unique_ptr. Don't copy this. Compiler refuses to accept unique ptr tho so idk.
	std::vector<std::shared_ptr<Modifier>> activeModifiers;



};