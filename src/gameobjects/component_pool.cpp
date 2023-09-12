#pragma once
#include <deque>
#include <vector>
#include <iterator> 
#include <cstddef>

// Object pool for components (although i suppose you could use it for something besides components).
// Automatically resizes.
// Stores all objects in mostly contiguous memory for cache performance.
// Guarantees that pointers to pool contents will always remain accurate, by instead of using std::vector, using multiple arrays.
// Just set COMPONENTS_PER_POOL to 65536 if you aren't sure, no default value because if you specify it in some places but not others you get types mixed up
template<typename T, unsigned int COMPONENTS_PER_POOL>
class ComponentPool {
    friend T;

    public:
        std::vector<T*> pools; // public because making an iterator was too much work

        ComponentPool() {
            AddPool();
        }

        // Returns a pointer to an uninitialized component. The fields of this component are undefined until you set them to something.
        T* GetNew() {
            // We use something called a "free list" to find a component
            int poolIndex = -1;
            for (T* & ptr: pools) {
                (void)ptr; // make compiler shut up about unused variable "ptr"
                poolIndex += 1;
                if (firstAvailable[poolIndex] == nullptr) {continue;} // if the pool is full go to the next one

                T* foundObject = firstAvailable[poolIndex];
                firstAvailable[poolIndex] = foundObject->next;
                return foundObject;
            }

            // if we got this far there is no available pool
            AddPool();
            T* foundObject = firstAvailable[poolIndex];
            firstAvailable[poolIndex] = foundObject->next;
            return foundObject;
        }

        // Takes a pointer to a component returned by GetNew() and returns the component back to the pool.
        // Obviously, do not use the component after calling this. 
        void ReturnObject(T* component) {
            component->next = firstAvailable[component->componentPoolId];
            firstAvailable[component->componentPoolId] = component;
        }

        ~ComponentPool() {
            for (auto & ptr: pools) {
                delete ptr;
            }
        }

        // // We wanna iterate through component pools so this exists
        // // see https://www.internalpointers.com/post/writing-custom-iterators-modern-cpp
        // // TODO: this iterator might be really bad idk
        // struct ComponentPoolIterator {
        //     public:
        //     using iterator_category = std::forward_iterator_tag;
        //     using difference_type   = std::ptrdiff_t;

        //     ComponentPoolIterator(unsigned int objectIndex, unsigned int poolIndex, T* pointer) { 
        //         index = objectIndex;
        //         pool = poolIndex;
        //         ptr = pointer;
        //     }

        //     private:
        //     unsigned int index;
        //     unsigned int pool;
        //     T* ptr;

        //     T& operator*() const { return *ptr;}
        //     T* operator->() {return ptr;}

        //     // prefix increment
        //     ComponentPoolIterator& operator++() {
        //         if (index == COMPONENTS_PER_POOL - 1) {
        //             index = 0;
        //             pool++;
        //             ptr = pools[pool];
                    
        //         }
                
        //     }
        // };
        
        

    private:
        // adds new pool with room for COMPONENTS_PER_POOL more objects
        void AddPool() {
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

        std::vector<T*> firstAvailable; // for free list, first unallocated object in each pool                                                       
};