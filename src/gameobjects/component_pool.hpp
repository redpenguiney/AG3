#pragma once
#include <deque>
#include <iostream>
#include <type_traits>
#include <vector>
#include <iterator> 
#include <cstddef>
#include "base_component.hpp"

// Object pool for components (although i suppose you could use it for something besides components).
// Automatically resizes.
// Stores all objects in mostly contiguous memory for cache performance.
// Guarantees that pointers to pool contents will always remain accurate, by instead of using std::vector, using multiple arrays.
// COMPONENTS_PER_POOL template argument removed for reasons
template<typename T>
class ComponentPool {
    friend T;

    public:
        // vectors of arrays of length COMPONENTS_PER_POOL
        std::vector<T*> pools; // public because making an iterator was too much work

        ComponentPool();
        ComponentPool(const ComponentPool<T>&) = delete;

        // Returns a pointer to an uninitialized component. The fields of this component are undefined until you set them to something.
        T* GetNew();

        // Takes a pointer to a component returned by GetNew() and returns the component back to the pool.
        // Obviously, do not use the component after calling this. 
        void ReturnObject(T* component);

        ~ComponentPool();

        const static unsigned int COMPONENTS_PER_POOL = 65536;   

    private:
        // adds new pool with room for COMPONENTS_PER_POOL more objects
        void AddPool();

        std::vector<T*> firstAvailable; // for free list, first unallocated object in each pool  

                                             
};

// no .cpp because templates are dumb
template<typename T>
T* ComponentPool<T>::GetNew() {
    // We use something called a "free list" to find a component
    int poolIndex = -1;
    for (T* & ptr: pools) {
        (void)ptr; // make compiler shut up about unused variable "ptr"
        poolIndex += 1;
        if (firstAvailable[poolIndex] == nullptr) {continue;} // if the pool is full go to the next one

        T* foundObject = firstAvailable[poolIndex];
        firstAvailable[poolIndex] = (T*)foundObject->next;

        return foundObject;
    }

    // if we got this far there is no available pool
    AddPool();
    T* foundObject = firstAvailable.back();
    firstAvailable.back() = (T*)foundObject->next;

    return foundObject;
}

template<typename T>
ComponentPool<T>::ComponentPool() {
    AddPool();
}

template<typename T>
void ComponentPool<T>::ReturnObject(T* component) {
    component->next = firstAvailable[component->componentPoolId];
    firstAvailable[component->componentPoolId] = component;
}

template<typename T>
ComponentPool<T>::~ComponentPool() {
    for (auto & ptr: pools) {
        delete ptr;
    }
}

template<typename T>
void ComponentPool<T>::AddPool() {
    unsigned int index = pools.size();
    T* firstPool = new T[COMPONENTS_PER_POOL];
    firstAvailable.push_back(&(firstPool[0]));
    for (unsigned int i = 0; i < COMPONENTS_PER_POOL - 1; i++) {
        firstPool[i].next = &(firstPool[i + 1]);
        firstPool[i].componentPoolId = index;
    }
    firstPool[COMPONENTS_PER_POOL - 1].next = nullptr;
    pools.push_back(firstPool);
}