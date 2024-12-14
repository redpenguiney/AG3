#pragma once
#include "gameobjects/component_id.hpp"
#include "gameobjects/component_pool.hpp"

#include <type_traits>
#include <bitset>
#include <memory>

// TODO: weak_ptr version?
// An object that stores a component and a shared_ptr to the gameobject that component came from. 
// Used to guarantee that the component reference returned by GameObject::Get() isn't left dangling when its gameobject is destructed.
// Can store nullptr, in which case dereferencing will abort.
template <std::derived_from<BaseComponent> Component>
class ComponentHandle {
public:
	ComponentHandle(Component* const comp, const std::shared_ptr<GameObject>& obj) :
		object(obj),
		component(comp)
	{

	}
	ComponentHandle(const ComponentHandle<Component>&) = default;
	ComponentHandle(const ComponentHandle<Component>&& other) {
		other.object = nullptr;
		other.component = nullptr;
	}

	Component& operator*() const {
		Assert(component != nullptr);
		return *component;
	}
	Component* operator->() const {
		Assert(component != nullptr);
		return component;
	}

	// Returns true if not nullptr
	explicit operator bool() const {
		return component != nullptr;
	}

	// might return nullptr, be careful.
	// only exists so you can pass a reference to a component to a function
	Component* const GetPtr() const {
		return component;
	}

	// If ptr != nullptr/it hasn't already been cleared, returns it to the pool (thus destructing the component), and sets ptr to nullptr.
	// Gameobjects do this for all their components when Destroy() is called on them.
	//void Clear();

private:
	const std::shared_ptr<GameObject> object;
	Component* const component;
};

// Specifies stuff about the gameobject to create, like what components it has (and what values to initialize those components with).
// Pass to GameObject::New().
struct GameobjectCreateParams {
	std::optional<std::shared_ptr<class PhysicsMesh>> physMesh;
	unsigned int meshId; // ignore if not rendering
	unsigned int materialId; // defaults to 0 for no material. ignore if not rendering
	unsigned int shaderId; // defaults to 0 for default shader. ignore if not rendering

	std::optional<std::shared_ptr<Sound>> sound; // for audio player components

	GameobjectCreateParams(std::vector<ComponentBitIndex::ComponentBitIndex> componentList) :
		physMesh(std::nullopt),
		meshId(0),
		materialId(0),
		shaderId(0),
		sound(std::nullopt)
	{
		// bitset defaults to all false so we good
		for (auto& i : componentList) {
			requestedComponents[i] = true;
		}
	}

	bool HasComponent(ComponentBitIndex::ComponentBitIndex bitIndex) const {
		return requestedComponents.test(bitIndex);
	}

private:
	friend class GameObject;
	std::bitset<N_COMPONENT_TYPES> requestedComponents;
};

// TODO: shared_from_this does increase size of gameobject by sizeof(std::weak_ptr), all for the sake of making someone who is guaranteed to already have the neccesary std::shared_ptr not have to pass it in themselves.
// Ditch for performance?
// The gameobject system uses ECS (google it).
// If you don't know what a gameobject is, no offense but maybe you shouldn't be reading this..
class GameObject : public std::enable_shared_from_this<GameObject> {
public:
	// name is not used anywhere except for debugging and by the user; set as you please
	std::string name;

	~GameObject();

	// static method instead of static variable so we ensure that the GAMEOBJECTS map is destroyed before the singletons that component destructors depend on
	static std::unordered_map<GameObject*, std::shared_ptr<GameObject>>& GAMEOBJECTS();

	// Factory constructor for gameobjects.
	// Initialize other singletons before calling.
	// TODO: option which returns weak_ptr?
	static std::shared_ptr<GameObject> New(const GameobjectCreateParams& params);

	// lets you create subclasses of gameobject correctly
	//template<std::derived_from<GameObject> T, typename ... tArgs>
	//static std::shared_ptr<T> New(const GameobjectCreateParams& params, tArgs ...);

	// Returns the given component (or an empty handle if it doesn't exist). The component handle will evalulate to true if the component exists.
	// The ComponentHandle contains a shared_ptr to the gameobject, so the gameobject will not be destroyed and the component will always remain valid for the entirety of the handle's lifetime.
	// WARNING!!! WARNING!!! You cannot call Get() inside component constructors. 
		// (Maybe)RawGet is fine, but because this will attempt to get a shared_ptr using shared_from_this before GameObject::New() makes a shared_ptr, std::bad_weak_ptr will be thrown.
	template <std::derived_from<BaseComponent> Component>
	ComponentHandle<Component> Get() {
		return ComponentHandle<Component>(MaybeRawGet<Component>(), shared_from_this());
	}

	// Returns the given component. Aborts if this gameobject does not have that component.
	// Be careful: the lifetime of this pointer is as long as the lifetime of the gameobject.
	template <std::derived_from<BaseComponent> Component>
	Component* RawGet() {
		auto component = MaybeRawGet<Component>();
		Assert(component != nullptr);
		return component;
	}

	// Returns the given component, or nullptr if this gameobject does not have that component.
	// Be careful: the lifetime of this pointer is as long as the lifetime of the gameobject.
	template <std::derived_from<BaseComponent> Component>
	Component* MaybeRawGet() {
		// verify pool has component and if so find byte offset
		int offset = -1;

		for (auto& info : pool->componentLayout) {
			if (info.componentId == ComponentIdFromType<Component>()) {
				offset = info.offset;
				break;
			}
		}

		if (offset == -1) { // then component was not found
			return nullptr;
		}

		uint8_t* pagePtr = pool->pages[page];
		return (Component*)(pagePtr + (objectIndex * pool->objectSize + offset));
	}

	// Specialization for render components so that RenderComponentNoFO can be used as a RenderComponent.
	template<>
	RenderComponent* MaybeRawGet<RenderComponent>() {

		// verify pool has component and if so find byte offset
		int offset = -1;

		for (auto& info : pool->componentLayout) {
			if (info.componentId == ComponentBitIndex::Render || info.componentId == ComponentBitIndex::RenderNoFO) {
				offset = info.offset;
				break;
			}
		}

		if (offset == -1) { // then component was not found
			return nullptr;
		}

		uint8_t* pagePtr = pool->pages[page];
		return (RenderComponent*)(pagePtr + (objectIndex * pool->objectSize + offset));
	}

	// DOES NOT neccesarily destroy the gameobject. 
	// All it does is remove the shared_ptr stored in the GAMEOBJECTS map, meaning that when all references aka shared_ptrs you 
		// have to the gameobject are destroyed (ComponentHandles count as references), the gameobject will be destroyed.
	// If you want Destroy() to always immediately destroy the gameobject regardless of other references to it, do not store shared_ptrs to your gameobjects; use weak_ptr instead.
	void Destroy();

private:
	// A forward iterator for systems to iterate through components with.
	template <std::derived_from<BaseComponent> ... Components>
	class SystemForwardIterator {
	public:
		using iterator_category = std::forward_iterator_tag;

		// meaningless since you can't subtract this iterator
		using difference_type = std::ptrdiff_t;

		// what you get by iterating through this
		using value_type = std::tuple<Components* ...>;
		using pointer = value_type*;
		using reference = value_type&;

		SystemForwardIterator(const std::vector<ComponentPool*>& poolVec) :
			pools(poolVec),
			poolIndex(0),
			pageIndex(0),
			objectIndex(0),
			currentLiveChecker(nullptr)
		{
			if (pools.size() > 0) { // if we have nothing to iterate through we can't write the tuple
				WriteCurrentTuple();
			}
		}

		template <typename T>
		void AddIfNotNull(T& t) {
			if (t != nullptr) {
				t = (T)(((uint8_t*)t) + pools[poolIndex]->objectSize);
			}
		}

		// call in for loop to check when the iterator has nothing left to offer. Returns false when iterator has nothing left and should not be dereferenced.
		bool Valid() {
			if (pools.size() == poolIndex) {
				return false;
			}
			else {
				return true;
			}
		}

		// int argument is there to specify that we're overloading postfix, not prefix; doesn't do anything
		void operator++(int) {

			//DebugLogInfo("Postfix");
			objectIndex++;
			if (objectIndex == ComponentPool::COMPONENTS_PER_PAGE) {

				objectIndex = 0;
				pageIndex++;

				if (pageIndex == pools[poolIndex]->pages.size()) {
					pageIndex = 0;
					poolIndex++;

					if (!Valid()) {
						return;
					}
				}

				// we're on a different page so we can't just increment pointers
				WriteCurrentTuple();
			}
			else {
				// increment pointers (that aren't nullptr)
				std::apply([this](auto& ... x) {(..., AddIfNotNull(x)); }, currentTuple);
				currentLiveChecker = (uint8_t**)((uint8_t*)currentLiveChecker + pools[poolIndex]->objectSize);
			}

			// check if the object we're now on is in use, or if we should skip it
			// objects in use have their first sizeof(uint8_t) bytes set to nullptr
			if (*currentLiveChecker != nullptr && Valid()) { // need to make sure we aren't going past end of iterator though or we'll just postfix forever until stack oveflow
				(*this)++;
			}
		}

		value_type& operator*() {
			//DebugLogInfo("Deref, return ", objectIndex);
			return currentTuple;
		}

	private:
		// stolen from stack overflow
		template <class T, class Tuple>
		struct TupleIndex;

		template <class T, class... Types>
		struct TupleIndex<T, std::tuple<T, Types...>> {
			static const std::size_t value = 0;
		};

		template <class T, class U, class... Types>
		struct TupleIndex<T, std::tuple<U, Types...>> {
			static const std::size_t value = 1 + TupleIndex<T, std::tuple<Types...>>::value;
		};

		template <typename ComponentPtr>
		void WriteTupleElement(ComponentPtr p) {
			static_assert(std::is_pointer<ComponentPtr>::value);

			ComponentPool* pool = pools[poolIndex];

			short offset = -1;
			for (auto& memoryInfo : pool->componentLayout) {

				constexpr int id(ComponentIdFromType < typename std::remove_pointer<ComponentPtr>::type>());
				if (id == memoryInfo.componentId) {
					offset = memoryInfo.offset;
				}

			}

			//typedef I = TupleIndex<ComponentPtr, value_type>::value;
			//static_assert(I == 1000);

			if (offset == -1) {
				std::get<TupleIndex<ComponentPtr, value_type>::value>(currentTuple) = nullptr;
			}
			else {
				void* page = pool->pages[pageIndex];
				std::get<TupleIndex<ComponentPtr, value_type>::value>(currentTuple) = (ComponentPtr)((uint8_t*)page + offset);
			}
		}

		void WriteCurrentTuple() {
			auto& pool = pools[poolIndex];
			uint8_t* page = pool->pages[pageIndex];
			currentLiveChecker = (uint8_t**)(void*)page;

			std::apply([this](auto& ... value) {
				(..., WriteTupleElement(value));
				}, currentTuple);
		}

		const std::vector<ComponentPool*> pools;

		unsigned int poolIndex;

		unsigned int pageIndex;

		unsigned int objectIndex;

		value_type currentTuple;
		uint8_t** currentLiveChecker;
	};

public:

	// Returns an iterator so you can iterate through all gameobjects that have the requested components (except you don't actually get the gameobject, just a tuple of components).
	// Components not specified as required may be nullptr. Check.
	template <std::derived_from<BaseComponent> ... Components>
	static SystemForwardIterator<Components...> SystemGetComponents(std::vector<ComponentBitIndex::ComponentBitIndex> requiredComponents) {
		std::bitset<N_COMPONENT_TYPES> requestedArchetype = GameobjectCreateParams(requiredComponents).requestedComponents;
		std::vector<ComponentPool*> matchingPools;
		for (auto& [archetype, pool] : COMPONENT_POOLS) {
			if ((archetype & requestedArchetype) == requestedArchetype) {
				matchingPools.push_back(pool.get());
			}
		}
		return SystemForwardIterator<Components...>(matchingPools);
	}

protected:
	// protected so you have to use factory constructor
	GameObject(const GameobjectCreateParams& params);

private:

	// delegating constructor used by main constructor
	GameObject(std::tuple<ComponentPool*, int, int> data);

	ComponentPool* const pool;
	const int objectIndex;
	const int page;

	// unique_ptr so it doesn't memory leak
	static inline std::unordered_map<std::bitset<N_COMPONENT_TYPES>, std::unique_ptr<ComponentPool>> COMPONENT_POOLS;

	static std::tuple<ComponentPool*, int, int> GetNewGameobjectComponentData(const GameobjectCreateParams& params);
};