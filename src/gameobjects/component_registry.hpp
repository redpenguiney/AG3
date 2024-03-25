// This file stores all components in a way that keeps them both organized and cache-friendly.
// Basically entites with the same set of components have their components stored together.
// Pointers to components are never invalidated thanks to component pool fyi.

// TODO: interleaved component pools?
// TODO: We can't really destroy gameobjects???

#pragma once
#include <array>
#include <bitset>
#include <cassert>
#include <cstddef>
#include <cstdio>
#include <memory>
#include <tuple>
#include <vector>
#include "component_pool.hpp"
#include <vector>
#include "../graphics/engine.hpp"
#include "transform_component.hpp"
#include "../physics/spatial_acceleration_structure.hpp"
#include "pointlight_component.hpp"
#include "rigidbody_component.hpp"
#include <optional>

struct GameobjectCreateParams;

namespace ComponentRegistry {
    enum ComponentBitIndex {
        TransformComponentBitIndex = 0,
        RenderComponentBitIndex = 1,
        ColliderComponentBitIndex = 2,
        RigidbodyComponentBitIndex = 3,
        PointlightComponentBitIndex = 4,
        RenderComponentNoFOBitIndex = 5
    };

    // How many different component classes there are. 
    static inline const unsigned int N_COMPONENT_TYPES = 8; 

    // Returns a new game object with the given components.
    std::shared_ptr<GameObject> NewGameObject(const GameobjectCreateParams& params);
}

template<typename T>
constexpr ComponentRegistry::ComponentBitIndex indexFromClass();
template<> constexpr inline ComponentRegistry::ComponentBitIndex indexFromClass<TransformComponent>() {
    return ComponentRegistry::TransformComponentBitIndex;
}
template<> constexpr inline ComponentRegistry::ComponentBitIndex indexFromClass<GraphicsEngine::RenderComponent>() {
    return ComponentRegistry::RenderComponentBitIndex;
}
template<> constexpr inline ComponentRegistry::ComponentBitIndex indexFromClass<GraphicsEngine::RenderComponentNoFO>() {
    return ComponentRegistry::RenderComponentNoFOBitIndex;
}
template<> constexpr inline ComponentRegistry::ComponentBitIndex indexFromClass<SpatialAccelerationStructure::ColliderComponent>() {
    return ComponentRegistry::ColliderComponentBitIndex;
}
template<> constexpr inline ComponentRegistry::ComponentBitIndex indexFromClass<RigidbodyComponent>() {
    return ComponentRegistry::RigidbodyComponentBitIndex;
}
template<> constexpr inline ComponentRegistry::ComponentBitIndex indexFromClass<PointLightComponent>() {
    return ComponentRegistry::PointlightComponentBitIndex;
}

struct GameobjectCreateParams {
    unsigned int physMeshId; // set to 0 (default) if you want automatically generated (requires meshId in that case); ignore if no collider
    unsigned int meshId; // ignore if not rendering
    unsigned int materialId; // defaults to 0 for no material. ignore if not rendering
    unsigned int shaderId; // defaults to 0 for default shader. ignore if not rendering

    GameobjectCreateParams(const std::vector<ComponentRegistry::ComponentBitIndex> componentList):
    physMeshId(0),
    meshId(0),
    materialId(0),
    shaderId(0)
    {
        // bitset defaults to all false so we good
        for (auto & i : componentList) {
            requestedComponents[i] = true;
        }
    }

    private:
    friend std::shared_ptr<GameObject> ComponentRegistry::NewGameObject(const GameobjectCreateParams& params);
    std::bitset<ComponentRegistry::N_COMPONENT_TYPES> requestedComponents;
};

// Just a little pointer wrapper for gameobjects that throws an error when trying to dereference a nullptr to a component (gameobjects have a nullptr to any components they don't have, and when Destroy() is called all components are set to nullptr)
// Still the gameobject's job to get and return objects to/from the pool since the pool used depends on the exact set of components the gameobject uses.
template<typename T>
class ComponentHandle {
    public:
    ComponentHandle(T* const comp_ptr);
    T& operator*() const;
    T* operator->() const;

    // Returns true if not nullptr
    explicit operator bool() const;

    // might return nullptr, be careful.
    // only exists so you can pass a reference to a component to a function
    T* const GetPtr() const;

    // If ptr != nullptr/it hasn't already been cleared, calls Destroy() on the component, returns it to the pool, and sets ptr to nullptr.
    // Gameobjects do this for all their components when Destroy() is called on them.
    void Clear();

    private:
    T* ptr;
};

// templates can't go in cpps
// TODO: apparently they can
template<typename T>
ComponentHandle<T>::ComponentHandle(T* const comp_ptr) : ptr(comp_ptr) {}

template<typename T>
ComponentHandle<T>::operator bool() const {
    return ptr != nullptr;
}

template<typename T>
T& ComponentHandle<T>::operator*() const {assert(ptr != nullptr); return *ptr;}

template<typename T>
T* ComponentHandle<T>::operator->() const {assert(ptr != nullptr); return ptr;};

template<typename T>
T* const ComponentHandle<T>::GetPtr() const {return ptr;}

template<typename T>
void ComponentHandle<T>::Clear() {
    if (ptr) {
        ptr->Destroy();
        ptr->pool->ReturnObject(ptr);
    }
}

class RigidbodyComponent;


// The gameobject system uses ECS (google it).
class GameObject {
    public:
    std::string name; // just an identifier i have mainly for debug reasons, scripts could also use it i guess

    struct GameObjectNetworkData {
        // Whether or not the running application owns this gameobject (and is sending sync data to other clients) or not (and is recieving sync data from the owning client)
        bool isOwner;

        // Value is pointless and undefined if not the owner.
        // 
        unsigned int syncAccumulator;
    };

    // If this is null, this gameobject will not be synced between server/client.
    std::optional<GameObjectNetworkData> networkData;

    // TODO: avoid not storing ptrs for components we don't have
    ComponentHandle<TransformComponent> transformComponent;
    ComponentHandle<GraphicsEngine::RenderComponent> renderComponent;
    // ComponentHandle<GraphicsEngine::RenderComponentNoFO> renderComponentNoFO; these two components have exact same size and methods and you can only have one of them so why bother
    ComponentHandle<RigidbodyComponent> rigidbodyComponent;
    ComponentHandle<SpatialAccelerationStructure::ColliderComponent> colliderComponent;
    ComponentHandle<PointLightComponent> pointLightComponent;

    ~GameObject();

    // Destroys all components of the gameobject, sets all component handles to nullptr.
    // ComponentHandle will catch if you try to access the components after this is called and throw an error.
    // Will error if you try to destroy the same gameobject twice.
    void Destroy();

    protected:
    
    // no copy constructing gameobjects.
    GameObject(const GameObject&) = delete; 

    // private because ComponentRegistry makes it and also because it needs to return a shared_ptr.
    friend class std::shared_ptr<GameObject>;
    GameObject(const GameobjectCreateParams& params, std::array<void*, ComponentRegistry::N_COMPONENT_TYPES> components); // components is not type safe because component registry weird, index is ComponentBitIndex, value is nullptr or ptr to componeny
    
};

namespace ComponentRegistry {
    // To hide all the non-type safe stuff, we need an iterator that just lets people iterate through the components of all gameobjects that have certain components (i.e give me all pairs of transform + render)
    template <typename ... Args>
    class Iterator {
        public: 

        // std libraries expect iterators to do this
        using iterator_category = std::forward_iterator_tag; // this is a forward iterator
        using difference_type   = std::ptrdiff_t; // ?
        using value_type        = std::tuple<Args* ...>; // thing you get by iterating thru
        using pointer           = value_type*;
        using reference         = value_type&; 

        // forward iterator stuff
        Iterator<Args...>& operator++() { // prefix
            componentIndex += 1;
        
            if (ComponentPool<TransformComponent>::COMPONENTS_PER_PAGE == componentIndex) {
                //std::cout << "uh oh2 " << pageIndex << "\n"; 
                componentIndex = 0;
                pageIndex += 1;
                if (nPages == pageIndex) { 
                    pageIndex = 0;
                    poolIndex += 1;
                    if (poolIndex != pools.size()) {
                        currentPoolArray = &pools.at(poolIndex);
                        nPages = ((ComponentPool<TransformComponent>*)(*currentPoolArray)[TransformComponentBitIndex])->pages.size();
                    }
                    else {
                        // std::cout << "Reached end\n";
                        isEnd = true;
                    }
                }
                setPgPtrs();
            }
            // std::printf("DID PREFIX, now %u %u %u\n", componentIndex, pageIndex, poolIndex);
            return *this;
        }

        void setPgPtrs() {
            currentPagePtrs = {setPgPtr<Args>() ...};
        }

        template<typename T>
        T* setPgPtr() {
            
            constexpr unsigned int poolTypeIndex = indexFromClass<T>();
            ComponentPool<T>* pool = (ComponentPool<T>*)((*currentPoolArray)[poolTypeIndex]);
            return pool->pages[pageIndex];
        }

        // Iterator<Args...> operator++(int) { // postfix; int arg is dumb and stupid
        //     auto temp = *this;
        //     componentIndex += 1;
        //     if (ComponentPool<TransformComponent>::COMPONENTS_PER_PAGE == componentIndex) {
        //         std::cout << "uh oh " << pageIndex << "\n"; 
        //         componentIndex = 0;
        //         pageIndex += 1;
        //         if (((ComponentPool<TransformComponent>*)(*currentPoolArray)[TransformComponentBitIndex])->pages.size() == pageIndex) {
        //             pageIndex = 0;
        //             poolIndex += 1;
        //             currentPoolArray = &pools.at(poolIndex);
        //         }
        //     }
        //     return temp;
        // }

        reference operator*() {
            // std::printf("DID operator*, now %u %u %u\n", componentIndex, pageIndex, poolIndex);
            // std::cout << "here array is " << currentPoolArray << "\n";
            currentThingWeIteratingOn = {getRef<Args>() ...};
            
            return currentThingWeIteratingOn;
        }

        pointer operator->() {
            //std::cout << "-> operator used\n";
            currentThingWeIteratingOn = std::make_tuple<value_type>(getRef<Args>() ...);
            return &currentThingWeIteratingOn;
        }

        friend bool operator==(const Iterator<Args...>& a, const Iterator<Args...>& b) {
            return (a.isEnd == b.isEnd);
        }

        friend bool operator!=(const Iterator<Args...>& a, const Iterator<Args...>& b) {
            return !(a == b);
        }
        

        Iterator<Args...> begin() {
            //std::cout << "Begin called.\n";
            return *this;
        } 
        
        const Iterator<Args...>& end() {
            //std::cout << "End called.\n";
            static const Iterator<Args...> it {true};
            return it;
        }

        Iterator(std::vector<std::array<void*, N_COMPONENT_TYPES>> arg_pools): 
        pools(arg_pools),
        componentIndex(0),
        poolIndex(0),
        pageIndex(0),
        isEnd(false),
        currentPoolArray(pools.size() == 0 ? nullptr: &(pools.at(pageIndex)))
        
        {

            if (currentPoolArray != nullptr) {
                setPgPtrs();
                nPages = ((ComponentPool<TransformComponent>*)(*currentPoolArray)[TransformComponentBitIndex])->pages.size();
            }
            else {
                isEnd = true;
            }
            //std::cout << "dear god you made an iterator why, there are " << pools.size() << " pools, set currentPoolArray to " << currentPoolArray << "\n";
        }

        Iterator(bool end)
        {
            assert(end);
            isEnd = true;
        }

        Iterator(const Iterator<Args...> & original) {
            pools = original.pools;
            componentIndex = original.componentIndex;
            poolIndex = original.poolIndex;
            pageIndex = original.pageIndex;     
            currentPoolArray = pools.size() == 0 ? nullptr: &(pools.at(pageIndex));
            isEnd = original.isEnd;
            if (currentPoolArray != nullptr) {
                setPgPtrs();
                nPages = ((ComponentPool<TransformComponent>*)(*currentPoolArray)[TransformComponentBitIndex])->pages.size();
            }
            

            //std::cout << "We copied, there are now " << pools.size() << " pools when the original had " << original.pools.size() << " pools.\n";
        }
        

        private:
        std::tuple<Args* ...> currentPagePtrs;
        value_type currentThingWeIteratingOn;

        // a little black magic from https://stackoverflow.com/questions/18063451/get-index-of-a-tuple-elements-type
        // lets you (statically) get tuple index from type
        template <class T, class Tuple>
        struct Index;

        template <class T, class... Types>
        struct Index<T, std::tuple<T, Types...>> {
            static const std::size_t value = 0;
        };

        template <class T, class U, class... Types>
        struct Index<T, std::tuple<U, Types...>> {
            static const std::size_t value = 1 + Index<T, std::tuple<Types...>>::value;
        };

        std::vector<std::array<void*, N_COMPONENT_TYPES>> pools;
        unsigned int componentIndex; // index into a pool of a componentPool
        unsigned int poolIndex; // index into pools
        unsigned int pageIndex; // within a componentPool, the index into the pools member
        unsigned int nPages;
        bool isEnd; // we need to store a thingy for this annoyingly so we can return a constexpr for end
        std::array<void*, N_COMPONENT_TYPES>* currentPoolArray;

        template<typename T>
        T* getRef() {
            //assert(currentPoolArray != nullptr);
            //std::cout << "Getting ref for type " << typeid(T).name() << ", currentPoolArray=" << currentPoolArray << ".\n";
            //currentPoolArray = &(pools.at(poolIndex));
            
            //std::cout << "We at p = " << pool << "\n";
            //std::cout << pageIndex << " my guy \n";
            constexpr static const unsigned int i = Index<T*, std::tuple<Args* ...>>().value;
            return std::get<i>(currentPagePtrs) + componentIndex;
        }
    };

    inline std::unordered_map<GameObject*, std::shared_ptr<GameObject>> GAMEOBJECTS;

    // Stores all the component pools.
    // Bitset has a bit for each component class, if its 1 then the value corresonding to that key stores gameobjects with that component. (but only if the gameobject stores all the exact same components as the bitset describes)
    // DONT TOUCH PLS
    // TODO: make private somehow
    inline std::unordered_map<std::bitset<N_COMPONENT_TYPES>, std::array<void*, N_COMPONENT_TYPES>> componentBuckets;

    // returns vector of bit indices from variadic template args
    template <typename ... Args>
    std::vector<ComponentRegistry::ComponentBitIndex> requestedComponentIndicesFromTemplateArgs() {
        std::vector<ComponentRegistry::ComponentBitIndex> indices;
        if constexpr((std::is_same_v<TransformComponent, Args> || ...)) {
            indices.push_back(ComponentRegistry::TransformComponentBitIndex);
        }
        if constexpr((std::is_same_v<GraphicsEngine::RenderComponent, Args> || ...)) {
            indices.push_back(ComponentRegistry::RenderComponentBitIndex);
        }
        if constexpr((std::is_same_v<GraphicsEngine::RenderComponentNoFO, Args> || ...)) {
            indices.push_back(ComponentRegistry::RenderComponentNoFOBitIndex);
        }
        if constexpr((std::is_same_v<SpatialAccelerationStructure::ColliderComponent, Args> || ...)) {
            indices.push_back(ComponentRegistry::ColliderComponentBitIndex);
        }
        if constexpr((std::is_same_v<RigidbodyComponent, Args> || ...)) {
            indices.push_back(ComponentRegistry::RigidbodyComponentBitIndex);
        }
        if constexpr((std::is_same_v<PointLightComponent, Args> || ...)) {
            indices.push_back(ComponentRegistry::PointlightComponentBitIndex);
        }
        return indices;
    }

    // Gives an iterator so you can iterate through all gameobjects that have the requested components (except you don't actually get the gameobject, just a tuple of components)
    // TODO: might be good to optimize by just precalculating this and updating when new gameobject component combination is added
    template <typename ... Args>
    Iterator<Args...> GetSystemComponents() {
        auto requestedComponents = requestedComponentIndicesFromTemplateArgs<Args...>();
        std::vector<std::array<void*, N_COMPONENT_TYPES>> poolsToReturn;

        for (auto & [bitset, pools] : componentBuckets) {
            //std::cout << "Considering bucket with bitset " << bitset << " to supply "; for (auto & i: requestedComponents) {std::cout << i << " ";} std:: cout << ".\n";
            //unsigned int i = 0;
            for (auto & bitIndex: requestedComponents) {
                if (bitset[bitIndex] == false) {
                    //std::cout << "Rejected bucket, missing index " << bitIndex << ".\n";
                    // this bucket is missing a pool for one of the requested components, don't send it to the system
                    goto innerLoopEnd;
                }
                //i++;
            }
            // this pool stores gameobjects with all the components we want, return it
            poolsToReturn.push_back(pools);

            innerLoopEnd:;
        }

        return Iterator<Args...>(poolsToReturn);
    }
    
    std::shared_ptr<GameObject> NewGameObject(const GameobjectCreateParams& params);
    

    // Call before destroying graphics engine, SAS, etc. at end of program, as gameobject destructors need to access those things.
    // Won't work if there are shared_ptr<GameObject>s outside the GAMEOBJECTS map (inside lua code), so make sure all lua code is eliminated before calling.
    void CleanupComponents();
};

