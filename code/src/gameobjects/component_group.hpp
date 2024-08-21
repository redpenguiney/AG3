#pragma once

// Stores a specfic combination of components. So all gameobjects with just a transform would store their components in ComponentGroup<TransformComponent>,
// while those with transform and render would store them in ComponentGroup<TransformComponent, RenderComponent>.
// Uses a free list/bucket list combo to store components for rapid iteration/extension + data locality
template <typename ... T>
class ComponentGroup {
public:
	typedef std::tuple<T...> Group;

	// returns 
	void New();

private:
	// vector of arrays of groups
	std::vector<Group*> pages;

	// for free list; first unallocated group on each page  
	std::vector<Group*> firstAvailable;
};