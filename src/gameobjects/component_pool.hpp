#pragma once
#include <deque>
#include <iostream>
#include <type_traits>
#include <vector>
#include <iterator> 
#include <cstddef>
#include "base_component.cpp"

// Object pool for components (although i suppose you could use it for something besides components).
// Automatically resizes.
// Stores all objects in mostly contiguous memory for cache performance.
// Guarantees that pointers to pool contents will always remain accurate, by instead of using std::vector, using multiple arrays.
// Just set COMPONENTS_PER_POOL to 65536 if you aren't sure, no default value because if you specify it in some places but not others you get types mixed up
template<typename T, unsigned int COMPONENTS_PER_POOL>
class ComponentPool {
    friend T;

    public:
        // vectors of arrays of length COMPONENTS_PER_POOL
        std::vector<T*> pools; // public because making an iterator was too much work

        ComponentPool();
        ComponentPool(const ComponentPool<T, COMPONENTS_PER_POOL>&) = delete;

        // Returns a pointer to an uninitialized component. The fields of this component are undefined until you set them to something.
        T* GetNew();

        // Takes a pointer to a component returned by GetNew() and returns the component back to the pool.
        // Obviously, do not use the component after calling this. 
        void ReturnObject(T* component);

        ~ComponentPool();

    private:
        // adds new pool with room for COMPONENTS_PER_POOL more objects
        void AddPool();

        std::vector<T*> firstAvailable; // for free list, first unallocated object in each pool                                                       
};