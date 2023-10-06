#include "component_pool.hpp"

template<typename T, unsigned int COMPONENTS_PER_POOL>
T* ComponentPool<T, COMPONENTS_PER_POOL>::GetNew() {
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

template<typename T, unsigned int COMPONENTS_PER_POOL>
ComponentPool<T, COMPONENTS_PER_POOL>::ComponentPool() {
    AddPool();
}

template<typename T, unsigned int COMPONENTS_PER_POOL>
void ComponentPool<T, COMPONENTS_PER_POOL>::ReturnObject(T* component) {
    component->next = firstAvailable[component->componentPoolId];
    firstAvailable[component->componentPoolId] = component;
}

template<typename T, unsigned int COMPONENTS_PER_POOL>
ComponentPool<T, COMPONENTS_PER_POOL>::~ComponentPool() {
    for (auto & ptr: pools) {
        delete ptr;
    }
}

template<typename T, unsigned int COMPONENTS_PER_POOL>
void ComponentPool<T, COMPONENTS_PER_POOL>::AddPool() {
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