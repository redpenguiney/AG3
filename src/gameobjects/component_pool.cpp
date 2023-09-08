// Object pool for components (although i suppose you could use it for something besides components).
// Automatically resizes.
// Stores all objects in mostly contiguous memory for cache performance.
// Guarantees that pointers to pool contents will always remain accurate, by instead of using std::vector, using multiple arrays.
#include <deque>
#include <vector>
template<typename T, unsigned int COMPONENTS_PER_POOL = 16384>
class ComponentPool {
    public:
        ComponentPool() {
            AddPool();
        }

        // Returns a pointer to an uninitialized component. The fields of this component are undefined until you set them to something.
        T* GetNew() {
            // We use something called a "free list" to find a component
            int poolIndex = -1;
            for (T* & ptr: pools) {
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
        std::vector<T*> pools;

        ~ComponentPool() {
            for (auto & ptr: pools) {
                delete ptr;
            }
        }
};