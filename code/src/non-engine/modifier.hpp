#pragma once
#include <functional>

enum class ModifierId {

};

// Used to modify the value of an Attribute. (see attribute.hpp)
// Can and will be copied.
class Modifier {
	public:
	Modifier(const ModifierId&);

	Modifier(const Modifier&) = default;

	const ModifierId id;

	
	float constFactor = 0;
	float multFactor = 1;

	// returns true if the modifier should stop. Called before modifier is ever applied so
	//   A: if this always returns true the modifier's const/mult factors will do nothing. 
	//   B: can change const/mult factors every frame if you so please.
	// Can have side-effects like creating particles or doing damage.
	virtual bool Tick();
};