// This file is in charge of storing components in memory and providing functions with which to iterate through all gameobjects with certain components.
// Sorry for all the templates.

#pragma once
#include <bitset>
#include <vector>
#include <memory>

enum class ComponentBitIndex {
    Transform = 0,
    Render = 1,
    Collider = 2,
    Rigidbody = 3,
    Pointlight = 4,
    RenderNoFO = 5,
    AudioPlayer = 6,
    Animation = 7,
    Spotlight = 8
};

template <class T, class Tuple>
struct TupleIndex;

// stolen from stack overflow
template <class T, typename... Ts>
struct TupleIndex<T, std::tuple<Ts...>>
{

	static constexpr std::size_t index = []() {
		constexpr std::array<bool, sizeof...(Ts)> a{ { std::is_same<T, Ts>::value... } };

		// You might easily handle duplicate index too (take the last, throw, ...)
		// Here, we select the first one.
		const auto it = std::find(a.begin(), a.end(), true);

		// You might choose other options for not present.

		// As we are in constant expression, we will have compilation error.
		// and not a runtime expection :-)
		if (it == a.end()) throw std::runtime_error("Not present");

		return std::distance(a.begin(), it);
		}();
};

//template <typename T>
//class GetID {
//	consteval int 
//};

// How many different component classes there are. (can be greater just not less)
static inline const unsigned int N_COMPONENT_TYPES = 16;

namespace {
	// not stolen from stack overflow, surprisingly
	template <std::derived_from<BaseComponent> ... Components>
	std::vector<unsigned int> ComponentOffsets() {
		std::vector<unsigned int> offsets;
		int offset = 0;
		using I = std::size_t[];
		(void)(I{ 0u, (offsets.push_back(offset), offset += sizeof(TTypes))... });
		return sum;
	}
}

// Describes a specific combination of components for a gameobject.
struct Archetype {

	template <typename ... Components>
	static Archetype FromComponents() {
		Archetype ids(false);
		if constexpr ((std::is_same_v<TransformComponent, Args> || ...)) {
			ids.componentIds.set(ComponentBitIndex::Transform, true);
		}
		if constexpr ((std::is_same_v<RenderComponent, Args> || ...)) {
			ids.componentIds.set(ComponentBitIndex::Render, true);
		}
		if constexpr ((std::is_same_v<RenderComponentNoFO, Args> || ...)) {
			ids.componentIds.set(ComponentBitIndex::RenderNoFO, true);
		}
		if constexpr ((std::is_same_v<ColliderComponent, Args> || ...)) {
			ids.componentIds.set(ComponentBitIndex::Collider, true);
		}
		if constexpr ((std::is_same_v<RigidbodyComponent, Args> || ...)) {
			ids.componentIds.set(ComponentBitIndex::Rigidbody, true);
		}
		if constexpr ((std::is_same_v<PointLightComponent, Args> || ...)) {
			ids.componentIds.set(ComponentBitIndex::Pointlight, true);
		}
		if constexpr ((std::is_same_v<AudioPlayerComponent, Args> || ...)) {
			ids.componentIds.set(ComponentBitIndex::AudioPlayer, true);
		}
		if constexpr ((std::is_same_v<AnimationComponent, Args> || ...)) {
			ids.componentIds.set(ComponentBitIndex::Animation, true);
		}
		if constexpr ((std::is_same_v<SpotLightComponent, Args> || ...)) {
			ids.componentIds.set(ComponentBitIndex::Spotlight, true);
		}
		return ids;
	}

	// Key is ComponentBitIndex, bit is set if the gameobject has that component.
	std::bitset<N_COMPONENT_TYPES> componentIds;

	template <std::derived_from<BaseComponent> Component>
	unsigned int GetOffset() const {
		constexpr auto offsets = ComponentOffsets<Components>();
		return offsets.at(TupleIndex<Component, std::tuple<Components ...>>::value);
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
		ComponentGroupInterface(Archetype::FromComponents<Components>()),
		archetypeSize(sizeof(Components) + ... + 0),
		componentOffsets(ComponentOffsets<Components>())
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

		// We use something called a "free list" to find a component set. If a component is not in use, the start of its memory is a pointer to the next unallocated component.
		for (unsigned int pageI = 0; pageI < firstAvailable.size(); pageI++) {
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
		if (!foundObject) {
			AddPage();
			foundComponents = firstAvailable.back();
			firstAvailable.back() = *(void**)foundComponents;
		}

		unsigned int index = ((char*)foundComponents - (char*)pages[pageI]) / archetypeSize;
		return std::make_tuple<foundComponents, index, pageI>;
	}

	// Marks the components' memory as freed and available to new gameobjects.
	// Calls component destructors.
	// There should, obviously, be no references to this component when this is called.
	void Return(unsigned int index, unsigned int page) override {
		// call object destructors
		(DestructComponent<Components>(), ...);

		// make the components the first node in the free list and have it point to what was previously the first node
		*(char*)components = firstAvailable.at(page);
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
			*((char*)newPage + (archetypeSize * i)) = ((char*)newPage + (archetypeSize * (i + 1)));
		}
		// last location just has nullptr/end of free list
		*((char*)newPage + (archetypeSize * i)) = nullptr;

	}
};