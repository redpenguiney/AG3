// This file is in charge of storing components in memory and providing functions with which to iterate through all gameobjects with certain components.
// JUST KIDDING DON"T USE THIS ITS A JOKE ITS  A LIE DONT TRUST IT
// Sorry for all the templates.

// HES WATCHING

#pragma once
#include <bitset>
#include <vector>
#include <memory>

#include "component_id.hpp"


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

//template <typename T>
//class GetID {
//	consteval int 
//};



// Describes a specific combination of components for a gameobject.
struct Archetype {

	// Key is ComponentBitIndex, value is -1 if no component, or the byte offset of the component if it's there.
	std::array<short, N_COMPONENT_TYPES> componentIds;

	template <std::derived_from<BaseComponent> ... Components>
	static Archetype FromComponents() {
		Archetype ids = Archetype();
		ids.componentIds.fill(-1);
		auto offsets = ComponentOffsets<Components...>();
		if constexpr ((std::is_same_v<TransformComponent, Components> || ...)) {
			ids.componentIds.at(ComponentBitIndex::Transform) = offsets.at(TupleIndex<TransformComponent, std::tuple<Components...>>::value);
		}
		if constexpr ((std::is_same_v<RenderComponent, Components> || ...)) {
			ids.componentIds.at(ComponentBitIndex::Render) = offsets.at(TupleIndex<RenderComponent, std::tuple<Components...>>::value);
		}
		if constexpr ((std::is_same_v<RenderComponentNoFO, Components> || ...)) {
			ids.componentIds.at(ComponentBitIndex::RenderNoFO) = offsets.at(TupleIndex<RenderComponentNoFO, std::tuple<Components...>>::value);
		}
		if constexpr ((std::is_same_v<ColliderComponent, Components> || ...)) {
			ids.componentIds.at(ComponentBitIndex::Collider) = offsets.at(TupleIndex<ColliderComponent, std::tuple<Components...>>::value);
		}
		if constexpr ((std::is_same_v<RigidbodyComponent, Components> || ...)) {
			ids.componentIds.at(ComponentBitIndex::Rigidbody) = offsets.at(TupleIndex<RigidbodyComponent, std::tuple<Components...>>::value);
		}
		if constexpr ((std::is_same_v<PointLightComponent, Components> || ...)) {
			ids.componentIds.at(ComponentBitIndex::Pointlight) = offsets.at(TupleIndex<PointLightComponent, std::tuple<Components...>>::value);
		}
		if constexpr ((std::is_same_v<AudioPlayerComponent, Components> || ...)) {
			ids.componentIds.at(ComponentBitIndex::AudioPlayer) = offsets.at(TupleIndex<AudioPlayerComponent, std::tuple<Components...>>::value);
		}
		if constexpr ((std::is_same_v<AnimationComponent, Components> || ...)) {
			ids.componentIds.at(ComponentBitIndex::Animation) = offsets.at(TupleIndex<AnimationComponent, std::tuple<Components...>>::value);
		}
		if constexpr ((std::is_same_v<SpotLightComponent, Components> || ...)) {
			ids.componentIds.at(ComponentBitIndex::Spotlight) = offsets.at(TupleIndex<SpotLightComponent, std::tuple<Components...>>::value);
		}
		return ids;
	}

	// returns -1 if not there
	template <std::derived_from<BaseComponent> Component>
	unsigned short GetOffset() const {
		return componentIds.at(ComponentIdFromType<Component>());
	}

	std::bitset<N_COMPONENT_TYPES> GetBitset() const {
		std::bitset<N_COMPONENT_TYPES> ret;
		int i = 0;
		for (auto& o : componentIds) {
			ret.set(i, o != -1);
			i++;
		}
	}
};

// type-erased interface for ComponentGroup.
class ComponentGroupInterface {
public:
	// The components stored inside this group.
	const Archetype contents;

	virtual ~ComponentGroupInterface();
	

	// allocates (cheaply) and returns a pointer to the components for a new gameobject, as well as the index and page in that order (needed to return the object).
	// Returned components are tightly packed and sorted by their ComponentBitIndex.
	// Does NOT call constructors.
	virtual std::tuple<void*, unsigned int, unsigned int> New() = 0;

	// Marks the components' memory as freed and available to new gameobjects.
	// Calls component destructors.
	// There should, obviously, be no references to this component when this is called.
	virtual void Return(unsigned int index, unsigned int page) = 0;

protected:
	ComponentGroupInterface(const Archetype&);
	
};



// Stores a specfic combination/archetype of components. So all gameobjects with just a transform would store their components in ComponentGroup<TransformComponent>,
// while those with transform and render would store them in ComponentGroup<TransformComponent, RenderComponent>.
// Uses a free list/bucket list combo to store components for rapid resizing + data locality
template <std::derived_from<BaseComponent> ... Components>
class ComponentGroup: public ComponentGroupInterface {
public:
	ComponentGroup() :
		ComponentGroupInterface(Archetype::FromComponents<Components...>()),
		archetypeSize((sizeof(Components) + ... + 0)),
		componentOffsets(ComponentOffsets<Components...>().begin(), ComponentOffsets<Components...>().end())
	{
		Assert(archetypeSize >= sizeof(void*)); // can't do free list if we don't have room for a pointer
		AddPage();
	}
	~ComponentGroup() {
		for (auto& ptr : pages) {
			free(ptr);
		}
	}

	// allocates (cheaply) and returns a pointer to the components for a new gameobject, as well as the index and page in that order (needed to return the object).
	// Returned components are tightly packed and sorted by their ComponentBitIndex.
	// Does NOT call constructors.
	std::tuple<void*, unsigned int, unsigned int> New() override {
		void* foundComponents = nullptr;
		unsigned int pageI = 0;

		// We use something called a "free list" to find a component set. If a component is not in use, the start of its memory is a pointer to the next unallocated component.
		for (pageI = 0; pageI < firstAvailable.size(); pageI++) {
			auto ptr = firstAvailable[pageI];
			if (ptr != nullptr) {
				// then this component will work
				foundComponents = ptr;

				// this ptr is pointing to a ptr to the next component set on this page (or nullptr if none); update firstAvailable which that
				firstAvailable[pageI] = *(void**)foundComponents;

				break;
			}
		}

		// if none of the pages have any space, make a new one
		if (!foundComponents) {
			AddPage();
			pageI = pages.size() - 1;
			foundComponents = firstAvailable.back();
			firstAvailable.back() = *(void**)foundComponents;
		}

		unsigned int index = ((char*)foundComponents - (char*)pages[pageI]) / archetypeSize;
		return std::make_tuple(foundComponents, index, pageI);
	}

	// Marks the components' memory as freed and available to new gameobjects.
	// Calls component destructors.
	// There should, obviously, be no references to this component when this is called.
	void Return(unsigned int index, unsigned int page) override {
		// call object destructors
		void* components = (void*)((char*)pages.at(page) + (index * archetypeSize));
		(DestructComponent<Components>(components), ...);

		// make the components the first node in the free list and have it point to what was previously the first node
		*(void**)components = firstAvailable.at(page);
		firstAvailable.at(page) = components;
	}

private:

	template <std::derived_from<BaseComponent> Component>
	void DestructComponent(void* slotPosition) {
		std::destroy_at((Component*)((char*)slotPosition + componentOffsets.at(TupleIndex<Component, std::tuple<Components ...>>::value)));
	}

	const unsigned int archetypeSize;
	const std::vector<unsigned int> componentOffsets;

	// vector of arrays of components. Components are tightly packed and sorted by their ComponentBitIndex.
	std::vector<void*> pages;

	// for free list; first unallocated group on each page. Nullptr if it's all in use.
	std::vector<void*> firstAvailable;

	constexpr static inline unsigned int COMPONENTS_PER_PAGE = 16384;

	// adds new page with room for COMPONENTS_PER_PAGE more objects
	void AddPage() {

		unsigned int index = pages.size();
		void* newPage = malloc(COMPONENTS_PER_PAGE * archetypeSize);

		firstAvailable.push_back(newPage);
		pages.push_back(newPage);

		// for free list, we have to, for each location a set of components would be written, write a pointer to the next such location
		for (unsigned int i = 0; i < COMPONENTS_PER_PAGE - 1; i++) {
			*((char**)newPage + (archetypeSize * i)) = ((char*)newPage + (archetypeSize * (i + 1)));
		}
		// last location just has nullptr/end of free list
		*((char**)newPage + (archetypeSize * (COMPONENTS_PER_PAGE - 1))) = nullptr;

	}
};