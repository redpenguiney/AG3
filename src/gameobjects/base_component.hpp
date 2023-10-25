#pragma once
#include "component_pool.hpp"

// A pointer to a component that gameobjects use to make sure the component is actually used by the gameobject.
// See, for several components, for cache reasons every gameobject has to grab 
template<typename T>
class ComponentHandle {

};

// Component pool needs components to have a few fields, so this makes sure it does.
// TODO: how to make this compatible with free list memory optimization?
template<typename T>
struct BaseComponent {
    public: // not private bc i couldn't make it work
    
    // indicates that the component is in use.
    // If this is set to false (either because its an unused component just sitting in the pool, or a gameobject set it to false because it doesn't need it), the systems of ECS will skip this component when iterating through all components
    bool live;
    void* next; // pointer to next available component in pool, void* so it works for all types
    unsigned int componentPoolId; // index into pools vector
};