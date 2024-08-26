#pragma once
#include <vector>
#include <bitset>
#include "component_id.hpp"

// Stores a specfic combination/archetype of components. So all gameobjects with just a transform would store their components in ComponentGroup<TransformComponent>,
// while those with transform and render would store them in ComponentGroup<TransformComponent, RenderComponent>.
// Uses a free list/bucket list combo to store components for rapid resizing + data locality
class ComponentPool {
public:
	struct ComponentMemoryInfo {
		unsigned int size; // size of the component in bytes
		unsigned int offset;
		int componentId;
	};

	constexpr static inline unsigned int COMPONENTS_PER_PAGE = 4096;

	// used to get individual components from the void* returned by GetObject(). Sorted by component id.
	const std::vector<ComponentMemoryInfo> componentLayout;

	ComponentPool(const std::bitset<N_COMPONENT_TYPES>& components);
	~ComponentPool();

	// Returns a pointer to a new object's components, the page that pointer is on, and its object index on that page in that order..
	// These components are uninitialized and you must call their constructors.
	std::tuple<void*, int, int> GetObject();

	// Returns an object to the component pool.
	// Doesn't call destructors.
	void ReturnObject(int pageIndex, int objectIndex);

	
	
private:
	// represents that there's nothing else in the free list on this page after this one
	constexpr static inline uint8_t* LAST_COMPONENT = (uint8_t*)1;

	// the combined size in bytes of an object's components in the pool, + sizeof(uint8_t*) bytes.
	// This uint8_t* at the start of each object's memory stores nullptr if the object is live/in use, LAST_COMPONENT if there are no more available objects in the page's free list after this one, or a pointer to the next free component otherwise
	const unsigned int objectSize;

	// Vector of arrays of length objectSize * COMPONENTS_PER_PAGE which store components.
	// to avoid frequent/expensive reallocations and ensure components are in mostly contiguous memory for cache-friendliness, we allocate components in batches of COMPONENTS_PER_PAGE.
	std::vector<uint8_t*> pages;

	// for each page, a pointer to the first free object in each page, or nullptr if none.
	std::vector<uint8_t*> firstFree;

	// creates a new page with room for COMPONENTS_PER_PAGE more objects.
	void AddPage();

	friend class GameObject;
};