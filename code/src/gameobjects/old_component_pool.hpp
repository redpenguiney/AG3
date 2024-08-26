// Deprecated in favor of component_group.hpp.

//#pragma once
//#include "debug/assert.hpp"
//#include <cstdio>
//#include <deque>
//#include <iostream>
//#include <memory>
//#include <type_traits>
//#include <vector>
//#include <iterator> 
//#include <cstddef>
//#include "base_component.hpp"
//
//// Object pool for components (although i suppose you could use it for something besides components).
//// Automatically resizes.
//// Stores all objects in mostly contiguous memory for cache performance.
//// Guarantees that pointers to pool contents will always remain accurate, by instead of using std::vector, using multiple arrays.
//// COMPONENTS_PER_PAGE template argument removed for reasons
//template<typename T>
//class ComponentPool {
//    friend T;
//
//    public:
//        // vectors of arrays of length COMPONENTS_PER_PAGE
//        std::vector<T*> pages;
//
//        ComponentPool();
//        ComponentPool(const ComponentPool<T>&) = delete;
//
//        // Returns a pointer to an uninitialized component. The fields of this component are undefined until you set them to something.
//        T* GetNew();
//
//        // Takes a pointer to a component returned by GetNew() and returns the component back to the pool.
//        // Obviously, do not use the component after calling this. 
//        void ReturnObject(T* component);
//
//        ~ComponentPool();
//
//        const static unsigned int COMPONENTS_PER_PAGE = 16384;   
//
//    private:
//        // adds new pool with room for COMPONENTS_PER_PAGE more objects
//        void AddPool();
//
//        std::vector<T*> firstAvailable; // for free list, first unallocated object in each pool  
//
//                                             
//};
//
//// no .cpp because templates are dumb
//template<typename T>
//T* ComponentPool<T>::GetNew() {
//    //std::cout << "Getting new " << typeid(T).name() << "\n";
//    T* foundObject = nullptr;
//
//    // We use something called a "free list" to find a component
//    int poolIndex = -1;
//    for (T* & ptr: pages) {
//        (void)ptr; // make compiler shut up about unused variable "ptr"
//        poolIndex += 1;
//        if (firstAvailable[poolIndex] == nullptr) {continue;} // if the pool is full go to the next one
//
//        foundObject = firstAvailable[poolIndex];
//        // Assert(foundObject->live == false);
//        firstAvailable[poolIndex] = (T*)(foundObject->next);
//
//        break;
//    }
//
//    // if there is no available pool
//    if (!foundObject) {
//        
//        AddPool();
//        foundObject = firstAvailable.back();
//        //std::cout << "Vec back is " << firstAvailable.back() << " and it holds " << firstAvailable.size() << ".\n";
//        firstAvailable.back() =  (T*)(foundObject->next);
//        
//        //std::cout << "Had to expand component pool, returning " << foundObject << "\n";
//    }
//    
//    foundObject->live = true;
//    // std::cout << "Component " << typeid(T).name() << " made that is live = " << foundObject->live << " and also its at " << (void*)foundObject << ".\n";
//    return foundObject;
//}
//
//template<typename T>
//ComponentPool<T>::ComponentPool() {
//    AddPool();
//}
//
//template<typename T>
//void ComponentPool<T>::ReturnObject(T* component) {
//    //std::cout << "Returning component at " << component << " poolId " << component->componentPoolId << ".\n"; 
//    component->live = false;
//    component->next = firstAvailable.at(component->componentPoolId);
//    firstAvailable[component->componentPoolId] = component;
//}
//
//template<typename T>
//ComponentPool<T>::~ComponentPool() {
//    for (auto & ptr: pages) {
//        delete ptr;
//    }
//}
//
//template<typename T>
//void ComponentPool<T>::AddPool() {
//    unsigned int index = pages.size();
//    T* firstPool = new T[COMPONENTS_PER_PAGE];
//    //std::cout << "Created new pool page at " << firstPool << " of type " << typeid(T).name() << "\n";
//    firstAvailable.push_back(firstPool);
//
//    for (unsigned int i = 0; i < COMPONENTS_PER_PAGE - 1; i++) {
//        firstPool[i].next = &(firstPool[i + 1]);
//    }
//    firstPool[COMPONENTS_PER_PAGE - 1].next = nullptr;
//
//    for (unsigned int i = 0; i < COMPONENTS_PER_PAGE; i++) {
//        firstPool[i].componentPoolId = index;
//        firstPool[i].SetPool(this);
//        firstPool[i].live = false;
//    }
//    
//    pages.push_back(firstPool);
//}