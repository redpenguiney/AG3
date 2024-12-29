#pragma once

class GameObject;

// Base class; used to initialize arbitrary component fields on gameobject creation.
class ComponentFieldInitializer {
public:
	virtual void Apply(GameObject& g) = 0;
};

template <typename FieldType> 
class FieldSetter : ComponentFieldInitializer {
	
};
