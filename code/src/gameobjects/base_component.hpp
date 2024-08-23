#pragma once

// // A pointer to a component that gameobjects use to make sure the component is actually used by the gameobject.
// i don't know why this exists
// template<typename T>
// class ComponentHandle {

// };

// Component pool needs components to have a few fields, so this makes sure it does.
// TODO: how to make this compatible with free list memory optimization?

//template<typename T>
//class ComponentPool;

//template<typename T>
struct BaseComponent {
    public: // not private bc i couldn't make it work
    
    // component group information is stored in the gameobject itself

    // indicates that the component is in use by a gameobject and not just sitting around in the object pool.
    // If this is set to false, the systems of ECS will skip this component when iterating through all components
    //bool live;
    //T* next; // pointer to next available component in pool, or nullptr if this component is alive.
    //unsigned int componentPoolId; // index into pools vector
    //ComponentPool<T>* pool; // TODO: could probably find a way to get rid of the index if we have this?

    // exists to let RenderComponentNoFO avoid type safety.
    // void* should be ptr to component pool
    //void SetPool(void* p) {
        //pool = (ComponentPool<T>*)p;
    //}
};