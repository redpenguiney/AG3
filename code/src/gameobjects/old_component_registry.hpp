//// This file stores all components in a way that keeps them both organized and cache-friendly.
//// Basically entites with the same set of components have their components stored together.
//// Pointers to components are never invalidated thanks to component pool fyi.
//
//// TODO: interleaved component pools?
//// TODO: We can't really destroy gameobjects???
//
//#pragma once
//#include <array>
//#include <bitset>
//#include "debug/assert.hpp"
//#include <cstddef>
//#include <cstdio>
//#include <memory>
//#include <tuple>
//#include <vector>
////#include "component_pool.hpp"
//#include "component_group.hpp"
//#include <vector>
//
//#include <optional>
//
//#include "modules/component_registry_export.hpp"
//
//struct GameobjectCreateParams;
//
//// Just a little pointer wrapper for gameobjects that throws an error when trying to dereference a nullptr to a component (gameobjects have a nullptr to any components they don't have, and when Destroy() is called all components are set to nullptr)
//// Still the gameobject's job to get and return objects to/from the pool since the pool used depends on the exact set of components the gameobject uses.
////template<typename T>
////class ComponentHandle {
////    public:
////    ComponentHandle(T* const comp_ptr);
////    ComponentHandle(const ComponentHandle<T>&) = delete;
////    T& operator*() const;
////    T* operator->() const;
////
////    // Returns true if not nullptr
////    explicit operator bool() const;
////
////    // might return nullptr, be careful.
////    // only exists so you can pass a reference to a component to a function
////    T* const GetPtr() const;
////
////    // If ptr != nullptr/it hasn't already been cleared, returns it to the pool (thus destructing the component), and sets ptr to nullptr.
////    // Gameobjects do this for all their components when Destroy() is called on them.
////    void Clear();
////
////    private:
////    // TODO: make unique?
////    T* ptr;
////};
//
//// We can't use normal ComponentHandle because sol would, if told it was like a pointer (which it is and it needs to know that), it would try to copy it, which is bad so we need to do this.
////template <typename T>
////class LuaComponentHandle {
////    public:
////    ComponentHandle<T>* const handle;
////    LuaComponentHandle(ComponentHandle<T>* ptr): handle(ptr) {}
////};
//
//// templates can't go in cpps
//// TODO: apparently they can
////template<typename T>
////ComponentHandle<T>::ComponentHandle(T* const comp_ptr) : ptr(comp_ptr) {}
////
////template<typename T>
////ComponentHandle<T>::operator bool() const {
////    return ptr != nullptr;
////}
////
////template<typename T>
////T& ComponentHandle<T>::operator*() const {Assert(ptr != nullptr); return *ptr;}
////
////template<typename T>
////T* ComponentHandle<T>::operator->() const {Assert(ptr != nullptr); return ptr;};
////
////template<typename T>
////T* const ComponentHandle<T>::GetPtr() const {return ptr;}
////
////template<typename T>
////void ComponentHandle<T>::Clear() {
////    if (ptr) {
////        ptr->Destroy();
////        ptr->pool->ReturnObject(ptr);
////        ptr = nullptr;
////    }
////}
//
//class RigidbodyComponent;
//
//
//
//
////template <std::derived_from<BaseComponent> ... Components>
////class SystemDependencies {
////    using components = Components...;
////};
//
//class ComponentRegistry {
//    public:
//    template<std::derived_from<BaseComponent> ... Components>
//    std::vector<ComponentGroupInterface*> GetGroupsWithComponents() {
//        auto targetBitset = Archetype::FromComponents<Components>().componentIds;
//
//        std::vector<ComponentGroupInterface*> matches;
//
//        for (auto& [archetype, group] : componentGroups) {
//            if ((group->contents.componentIds & targetBitset) == targetBitset) {
//                matches.push_back(group);
//            }
//        }
//
//        return matches;
//    }
//
//    // Returns a new game object with the given components.
//    // TODO: return weak_ptr instead?
//    template <std::derived_from<BaseComponent> ... Components>
//    std::shared_ptr<GameObject> NewGameObject(const GameobjectCreateParams& params) {
//        static const Archetype archetype = Archetype::FromComponents<Components...>();
//
//        if (componentGroups.count(archetype.GetBitset()) == 0) {
//            componentGroups.emplace(archetype.GetBitset(), new ComponentGroup<Components...>());
//        }
//        ComponentGroupInterface* group = componentGroups[archetype.GetBitset()];
//        auto [components, index, page] = group->New();
//        return protected_make_shared(params, archetype, components, index, page);
//    }
//
//    static ComponentRegistry& Get();
//
//    /*
//    // To hide all the non-type safe stuff, we need an iterator that just lets people iterate through the components of all gameobjects that have certain components (i.e give me all pairs of transform + render)
//    template <typename ... Args>
//    class Iterator {
//        public: 
//
//        // std libraries expect iterators to do this
//        using iterator_category = std::forward_iterator_tag; // this is a forward iterator
//        using difference_type   = std::ptrdiff_t; // ?
//        using value_type        = std::tuple<Args* ...>; // thing you get by iterating thru
//        using pointer           = value_type*;
//        using reference         = value_type&; 
//
//        // forward iterator stuff
//        Iterator<Args...>& operator++() { // prefix
//            componentIndex += 1;
//        
//            if (ComponentPool<TransformComponent>::COMPONENTS_PER_PAGE == componentIndex) {
//                //std::cout << "uh oh2 " << pageIndex << "\n"; 
//                componentIndex = 0;
//                pageIndex += 1;
//                if (nPages == pageIndex) { 
//                    pageIndex = 0;
//                    poolIndex += 1;
//                    if (poolIndex != pools.size()) {
//                        currentPoolArray = &pools.at(poolIndex);
//                        nPages = ((ComponentPool<TransformComponent>*)(*currentPoolArray)[TransformComponentBitIndex])->pages.size();
//                    }
//                    else {
//                        // std::cout << "Reached end\n";
//                        isEnd = true;
//                    }
//                }
//                setPgPtrs();
//            }
//            // std::printf("DID PREFIX, now %u %u %u\n", componentIndex, pageIndex, poolIndex);
//            return *this;
//        }
//
//        void setPgPtrs() {
//            currentPagePtrs = {setPgPtr<Args>() ...};
//        }
//
//        template<typename T>
//        T* setPgPtr() {
//            
//            constexpr unsigned int poolTypeIndex = ComponentIdFromType<T>();
//            ComponentPool<T>* pool = (ComponentPool<T>*)((*currentPoolArray)[poolTypeIndex]);
//            return pool->pages[pageIndex];
//        }
//
//        // Iterator<Args...> operator++(int) { // postfix; int arg is dumb and stupid
//        //     auto temp = *this;
//        //     componentIndex += 1;
//        //     if (ComponentPool<TransformComponent>::COMPONENTS_PER_PAGE == componentIndex) {
//        //         std::cout << "uh oh " << pageIndex << "\n"; 
//        //         componentIndex = 0;
//        //         pageIndex += 1;
//        //         if (((ComponentPool<TransformComponent>*)(*currentPoolArray)[TransformComponentBitIndex])->pages.size() == pageIndex) {
//        //             pageIndex = 0;
//        //             poolIndex += 1;
//        //             currentPoolArray = &pools.at(poolIndex);
//        //         }
//        //     }
//        //     return temp;
//        // }
//
//        reference operator*() {
//            // std::printf("DID operator*, now %u %u %u\n", componentIndex, pageIndex, poolIndex);
//            // std::cout << "here array is " << currentPoolArray << "\n";
//            currentThingWeIteratingOn = {getRef<Args>() ...};
//            
//            return currentThingWeIteratingOn;
//        }
//
//        pointer operator->() {
//            //std::cout << "-> operator used\n";
//            currentThingWeIteratingOn = std::make_tuple<value_type>(getRef<Args>() ...);
//            return &currentThingWeIteratingOn;
//        }
//
//        friend bool operator==(const Iterator<Args...>& a, const Iterator<Args...>& b) {
//            return (a.isEnd == b.isEnd);
//        }
//
//        friend bool operator!=(const Iterator<Args...>& a, const Iterator<Args...>& b) {
//            return !(a == b);
//        }
//        
//
//        Iterator<Args...> begin() {
//            //std::cout << "Begin called.\n";
//            return *this;
//        } 
//        
//        const Iterator<Args...>& end() {
//            //std::cout << "End called.\n";
//            static const Iterator<Args...> it {true};
//            return it;
//        }
//
//        Iterator(std::vector<std::array<void*, N_COMPONENT_TYPES>> arg_pools): 
//        pools(arg_pools),
//        componentIndex(0),
//        poolIndex(0),
//        pageIndex(0),
//        isEnd(false),
//        currentPoolArray(pools.size() == 0 ? nullptr: &(pools.at(pageIndex)))
//        
//        {
//
//            if (currentPoolArray != nullptr) {
//                setPgPtrs();
//                nPages = ((ComponentPool<TransformComponent>*)(*currentPoolArray)[TransformComponentBitIndex])->pages.size();
//            }
//            else {
//                isEnd = true;
//            }
//            //std::cout << "dear god you made an iterator why, there are " << pools.size() << " pools, set currentPoolArray to " << currentPoolArray << "\n";
//        }
//
//        Iterator(bool end)
//        {
//            Assert(end);
//            isEnd = true;
//        }
//
//        Iterator(const Iterator<Args...> & original) {
//            pools = original.pools;
//            componentIndex = original.componentIndex;
//            poolIndex = original.poolIndex;
//            pageIndex = original.pageIndex;     
//            currentPoolArray = pools.size() == 0 ? nullptr: &(pools.at(pageIndex));
//            isEnd = original.isEnd;
//            if (currentPoolArray != nullptr) {
//                setPgPtrs();
//                nPages = ((ComponentPool<TransformComponent>*)(*currentPoolArray)[TransformComponentBitIndex])->pages.size();
//            }
//            
//
//            //std::cout << "We copied, there are now " << pools.size() << " pools when the original had " << original.pools.size() << " pools.\n";
//        }
//        
//
//        private:
//        std::tuple<Args* ...> currentPagePtrs;
//        value_type currentThingWeIteratingOn;
//
//        // a little black magic from https://stackoverflow.com/questions/18063451/get-index-of-a-tuple-elements-type
//        // lets you (statically) get tuple index from type
//        template <class T, class Tuple>
//        struct Index;
//
//        template <class T, class... Types>
//        struct Index<T, std::tuple<T, Types...>> {
//            static const std::size_t value = 0;
//        };
//
//        template <class T, class U, class... Types>
//        struct Index<T, std::tuple<U, Types...>> {
//            static const std::size_t value = 1 + Index<T, std::tuple<Types...>>::value;
//        };
//
//        std::vector<std::array<void*, N_COMPONENT_TYPES>> pools;
//        unsigned int componentIndex; // index into a pool of a componentPool
//        unsigned int poolIndex; // index into pools
//        unsigned int pageIndex; // within a componentPool, the index into the pools member
//        unsigned int nPages;
//        bool isEnd; // we need to store a thingy for this annoyingly so we can return a constexpr for end
//        std::array<void*, N_COMPONENT_TYPES>* currentPoolArray;
//
//        template<typename T>
//        T* getRef() {
//            //Assert(currentPoolArray != nullptr);
//            //std::cout << "Getting ref for type " << typeid(T).name() << ", currentPoolArray=" << currentPoolArray << ".\n";
//            //currentPoolArray = &(pools.at(poolIndex));
//            
//            //std::cout << "We at p = " << pool << "\n";
//            //std::cout << pageIndex << " my guy \n";
//            constexpr static const unsigned int i = Index<T*, std::tuple<Args* ...>>().value;
//            return std::get<i>(currentPagePtrs) + componentIndex;
//        }
//    };
//    */
//    
//    
//
//    std::unordered_map<GameObject*, std::shared_ptr<GameObject>> GAMEOBJECTS;
//
//    // Stores all the component pools.
//    // Bitset has a bit for each component class, if its 1 then the value corresonding to that key stores gameobjects with that component. (but only if the gameobject stores all the exact same components as the bitset describes)
//    // DONT TOUCH PLS
//    // TODO: make private somehow
//    std::unordered_map<std::bitset<N_COMPONENT_TYPES>, ComponentGroupInterface*> componentGroups;
//
//    // returns vector of bit indices from variadic template args
//    /*template <typename ... Args>
//    std::vector<ComponentBitIndex::ComponentBitIndex> requestedComponentIndicesFromTemplateArgs() {
//        std::vector<ComponentBitIndex::ComponentBitIndex> indices;
//        if constexpr((std::is_same_v<TransformComponent, Args> || ...)) {
//            indices.push_back(ComponentBitIndex::Transform);
//        }
//        else if constexpr((std::is_same_v<RenderComponent, Args> || ...)) {
//            indices.push_back(ComponentBitIndex::Render);
//        }
//        else if constexpr((std::is_same_v<RenderComponentNoFO, Args> || ...)) {
//            indices.push_back(ComponentBitIndex::RenderNoFO);
//        }
//        else if constexpr((std::is_same_v<ColliderComponent, Args> || ...)) {
//            indices.push_back(ComponentBitIndex::Collider);
//        }
//        else if constexpr((std::is_same_v<RigidbodyComponent, Args> || ...)) {
//            indices.push_back(ComponentBitIndex::Rigidbody);
//        }
//        else if constexpr((std::is_same_v<PointLightComponent, Args> || ...)) {
//            indices.push_back(ComponentBitIndex::Pointlight);
//        }
//        else if constexpr((std::is_same_v<AudioPlayerComponent, Args> || ...)) {
//            indices.push_back(ComponentBitIndex::AudioPlayer);
//        }
//        else if constexpr((std::is_same_v<AnimationComponent, Args> || ...)) {
//            indices.push_back(ComponentBitIndex::Animation);
//        }
//        else if constexpr ((std::is_same_v<SpotLightComponent, Args> || ...)) {
//            indices.push_back(ComponentBitIndex::Spotlight);
//        }
//        else {
//            Assert(false);
//        }
//        return indices;
//    }*/
//
//    // Calls the given function for every gameobject with all RequiredComponents, also passing OptionalComponents if the object has them (or nullptr if not).
//    /*template <typename RequiredComponents, typename OptionalComponents>
//    void GetSystemComponents(std::function<void(std::tuple<typename RequiredComponents::components& ...>, std::tuple<typename OptionalComponents::components* ...>)> function) {
//
//    }*/
//
//    /*
//    // Gives an iterator so you can iterate through all gameobjects that have the requested components (except you don't actually get the gameobject, just a tuple of components)
//    template <typename ... Args>
//    Iterator<Args...> GetSystemComponents() {
//        auto requestedComponents = requestedComponentIndicesFromTemplateArgs<Args...>();
//        std::vector<std::array<void*, N_COMPONENT_TYPES>> poolsToReturn;
//
//        for (auto & [bitset, pools] : componentBuckets) {
//            //std::cout << "Considering bucket with bitset " << bitset << " to supply "; for (auto & i: requestedComponents) {std::cout << i << " ";} std:: cout << ".\n";
//            //unsigned int i = 0;
//            for (auto & bitIndex: requestedComponents) {
//                if (bitset[bitIndex] == false) {
//                    //std::cout << "Rejected bucket, missing index " << bitIndex << ".\n";
//                    // this bucket is missing a pool for one of the requested components, don't send it to the system
//                    goto innerLoopEnd;
//                }
//                //i++;
//            }
//            // this pool stores gameobjects with all the components we want, return it
//            poolsToReturn.push_back(pools);
//
//            innerLoopEnd:;
//        }
//
//        return Iterator<Args...>(poolsToReturn);
//    }
//    */
//        
//    private:
//    ComponentRegistry();
//
//    // Won't work if there are shared_ptr<GameObject>s outside the GAMEOBJECTS map (inside lua code), so make sure all lua code is eliminated before calling (supposedly/allegedly?? why???) TODO.
//    ~ComponentRegistry();
//   
//};
//
//
//
//// The gameobject system uses ECS (google it).
//class GameObject {
//public:
//    std::string name; // just an identifier i have mainly for debug reasons, scripts could also use it i guess
//    const Archetype componentTypes;
//    
//    template <std::derived_from<BaseComponent> Component> 
//    Component& Get() {
//        short offset = componentTypes.GetOffset<Component>();
//        assert(offset != -1);
//        return *(Component*)((char*)components + offset);
//    }
//
//    // override Get() for rendercomponent so it handles RenderComponentNoFO
//    template <>
//    RenderComponent& Get<RenderComponent>() {
//
//    }
//
//    template <std::derived_from<BaseComponent> Component>
//    std::optional<Component*> MaybeGet() {
//        short offset = componentTypes.GetOffset<Component>();
//        if (offset == -1) {
//            return std::nullopt;
//        }
//        else {
//            return (Component*)((char*)components + componentTypes.GetOffset<Component>());
//        }
//    }
//
//    struct GameObjectNetworkData {
//        // Who owns this gameobject (and is sending sync data to other clients which recieve it)
//        // unsigned int owner;
//
//        // Value is pointless and undefined if not the owner.
//        // 
//        unsigned int syncAccumulator;
//    };
//
//    // If this is null, this gameobject will not be synced between server/client.
//    std::optional<GameObjectNetworkData> networkData;
//
//    // TODO: avoid not storing ptrs for components we don't have
//    //ComponentHandle<TransformComponent> transformComponent;
//    //ComponentHandle<RenderComponent> renderComponent;
//    // ComponentHandle<RenderComponentNoFO> renderComponentNoFO; these two components have exact same size and methods and you can only have one of them so we store both with the same pointer
//    //ComponentHandle<RigidbodyComponent> rigidbodyComponent;
//    //ComponentHandle<ColliderComponent> colliderComponent;
//    //ComponentHandle<PointLightComponent> pointLightComponent;
//    //ComponentHandle<AudioPlayerComponent> audioPlayerComponent;
//    //ComponentHandle<AnimationComponent> animationComponent;
//    //ComponentHandle<SpotLightComponent> spotLightComponent;
//
//    //LuaComponentHandle<TransformComponent> LuaGetTransform();
//    //LuaComponentHandle<RenderComponent> LuaGetRender();
//    //LuaComponentHandle<RigidbodyComponent> LuaGetRigidbody();
//    //LuaComponentHandle<ColliderComponent> LuaGetCollider();
//    //LuaComponentHandle<PointLightComponent> LuaGetPointLight();
//    //LuaComponentHandle<AudioPlayerComponent> LuaGetAudioPlayer();
//    //LuaComponentHandle<AnimationComponent> LuaGetAnimation();
//
//    ~GameObject();
//
//    // Destroys all components of the gameobject, sets all component handles to nullptr.
//    // ComponentHandle will catch if you try to access the components after this is called and throw an error.
//    // Will error if you try to destroy the same gameobject twice.
//    void Destroy();
//
//protected:
//    
//    // no copy constructing gameobjects.
//    GameObject(const GameObject&) = delete; 
//
//    // private because ComponentRegistry makes it and also because it needs to return a shared_ptr.
//    friend class std::shared_ptr<GameObject>;
//    
//    // groupIndex/Page are for returning the components to their ComponentGroup.
//    GameObject(const GameobjectCreateParams& params, const Archetype& cTypes, void* comps, unsigned int cgroupIndex, unsigned int cgroupPage);
//   
//private:
//    const unsigned int groupIndex;
//    const unsigned int groupPage;
//    void* const components;
//};